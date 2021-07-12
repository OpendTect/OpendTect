/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2011
________________________________________________________________________

-*/

#include "uimanprops.h"

#include "mathexpression.h"
#include "mathproperty.h"
#include "mnemonics.h"
#include "separstr.h"
#include "uibuildlistfromlist.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uilistbox.h"
#include "uimathexpression.h"
#include "uimathpropeddlg.h"
#include "uimsg.h"
#include "uirockphysform.h"
#include "uistrings.h"
#include "uitoolbutton.h"
#include "uiunitsel.h"
#include "unitofmeasure.h"
#include "od_helpids.h"


class uiBuildPROPS : public uiBuildListFromList
{ mODTextTranslationClass(uiBuildPROPS);
public:
			uiBuildPROPS(uiParent*,PropertySet&,bool);

    PropertySet&	props_;
    bool		allowmath_;

    virtual const char*	avFromDef(const char*) const;
    virtual void	editReq(bool);
    virtual void	removeReq();
    virtual void	itemSwitch(const char*,const char*);
    void		initGrp(CallBacker*);
    bool		isPropRemovable(int propidx);

};


uiBuildPROPS::uiBuildPROPS( uiParent* p, PropertySet& prs, bool allowmath )
    : uiBuildListFromList(p,
	    uiBuildListFromList::Setup(false,"property type","usable property")
	    .withio(false).withtitles(true), "Property selection group")
    , props_(prs)
    , allowmath_(allowmath)
{
    const BufferStringSet dispnms( PropertyRef::StdTypeNames() );

    setAvailable( dispnms );

    BufferStringSet pnms;
    for ( int idx=0; idx<props_.size(); idx++ )
	pnms.add( props_.get(idx).name() );

    pnms.sort();
    for ( int idx=0; idx<pnms.size(); idx++ )
	addItem( pnms.get(idx) );

    postFinalise().notify( mCB(this,uiBuildPROPS,initGrp) );
}

void uiBuildPROPS::initGrp( CallBacker* )
{
    if ( !props_.isEmpty() )
	setCurDefSel( props_.get(0).name() );
}

const char* uiBuildPROPS::avFromDef( const char* nm ) const
{
    const Property* pr = props_.find( nm );
    if ( !pr )
	return 0;

    return PropertyRef::toString( pr->mnem().stdType() );
}


class uiEditProp : public uiDialog
{ mODTextTranslationClass(uiEditProp);
public:

			uiEditProp(uiParent*,Property&,bool);
			uiEditProp(uiParent*,const char* nm,
				   PropertyRef::StdType,bool);
    bool		acceptOK(CallBacker*);

    Property&		pr_;
    Mnemonic*		mn_;
    const bool		withform_;

    uiGenInput*		namefld_;
    uiGenInput*		mnemonicsfld_;
    uiGenInput*		aliasfld_;
    uiColorInput*	colfld_;
    uiGenInput*		rgfld_;
    uiUnitSel*		unfld_;
    uiGenInput*		defaultfld_;
    uiGenInput*		definitionfld_;
    uiPushButton*	defaultformbut_;
    uiPushButton*	definitionformbut_;

    const UnitOfMeasure* curunit_;
    MathProperty	definitionmathprop_;
    MathProperty	defaultmathprop_;

    void		setForm(bool);

    void		setDefinitionForm(CallBacker*)	{ setForm(true); }
    void		setDefaultForm(CallBacker*)	{ setForm(false); }
    void		definitionChecked(CallBacker*);
    void		mnemonicSelCB(CallBacker*);
    void		unitSel(CallBacker*);

protected:
    void		fillDlg(bool fromavilprop=false);
};


uiEditProp::uiEditProp( uiParent* p, const char* nm,
			PropertyRef::StdType stdtype, bool supportform )
    : uiDialog(p,uiDialog::Setup(tr("Property definition"),
			    toUiString("%1 '%2' property")
			    .arg(uiStrings::sAdd())
			    .arg(PropertyRef::toString(stdtype)),
			    mODHelpKey(mEditPropRefHelpID) ))
    , pr_(*new ValueProperty(nm,*MNC().getGuessed(stdtype)))
    , defaultmathprop_(*new MathProperty(pr_.name(),pr_.mnem()))
    , definitionmathprop_(*new MathProperty(pr_.name(),pr_.mnem()))
    , withform_(supportform)
    , curunit_(nullptr)
{
    fillDlg( true );
}


