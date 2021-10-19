/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert / Helene
 Date:          Feb 2012
________________________________________________________________________

-*/

#include "uirockphysform.h"
#include "rockphysics.h"
#include "mathformula.h"
#include "mathproperty.h"
#include "mathspecvars.h"
#include "unitofmeasure.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimnemonicsel.h"
#include "uimsg.h"
#include "uitextedit.h"
#include "uitoolbutton.h"
#include "uiofferinfo.h"

#define mMaxNrCsts	5


class uiRockPhysConstantFld : public uiGroup
{
public:


uiRockPhysConstantFld( uiParent* p )
    : uiGroup(p,"Rock Physics Constant Field")
{
    nmlbl_ = new uiLabel( this, uiString::empty() );
    nmlbl_->setPrefWidthInChar( 30 );
    nmlbl_->setAlignment( Alignment::Right );

    valfld_ = new uiGenInput( this, uiString::empty(), FloatInpSpec() );
    valfld_->attach( rightOf, nmlbl_ );

    infofld_ = new uiOfferInfo( this, false );
    infofld_->attach( rightOf, valfld_ );

    rangelbl_ = new uiLabel( this, uiString::empty() );
    rangelbl_->setPrefWidthInChar( 30 );
    rangelbl_->attach( rightOf, infofld_ );

    setHAlignObj( valfld_ );
}


float value() const
{
    return valfld_->getFValue();
}


void update( const RockPhysics::Formula::ConstDef* pcd )
{
    display( pcd );
    if ( !pcd )
	{ cstnm_.setEmpty(); infofld_->setInfo( "" ); return; }

    const RockPhysics::Formula::ConstDef& cd = *pcd;
    cstnm_ = cd.name();

    nmlbl_->setText( od_static_tr("update",
			      "Value for '%1'").arg(mToUiStringTodo(cstnm_)) );
    infofld_->setInfo( cd.desc_, od_static_tr("update","Information on '%1'").
						      arg(toUiString(cstnm_)) );
    valfld_->setValue( cd.defaultval_ );

    const bool haverg = !cd.typicalrg_.isUdf();
    if ( haverg )
    {
	uiString rgstr = od_static_tr("update","Typical range: [%1,%2]").
			 arg(cd.typicalrg_.start).arg(cd.typicalrg_.stop );
	rangelbl_->setText( rgstr );
    }
    rangelbl_->display( haverg );
}


    BufferString	cstnm_;
    BufferString	desc_;

    uiGenInput*		valfld_;
    uiLabel*		nmlbl_;
    uiLabel*		rangelbl_;
    uiOfferInfo*	infofld_;

};




uiRockPhysForm::uiRockPhysForm( uiParent* p )
    : uiGroup(p,"RockPhysics Formula Selector")
{
    BufferStringSet typnms( Mnemonic::StdTypeNames() );
    ManagedObjectSet<MnemonicSelection> mnsels;
    for ( int idx=typnms.size()-1; idx>=0; idx-- )
    {
	const Mnemonic::StdType typ
			= Mnemonic::parseEnumStdType( typnms.get(idx) );
	auto* mnsel = new MnemonicSelection;
	ROCKPHYSFORMS().getRelevant( typ, *mnsel );
	if ( mnsel->isEmpty() )
	{
	    typnms.removeSingle( idx );
	    delete mnsel;
	}
	else
	    mnsels.insertAt( mnsel, 0 );
    }

    const int defidx = typnms.indexOf(
		    Mnemonic::StdTypeDef().getKeyForIndex( Mnemonic::Imp ) );

    auto* lcb = new uiLabeledComboBox( this, typnms, tr("Property Type") );
    typfld_ = lcb->box();
    typfld_->setHSzPol( uiObject::MedMax );
    if ( typnms.validIdx(defidx) )
	typfld_->setCurrentItem( defidx );
    mAttachCB( typfld_->selectionChanged, uiRockPhysForm::typSel );

    for ( const auto* mnsel : mnsels )
    {
	uiMnemonicsSel::Setup umnselsu( mnsel );
	auto* mnselfld = new uiMnemonicsSel( this, umnselsu );
	mnselfld->attach( alignedBelow, lcb );
	mnselflds_.add( mnselfld );
	uiComboBox* mnselcbfld = mnselfld->box();
	mAttachCB( mnselcbfld->selectionChanged, uiRockPhysForm::mnSel );
    }

    createFlds( mnselflds_.isEmpty()
			? (uiObject*)lcb->attachObj()
			: (uiObject*)mnselflds_.first()->attachObj() );
}


