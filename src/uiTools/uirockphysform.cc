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
#include "unitofmeasure.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
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
    nmlbl_ = new uiLabel( this, uiStrings::sEmptyString() );
    nmlbl_->setPrefWidthInChar( 30 );
    nmlbl_->setAlignment( Alignment::Right );

    valfld_ = new uiGenInput( this, uiStrings::sEmptyString(), FloatInpSpec() );
    valfld_->attach( rightOf, nmlbl_ );

    infofld_ = new uiOfferInfo( this, false );
    infofld_->attach( rightOf, valfld_ );

    rangelbl_ = new uiLabel( this, uiStrings::sEmptyString() );
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
    : uiGroup(p,"RockPhyics Formula Selector")
    , fixedtype_(PropertyRef::Den)
{
    BufferStringSet typnms( PropertyRef::StdTypeNames() );
    for ( int idx=0; idx<typnms.size(); idx++ )
    {
	BufferStringSet nms;
	const PropertyRef::StdType typ
			= PropertyRef::parseEnumStdType( typnms.get(idx) );
	ROCKPHYSFORMS().getRelevant( typ, nms );
	if ( nms.isEmpty() )
	    { typnms.removeSingle( idx ); idx--; }
    }

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, typnms,
						    tr("Property Type") );
    typfld_ = lcb->box();
    typfld_->setHSzPol( uiObject::MedMax );
    typfld_->selectionChanged.notify( mCB(this,uiRockPhysForm,typSel) );

    createFlds( lcb );
}


uiRockPhysForm::uiRockPhysForm( uiParent* p, PropertyRef::StdType typ )
    : uiGroup(p,"RockPhyics Formula Selector")
    , fixedtype_(typ)
    , typfld_(0)
{
    createFlds( 0 );
}


void uiRockPhysForm::createFlds( uiGroup* attobj )
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, tr("Formula") );
    lcb->box()->setHSzPol( uiObject::WideMax );
    lcb->label()->setPrefWidthInChar( 35 );
    lcb->label()->setAlignment( Alignment::Right );

    nmfld_ = lcb->box();
    nmfld_->selectionChanged.notify( mCB(this,uiRockPhysForm,nameSel) );

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
	uiRockPhysConstantFld* rpcfld = new uiRockPhysConstantFld( this );
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
    setType( fixedtype_ );
}


PropertyRef::StdType uiRockPhysForm::getType() const
{
    if ( !typfld_ )
	return fixedtype_;

    const char* txt = typfld_->text();
    if ( !txt || !*txt )
	return PropertyRef::Other;

    return PropertyRef::parseEnumStdType(txt);
}


void uiRockPhysForm::setType( PropertyRef::StdType typ )
{
    if ( typfld_ )
	typfld_->setText( PropertyRef::toString(typ) );

    BufferStringSet nms;
    ROCKPHYSFORMS().getRelevant( typ, nms );

    NotifyStopper ns( nmfld_->selectionChanged );

    nmfld_->setEmpty();
    nmfld_->addItems( nms );
    nmfld_->setCurrentItem( 0 );

    nameSel( 0 );
}


void uiRockPhysForm::typSel( CallBacker* cb )
{
    if ( !typfld_ ) return;

    const char* txt = typfld_->text();
    if ( !txt || !*txt ) return;

    setType( PropertyRef::parseEnumStdType(txt) );
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


void uiRockPhysForm::nameSel( CallBacker* cb )
{
    const char* txt = nmfld_->text();
    if ( !txt || !*txt ) return;

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


const char* uiRockPhysForm::getText( bool replcst ) const
{
    if ( !replcst )
	return nmfld_->text();

    Math::Formula form; getFormulaInfo( form );
    mDeclStaticString(ret);
    ret = form.text();
    return ret;
}


bool uiRockPhysForm::getFormulaInfo( Math::Formula& form,
			     TypeSet<PropertyRef::StdType>* sttypes ) const
{
    if ( sttypes )
	sttypes->setEmpty();

    const char* txt = nmfld_->text();
    if ( !txt || !*txt )
	{ uiMSG().error( tr("No formula name selected") ); return false; }

    const RockPhysics::Formula* fm = ROCKPHYSFORMS().getByName( txt );
    if ( !fm )
	{ uiMSG().error( tr("Internal: formula not found") ); return false; }
    const RockPhysics::Formula& rpfm = *fm;

    form.setText( getFormText(rpfm,false) );

    int nrinps = form.nrInputs();
    if ( nrinps != fm->vardefs_.size() )
    {
	BufferString msg; msg.set(nrinps).add("!=").add(fm->vardefs_.size());
	pErrMsg( msg );
	if ( nrinps > fm->vardefs_.size() )
	    nrinps = fm->vardefs_.size();
    }

    for ( int idx=0; idx<nrinps; idx++ )
    {
	const RockPhysics::Formula::VarDef& vd = *fm->vardefs_[idx];
	form.setInputUnit( idx, UoMR().get(vd.unit_) );
	if ( sttypes )
	    *sttypes += vd.type_;
    }

    form.setOutputUnit( UoMR().get(fm->unit_) );
    return true;
}


bool uiRockPhysForm::isOK()
{
    for ( int idx=0; idx<cstflds_.size(); idx++ )
    {
	if ( cstflds_[idx]->attachObj()->isDisplayed()
	     && mIsUdf( cstflds_[idx]->value() ) )
	{
	    errmsg_ += "Please provide a value for constant '";
	    errmsg_ += cstflds_[idx]->cstnm_; errmsg_ += "'\n";
	    return false;
	}
    }
    return true;
}