uiEditProp::uiEditProp( uiParent* p, Property& pr, bool supportform )
    : uiDialog(p,uiDialog::Setup(tr("Property definition"),
			     toUiString("%1 '%2' property")
			     .arg(uiStrings::sEdit())
			     .arg( PropertyRef::toString(pr.mnem().stdType())),
			     mODHelpKey(mEditPropRefHelpID) ))
    , pr_(pr)
    , defaultmathprop_(*new MathProperty(pr.name(),pr.mnem()))
    , definitionmathprop_(*new MathProperty(pr.name(),pr.mnem()))
    , withform_(supportform)
    , curunit_(nullptr)
{
    fillDlg();
}


void uiEditProp::fillDlg( bool fromavailprop )
{
    namefld_ = new uiGenInput( this, uiStrings::sName(),
			       StringInpSpec(pr_.name()) );

    MnemonicSet& mns = eMNC();
    BufferStringSet mnnames;
    if ( !fromavailprop )
	mnnames.add( pr_.mnem().name() );
    else
    {
	MnemonicSet* mnsforpr = mns.getSet( &pr_.ref() );
	mnsforpr->getNames( mnnames );
	if ( mnnames.isEmpty() )
	    mns.getNames( mnnames );
    }

    mnemonicsfld_ = new uiGenInput( this, tr("Mnemonic"),
	   			StringListInpSpec(mnnames) );
    mnemonicsfld_->attach( alignedBelow, namefld_ );
    mAttachCB( mnemonicsfld_->valuechanged, uiEditProp::mnemonicSelCB );
    mn_ = mns.find( mnemonicsfld_->text() );

    SeparString ss;
    if ( mn_ )
    {
	for ( int idx=0; idx<mn_->aliases().size(); idx++ )
	    ss += mn_->aliases().get(idx);
    }

    aliasfld_ = new uiGenInput( this, tr("Aliases (e.g. 'abc, uvw*xyz')"),
				StringInpSpec(ss.buf()) );
    aliasfld_->setElemSzPol( uiObject::Wide );
    aliasfld_->attach( alignedBelow, mnemonicsfld_ );

    colfld_ = new uiColorInput( this,
				uiColorInput::Setup(pr_.disp_.color_)
				.lbltxt(tr("Default display color")) );
    colfld_->attach( alignedBelow, aliasfld_ );

    rgfld_ = new uiGenInput( this, tr("Typical value range"),
			     FloatInpIntervalSpec() );
    rgfld_->attach( alignedBelow, colfld_ );

    unfld_ = new uiUnitSel( this, pr_.mnem().stdType() );
    unfld_->setUnit( pr_.disp_.unit_ );

    unfld_->inpFld()->setHSzPol( uiObject::MedVar );
    unfld_->attach( rightOf, rgfld_ );
    unfld_->selChange.notify( mCB(this,uiEditProp,unitSel) );
    curunit_ = unfld_->getUnit();

    Interval<float> udfintv;
    udfintv.setUdf();
    Interval<float> vintv( &pr_ ? pr_.disp_.typicalrange_ : udfintv );
    if ( curunit_ )
    {
	if ( !mIsUdf(vintv.start) )
	    vintv.start = curunit_->getUserValueFromSI( vintv.start );
	if ( !mIsUdf(vintv.stop) )
	    vintv.stop = curunit_->getUserValueFromSI( vintv.stop );
    }
    rgfld_->setValue( vintv );

    defaultfld_ = new uiGenInput( this, tr("Default value") );
    defaultfld_->attach( alignedBelow, rgfld_ );
    if ( !pr_.defval_ || pr_.defval_->isValue() )
    {
	float val = pr_.defval_ ? pr_.defval_->value()
				      : pr_.commonValue();
	if ( curunit_ )
	    val = curunit_->getUserValueFromSI( val );

	if ( val )
	    defaultfld_->setValue( val );
    }
    else
    {
	defaultmathprop_.setDef( pr_.defval_->def() );
	defaultfld_->setText( defaultmathprop_.formText(true) );
    }
    defaultformbut_ = new uiPushButton( this, tr("Formula"),
				mCB(this,uiEditProp,setDefaultForm), false );
    defaultformbut_->attach( rightOf, defaultfld_ );

    definitionfld_ = new uiGenInput( this, tr("Fixed definition") );
    definitionfld_->attach( alignedBelow, defaultfld_ );
    definitionfld_->setWithCheck( true );
    definitionfld_->setChecked( pr_.hasFixedDef() );
    if ( pr_.hasFixedDef() )
    {
	definitionmathprop_.setDef( pr_.fixedDef().def() );
	definitionfld_->setText( definitionmathprop_.formText(true) );
    }
    definitionformbut_ = new uiPushButton( this, tr("Formula"),
			    mCB(this,uiEditProp,setDefinitionForm), false );
    definitionformbut_->attach( rightOf, definitionfld_ );

    const CallBack defchckcb( mCB(this,uiEditProp,definitionChecked) );
    definitionfld_->checked.notify( defchckcb );
    postFinalise().notify( defchckcb );
}