uiRockPhysForm::uiRockPhysForm( uiParent* p, const Mnemonic& mn )
    : uiGroup(p,"RockPhyics Formula Selector")
    , fixedmn_(&mn)
{
    createFlds( nullptr );
}


uiRockPhysForm::~uiRockPhysForm()
{
    detachAllNotifiers();
}


void uiRockPhysForm::createFlds( uiObject* attobj )
{
    auto* lcb = new uiLabeledComboBox( this, tr("Formula") );
    lcb->label()->setPrefWidthInChar( 35 );
    lcb->label()->setAlignment( Alignment::Right );
    nmfld_ = lcb->box();
    nmfld_->setHSzPol( uiObject::WideMax );
    mAttachCB( nmfld_->selectionChanged, uiRockPhysForm::nameSel );

    formulafld_ = new uiTextEdit( this, "Formula", true );
    formulafld_->setPrefHeightInChar( 2 );
    formulafld_->setPrefWidthInChar( 80 );
    formulafld_->setStretch(2,0);
    formulafld_->attach( ensureBelow, lcb );

    if ( attobj )
	attobj->attach( alignedAbove, lcb );

    descriptionfld_ = new uiTextBrowser( this, "Formula Desc", mUdf(int),
					 false );
    descriptionfld_->setPrefHeightInChar( 4 );
    descriptionfld_->setPrefWidthInChar( 80 );
    descriptionfld_->setStretch(2,0);
    descriptionfld_->attach( ensureBelow, formulafld_ );

    for ( int idx=0; idx<mMaxNrCsts; idx++ )
     {
	auto* rpcfld = new uiRockPhysConstantFld( this );
	if ( idx )
	    rpcfld->attach( alignedBelow, cstflds_[idx-1] );
	else
	{
	    rpcfld->attach( alignedBelow, lcb );
	    rpcfld->attach( ensureBelow, descriptionfld_ );
	}

	cstflds_ += rpcfld;
    }

    setHAlignObj( lcb );
    mAttachCB( postFinalise(), uiRockPhysForm::initGrp );
}


void uiRockPhysForm::initGrp( CallBacker* )
{
    if ( typfld_ )
	typSel( nullptr );
    else if ( fixedmn_ )
	setType( *fixedmn_ );
}


void uiRockPhysForm::typSel( CallBacker* )
{
    const int typidx = typfld_->currentItem();
    for ( int idx=0; idx<mnselflds_.size(); idx++ )
	mnselflds_.get( idx )->display( idx == typidx );

    mnSel( mnselflds_.get(typidx)->box() );
}


void uiRockPhysForm::mnSel( CallBacker* cb )
{
    mDynamicCastGet(uiComboBox*,mnselfldbox,cb)
    if ( !mnselfldbox )
	return;

    for ( const auto mnselfld : mnselflds_ )
    {
	if ( mnselfld->box() != mnselfldbox )
	    continue;

	const Mnemonic* mn = mnselfld->mnemonic();
	if ( mn )
	    setType( *mn );
    }
}


void uiRockPhysForm::setType( const Mnemonic& mn )
{
    BufferStringSet nms;
    ROCKPHYSFORMS().getRelevant( mn, nms );
    nmfld_->setEmpty();
    nmfld_->addItems( nms );
    if ( !nms.isEmpty() )
	nmfld_->setCurrentItem( 0 );

    nameSel( nullptr );
}


