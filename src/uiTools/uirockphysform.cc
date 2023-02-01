/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
{ mODTextTranslationClass(uiRockPhysConstantFld)
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


void update( const RockPhysics::Formula& form, int iinp )
{
    const bool validcst = iinp >= 0 && iinp < form.nrInputs();
    display( validcst );
    if ( !validcst )
	{ cstnm_.setEmpty(); infofld_->setInfo( "" ); return; }

    cstnm_ = form.inputConstantName( iinp );

    nmlbl_->setText( tr("Value for '%1'" ).arg( cstnm_) );
    infofld_->setInfo( form.inputDescription( iinp ),
		       od_static_tr("update","Information on '%1'").
						      arg(toUiString(cstnm_)) );
    valfld_->setValue( form.getConstVal(iinp) );

    const Interval<float> typicalrg = form.inputTypicalRange( iinp );
    const bool haverg = !typicalrg.isUdf();
    if ( haverg )
    {
	const uiString rgstr = tr("Typical range: [%1,%2]").
			 arg(typicalrg.start).arg(typicalrg.stop );
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




uiRockPhysForm::uiRockPhysForm( uiParent* p, Mnemonic::StdType deftyp )
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
		       Mnemonic::StdTypeDef().getKeyForIndex( deftyp ) );

    auto* lcb = new uiLabeledComboBox( this, typnms, tr("Property Type") );
    typfld_ = lcb->box();
    typfld_->setHSzPol( uiObject::MedMax );
    if ( typnms.validIdx(defidx) )
	typfld_->setCurrentItem( defidx );
    mAttachCB( typfld_->selectionChanged, uiRockPhysForm::typSel );

    createMnemonicFld( mnsels, lcb->attachObj() );

    createFlds( mnselflds_.isEmpty()
			? (uiObject*)lcb->attachObj()
			: (uiObject*)mnselflds_.first()->attachObj() );
}


uiRockPhysForm::uiRockPhysForm( uiParent* p, const Mnemonic& mn )
    : uiGroup(p,"RockPhyics Formula Selector")
{
    if ( mn.stdType() == Mnemonic::Volum )
	createVolumeFlds( mn );
    else
	fixedmn_ = &mn;

    createFlds( mnselflds_.isEmpty() ? nullptr
	    : (uiObject*)mnselflds_.first()->attachObj() );
}


uiRockPhysForm::~uiRockPhysForm()
{
    detachAllNotifiers();
}


void uiRockPhysForm::createVolumeFlds( const Mnemonic& mn )
{
    MnemonicSelection mnsel = MnemonicSelection::getGroupFor( mn );
    ObjectSet<const Math::Formula> mathforms;
    const RockPhysics::FormulaSet& rphys = ROCKPHYSFORMS();
    for ( int idx=mnsel.size()-1; idx>=0; idx--, mathforms.setEmpty() )
    {
	const Mnemonic* tmn = mnsel.get( idx );
	if ( !rphys.getRelevant(*tmn,mathforms) )
	    mnsel.removeSingle( idx );
    }

    ObjectSet<MnemonicSelection> mnsels;
    mnsels.add( &mnsel );
    createMnemonicFld( mnsels, nullptr );
}


void uiRockPhysForm::createMnemonicFld(
				const ObjectSet<MnemonicSelection>& mnsels,
				uiObject* attachobj )
{
    for ( const auto* mnsel : mnsels )
    {
	uiMnemonicsSel::Setup umnselsu( mnsel );
	auto* mnselfld = new uiMnemonicsSel( this, umnselsu );
	if ( attachobj )
	    mnselfld->attach( alignedBelow, attachobj );

	mnselflds_.add( mnselfld );
	uiComboBox* mnselcbfld = mnselfld->box();
	mAttachCB( mnselcbfld->selectionChanged, uiRockPhysForm::mnSel );
    }
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
    mAttachCB( postFinalize(), uiRockPhysForm::initGrp );
}


void uiRockPhysForm::initGrp( CallBacker* )
{
    if ( typfld_ )
	typSel( nullptr );
    else if ( !mnselflds_.isEmpty() &&
	      !mnselflds_.first()->getSelection().isEmpty() )
    {
	setType( *mnselflds_.first()->getSelection().first() );
    }
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
    ObjectSet<const Math::Formula> forms;
    ROCKPHYSFORMS().getRelevant( mn, forms );
    BufferStringSet nms;
    for ( const auto* fm : forms )
	nms.add( fm->name() );
    nmfld_->setEmpty();
    nmfld_->addItems( nms );
    if ( !nms.isEmpty() )
	nmfld_->setCurrentItem( 0 );

    nameSel( nullptr );
}


const Mnemonic& uiRockPhysForm::getMnemonic() const
{
    if ( fixedmn_ )
	return *fixedmn_;

    for ( const auto* mnfld : mnselflds_ )
    {
	if ( !mnfld->isDisplayed() )
	    continue;

	const Mnemonic* mn = mnfld->mnemonic();
	if ( mn )
	    return *mn;
	break;
    }

    return Mnemonic::undef();
}


void uiRockPhysForm::nameSel( CallBacker* )
{
    const StringView txt = nmfld_->text();
    if ( txt.isEmpty() )
	return;

    const RockPhysics::Formula* fm =
				ROCKPHYSFORMS().getByName( getMnemonic(), txt );
    if ( !fm )
	{ uiMSG().error( tr("Internal: formula not found") ); return;}

    const RockPhysics::Formula& rpfm = *fm;
    int icst = 0;
    for ( int iinp=0; iinp<rpfm.nrInputs(); iinp++ )
    {
	if ( !rpfm.isConst(iinp) )
	    continue;

	uiRockPhysConstantFld* cstflds = cstflds_[icst++];
	cstflds->update( rpfm, iinp );
    }
    for ( int idx=icst; idx<cstflds_.size(); idx++ )
	cstflds_[idx]->update( rpfm, -1 );

    formulafld_->setText( getFormText(rpfm,true) );
    descriptionfld_->setHtmlText( rpfm.description() );
}


BufferString uiRockPhysForm::getFormText( const RockPhysics::Formula& rpfm,
					  bool fortxtdisp ) const
{
    BufferString formstr( rpfm.text() );
    int iconstdef = 0;
    for ( int iinp=0; iinp<rpfm.nrInputs(); iinp++ )
    {
	BufferString formval;
	if ( rpfm.isConst(iinp) )
	{
	    if ( fortxtdisp )
		formval.set( rpfm.inputConstantName( iinp ) );
	    else
		formval.set( cstflds_[iconstdef++]->value() );
	}
	else
	{
	    formval.set( rpfm.inputDef( iinp ) );
	    formval.clean();
	}
	formstr.replace( rpfm.variableName(iinp), formval );
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
    const StringView txt = nmfld_->text();
    if ( txt.isEmpty() )
	{ uiMSG().error( tr("No formula name selected") ); return false; }

    const RockPhysics::Formula* fm =
				ROCKPHYSFORMS().getByName( getMnemonic(), txt );
    if ( !fm )
    {
	uiMSG().error( tr("Internal: formula not found") );
	return false;
    }

    form = *fm;
    return true;
}


uiRetVal uiRockPhysForm::isOK() const
{
    uiRetVal ret;

    if ( finalized() )
    {
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
    }

    bool hasformula = false;
    if ( fixedmn_ )
    {
	ObjectSet<const Math::Formula> forms;
	hasformula = ROCKPHYSFORMS().getRelevant( *fixedmn_, forms );
    }
    else
    {
	int nrmn = 0;
	for ( const auto* mnselfld : mnselflds_ )
	    nrmn += mnselfld->getSelection().size();
	hasformula = nrmn > 0;
    }

    if ( !hasformula )
	ret.add( tr("No formula available for this property") );

    return ret;
}