void uiEditProp::unitSel( CallBacker* )
{
    const UnitOfMeasure* newun = unfld_->getUnit();
    if ( newun == curunit_ )
	return;

    Interval<double> vintv( rgfld_->getDInterval() );
    convValue( vintv.start, curunit_, newun );
    convValue( vintv.stop, curunit_, newun );
    rgfld_->setValue( vintv );
    if ( !pr_.defval_ || pr_.defval_->isValue() )
    {
	float val = pr_.defval_ ? pr_.defval_->value()
				      : pr_.commonValue();
	val = newun->getUserValueFromSI( val );
	defaultfld_->setValue( val );
    }

    curunit_ = newun;
}


void uiEditProp::mnemonicSelCB( CallBacker* )
{
    mn_ = eMNC().find( mnemonicsfld_->text() );
    if ( mn_ )
    {
	SeparString ss;
	for ( int idx=0; idx<mn_->aliases().size(); idx++ )
            ss += mn_->aliases().get(idx);

	aliasfld_->setText( ss );
	pr_.setMnemonic( *mn_ );
	colfld_->setColor( pr_.disp_.color_ );
	unfld_->setMnemonic( *mn_ );
	curunit_ = unfld_->getUnit();
	Interval<float> vintv( pr_.disp_.typicalrange_ );
	if ( curunit_ )
	{
	    if ( !mIsUdf(vintv.start) )
		vintv.start = curunit_->getUserValueFromSI( vintv.start );
	    if ( !mIsUdf(vintv.stop) )
		vintv.stop = curunit_->getUserValueFromSI( vintv.stop );
	}

	rgfld_->setValue( vintv );
    }
}


void uiEditProp::setForm( bool definition )
{
    PropertySelection prsel = PropertySelection::getAll( true, &pr_ );
    MathProperty& mped = definition ? definitionmathprop_ : defaultmathprop_;
    uiMathPropEdDlg dlg( parent(), mped, prsel );
    if ( !dlg.go() )
	return;

    (definition?definitionfld_:defaultfld_)->setText( mped.formText(true) );
}


void uiEditProp::definitionChecked( CallBacker* )
{
    definitionformbut_->setSensitive( definitionfld_->isChecked() );
    defaultformbut_->display( !definitionfld_->isChecked() );
}


#define mErrRet(s,retype) { uiMSG().error(s); return retype; }

bool uiEditProp::acceptOK( CallBacker* )
{
    const BufferString newnm( namefld_->text() );
    if ( newnm.isEmpty() || !iswalpha(newnm[0]) || newnm.size() < 2 )
	mErrRet(uiStrings::sEnterValidName(),false);
    const BufferString definitionstr( definitionfld_->text() );
    const bool isfund = definitionfld_->isChecked();
    if ( isfund && definitionstr.isEmpty() )
	mErrRet( tr("Please un-check the 'Fixed Definition' - or provide one"),
		 false );

    pr_.setName( newnm );
    SeparString ss( aliasfld_->text() ); const int nral = ss.size();
    if ( mn_ )
    {
	pr_.setMnemonic( *mn_ );
	mn_->aliases().erase();
	for ( int idx=0; idx<nral; idx++ )
	{
	    if ( mn_ )
		mn_->aliases().add( ss[idx] );
	}

	pr_.disp_.color_ = colfld_->color();
	Interval<float> vintv( rgfld_->getFInterval() );
	if ( !curunit_ )
	    pr_.disp_.unit_.setEmpty();
	else
	{
	    pr_.disp_.unit_ = curunit_->name();
	    if ( !mIsUdf(vintv.start) )
		vintv.start = curunit_->getSIValue( vintv.start );

	    if ( !mIsUdf(vintv.stop) )
		vintv.stop = curunit_->getSIValue( vintv.stop );
	}

	pr_.disp_.typicalrange_ = vintv;
    }

    BufferString defaultstr( defaultfld_->text() );
    defaultstr.trimBlanks();
    if ( defaultstr.isEmpty() )
    {
	delete pr_.defval_;
	pr_.defval_ = 0;
    }
    else
    {
	if ( !withform_ || defaultstr.isNumber() )
	{
	    float val = defaultstr.toFloat();
	    if ( curunit_ )
		val = curunit_->getSIValue( val );
	    pr_.defval_ = new ValueProperty( pr_.name(), pr_.mnem() );
	}
	else
	    pr_.defval_ = new MathProperty( defaultmathprop_ );
    }

    if ( !isfund )
	pr_.setFixedDef( nullptr );
    else
	pr_.setFixedDef( definitionmathprop_.clone() );

    return true;
}