void uiRockPhysForm::nameSel( CallBacker* )
{
    const FixedString txt = nmfld_->text();
    if ( txt.isEmpty() )
	return;

    const RockPhysics::Formula* fm = ROCKPHYSFORMS().getByName( txt );
    if ( !fm )
	{ uiMSG().error( tr("Internal: formula not found") ); return;}

    const RockPhysics::Formula& rpfm = *fm;
    const int nrconsts = rpfm.constdefs_.size();
    for ( int idx=0; idx<cstflds_.size(); idx++ )
	cstflds_[idx]->update( idx<nrconsts ? rpfm.constdefs_[idx] : 0 );

    formulafld_->setText( getFormText(rpfm,true) );
    descriptionfld_->setHtmlText( rpfm.desc_ );
}


BufferString uiRockPhysForm::getFormText( const RockPhysics::Formula& rpfm,
					  bool fortxtdisp ) const
{
    BufferString formstr( rpfm.def_ );
    Math::Formula form( true, formstr );
    int ivardef = 0; int iconstdef = 0;
    for ( int iinp=0; iinp<form.nrInputs(); iinp++ )
    {
	BufferString formval;
	if ( form.isConst(iinp) )
	{
	    if ( fortxtdisp )
		formval = rpfm.constdefs_[iconstdef]->name();
	    else
		formval.set( cstflds_[iconstdef]->value() );
	    iconstdef++;
	}
	else
	{
	    formval = rpfm.vardefs_[ivardef]->name();
	    formval.clean();
	    ivardef++;
	}
	formstr.replace( form.variableName(iinp), formval );
    }

    return formstr;
}


const char* uiRockPhysForm::getText( bool replcst ) const
{
    if ( !replcst )
	return nmfld_->text();

    Math::Formula form;
    getFormulaInfo( form );
    mDeclStaticString(ret);
    ret = form.text();
    return ret;
}


bool uiRockPhysForm::getFormulaInfo( Math::Formula& form,
			     TypeSet<Mnemonic::StdType>* sttypes ) const
{
    const bool res = getFormulaInfo( form );
    if ( res && sttypes )
    {
	sttypes->setEmpty();
	for ( const auto& specvar : form.specVars() )
	    sttypes->add( specvar.getMnemonic().stdType() );
    }

    return res;
}


bool uiRockPhysForm::getFormulaInfo( Math::Formula& form ) const
{
    const FixedString txt = nmfld_->text();
    if ( txt.isEmpty() )
	{ uiMSG().error( tr("No formula name selected") ); return false; }

    const RockPhysics::Formula* fm = ROCKPHYSFORMS().getByName( txt );
    if ( !fm )
	{ uiMSG().error( tr("Internal: formula not found") ); return false; }
    const RockPhysics::Formula& rpfm = *fm;

    form.setText( getFormText(rpfm,false) );

    const int nrinps = form.nrInputs();
    if ( nrinps != fm->vardefs_.size() )
    {
	BufferString msg; msg.set(nrinps).add("!=").add(fm->vardefs_.size());
	pErrMsg( msg );
    }

    for ( int idx=0; idx<fm->vardefs_.size(); idx++ )
    {
	const RockPhysics::Formula::VarDef& vardef = *fm->vardefs_.get( idx );
	form.setInputDef( idx, vardef.desc_ );
	form.setInputMnemonic( idx, vardef.mn_ );
	form.setInputFormUnit( idx, UoMR().get( vardef.unit_ ) );
    }

    form.setOutputMnemonic( fm->mn_ );
    form.setOutputFormUnit( UoMR().get(fm->unit_) );
    return true;
}


uiRetVal uiRockPhysForm::isOK() const
{
    uiRetVal ret;
    for ( const auto* cstfld : cstflds_ )
    {
	if ( cstfld->attachObj()->isDisplayed() && mIsUdf(cstfld->value()) )
	{
	    const uiString errmsg =
		    tr("Please provide a value for constant '%1'" )
					.arg( cstfld->cstnm_ );
	    ret.add( errmsg );
	}
    }

    return ret;
}