void uiBuildPROPS::editReq( bool isadd )
{
    const char* nm = isadd ? curAvSel() : curDefSel();
    if ( !nm || !*nm )
	return;

    Property* pr = nullptr;
    PropertyRef::StdType stdtype = PropertyRef::Other;
    if ( isadd )
	stdtype = PropertyRef::StdTypeDef().parse( nm );
    else
	pr = props_.find( nm );

    if ( !isadd && !pr )
	return;

    PtrMan<uiEditProp> dlg = pr ? new uiEditProp( this, *pr, allowmath_ )
				: new uiEditProp( this, nm ,
						  stdtype, allowmath_ );
    if ( !dlg->go() )
	return;

    if ( isadd )
    {
	pr = &dlg->pr_;
	if ( props_.isPresent(pr->name()) )
	    mErrRet( tr("Property with same name '%1' already "
			" present.").arg(pr->name()),  )
	props_.add( pr );
    }

    handleSuccessfullEdit( isadd, pr->name() );
}


bool uiBuildPROPS::isPropRemovable( int propidx )
{
    const Property* prop = &props_.get( propidx );
    if ( prop->isThickness() )
	mErrRet( tr("Cannot remove inbuilt property 'Thickness'."), false )
    if ( prop->mnem().stdType()==PropertyRef::Den ||
	 prop->mnem().stdType()==PropertyRef::Vel )
    {
	ObjectSet<const Property> subselprs;
	props_.getPropertiesOfRefType( prop->mnem().stdType(), subselprs );
	if ( subselprs.size()<=1 )
	    mErrRet( tr( "Cannot remove this property.Need to have atleast one "
			 "usable property of type '%1'")
			.arg(PropertyRef::toString(
					    prop->mnem().stdType())), false )
    }

    return true;
}


void uiBuildPROPS::removeReq()
{
    const char* prnm = curDefSel();
    if ( prnm && *prnm )
    {
	const int idx = props_.indexOf( prnm );
	if ( idx < 0 ) return;
	if ( isPropRemovable(idx) )
	{
	    props_.remove( idx );
	    removeItem();
	}
    }
}


void uiBuildPROPS::itemSwitch( const char* nm1, const char* nm2 )
{
    const int idx1 = props_.indexOf( nm1 );
    const int idx2 = props_.indexOf( nm2 );
    if ( idx1 < 0 || idx2 < 0 )
    {
	pErrMsg("Huh");
	return;
    }

    props_.swap( idx1, idx2 );
}


uiManPROPS::uiManPROPS( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Manage Layer Properties"),
				 tr("Define possible layer properties"),
				 mODHelpKey(mManPROPSHelpID)))
{
    setCtrlStyle( CloseOnly );
    eMNC();
    buildfld_ = new uiBuildPROPS( this, ePROPS(), true );
    const char* strs[] = { "For this survey only",
			   "As default for all surveys",
			   "As default for my user ID only", nullptr };
    srcfld_ = new uiGenInput( this, tr("Store"), StringListInpSpec(strs) );
    srcfld_->attach( centeredBelow, buildfld_ );
}


bool uiManPROPS::rejectOK( CallBacker* )
{
    if ( !haveUserChange() )
	return true;

    ePROPS() = buildfld_->props_;
    const int isrc = srcfld_->getIntValue();
    const Repos::Source repsrc =   isrc == 0	? Repos::Survey
				: (isrc == 1	? Repos::Data
						: Repos::User);

    if ( !PROPS().save(repsrc) )
	uiMSG().warning( tr("Cannot store the definitions to file."
			 "\nPlease check file/directory permissions.") );
    else if ( repsrc != Repos::Survey )
    {
	if ( !PROPS().save(Repos::Survey) )
	    uiMSG().warning(tr("Could not store the definitions in your survey."
			 "\nPlease check file/directory permissions there.") );
    }

    return true;
}


bool uiManPROPS::haveUserChange() const
{
    return buildfld_->haveUserChange();
}


uiSelectProps::uiSelectProps( uiParent* p,PropertySelection& prs,
				    const char* lbl )
    : uiDialog(p,uiDialog::Setup(toUiString("%1 %2 - %3")
		.arg(uiStrings::sLayer()).arg(uiStrings::sProperties())
		.arg(uiStrings::sSelection()),
		uiStrings::phrSelect(tr("layer properties to use")),
		mODHelpKey(mSelectPropRefsHelpID) ))
{
    proprefgrp_ = new uiSelectPropsGrp( this, prs, lbl );
}


bool uiSelectProps::acceptOK( CallBacker* )
{
    return proprefgrp_->acceptOK();
}


uiSelectPropsVWDlg::uiSelectPropsVWDlg(
	uiParent* p, PropertySelection& prs, IOPar& pars, int pos,
	const char* lbl )
    : uiVarWizardDlg(p,uiDialog::Setup(toUiString("%1 %2 - %3")
	    .arg(uiStrings::sLayer()).arg(uiStrings::sProperties())
	    .arg(uiStrings::sSelection()),
	    tr("You will be modeling layer properties, we have pre-selected"
		" essential ones.\nAdd properties only if you are interested in"
		" modeling those."),
	    mODHelpKey(mSelectPropRefsHelpID)),
	    pars, (uiVarWizardDlg::Position)pos )
{
    proprefgrp_ = new uiSelectPropsGrp( this, prs, lbl );
}


bool uiSelectPropsVWDlg::acceptOK( CallBacker* )
{
    return proprefgrp_->acceptOK();
}



uiSelectPropsGrp::uiSelectPropsGrp( uiParent* p,PropertySelection& prs,
					  const char* lbl )
    : uiDlgGroup(p,tr("Layer Properties"))
    , props_(PROPS())
    , prsel_(prs)
    , thref_(&Property::thickness())
    , structchg_(false)
{
    uiListBox::Setup su( OD::ChooseAtLeastOne, mToUiStringTodo(lbl),
			 uiListBox::AboveMid );
    propfld_ = new uiListBox( this, su, "Available properties" );
    fillList();

    uiToolButton* manpropsbut = new uiToolButton( this, "man_props",
					tr("Manage available properties"),
					mCB(this,uiSelectPropsGrp,manPROPS));
    manpropsbut->attach( centeredRightOf, propfld_ );
}


void uiSelectPropsGrp::fillList()
{
    PropertySelection prs( PropertySelection::getAll(false) );
    BufferStringSet dispnms; addNames( prs, dispnms );
    if ( dispnms.isEmpty() )
	return;

    dispnms.sort();
    propfld_->addItems( dispnms );

    int firstsel = -1;
    for ( int idx=0; idx<dispnms.size(); idx++ )
    {
	const char* nm = dispnms.get( idx ).buf();
	const bool issel = prsel_.isPresent( nm );
	propfld_->setChosen( idx, issel );
	if ( issel && firstsel < 0 ) firstsel = idx;
    }

    propfld_->setCurrentItem( firstsel < 0 ? 0 : firstsel );
}


void uiSelectPropsGrp::manPROPS( CallBacker* )
{
    BufferStringSet orgnms;
    for ( int idx=0; idx<prsel_.size(); idx++ )
	orgnms.add( prsel_[idx]->name() );

    uiManPROPS dlg( this );
    if ( !dlg.go() )
	return;

    structchg_ = structchg_ || dlg.haveUserChange();

    // Even if user will cancel we cannot leave removed PROP's in the set
    for ( int idx=0; idx<orgnms.size(); idx++ )
    {
	if ( !props_.isPresent(prsel_[idx]) )
	    { structchg_ = true; prsel_.removeSingle( idx ); idx--; }
    }

    propfld_->setEmpty();
    fillList();
}


bool uiSelectPropsGrp::acceptOK()
{
    prsel_.erase();
    prsel_.insertAt( &Property::thickness(), 0 );

    for ( int idx=0; idx<propfld_->size(); idx++ )
    {
	if ( !propfld_->isChosen(idx) )
	    continue;

	const char* pnm = propfld_->textOfItem( idx );
	const Property* pr = props_.find( pnm );
	if ( !pr ) { pErrMsg("Huh"); structchg_ = true; continue; }
	prsel_ += pr;
    }

    return true;
}
