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
			uiBuildPROPS(uiParent*,PropertyRefSet&,bool);

    PropertyRefSet&	props_;
    bool		allowmath_;

    virtual const char*	avFromDef(const char*) const;
    virtual void	editReq(bool);
    virtual void	removeReq();
    virtual void	itemSwitch(const char*,const char*);
    void		initGrp(CallBacker*);
    bool		isPropRemovable(int propidx);

};


uiBuildPROPS::uiBuildPROPS( uiParent* p, PropertyRefSet& prs, bool allowmath )
    : uiBuildListFromList(p,
	    uiBuildListFromList::Setup(false,"property type","usable property")
	    .withio(false).withtitles(true), "PropertyRef selection group")
    , props_(prs)
    , allowmath_(allowmath)
{
    const BufferStringSet dispnms( PropertyRef::StdTypeNames() );

    setAvailable( dispnms );

    BufferStringSet pnms;
    for ( int idx=0; idx<props_.size(); idx++ )
	pnms.add( props_[idx]->name() );
    pnms.sort();
    for ( int idx=0; idx<pnms.size(); idx++ )
	addItem( pnms.get(idx) );

    postFinalise().notify( mCB(this,uiBuildPROPS,initGrp) );
}

void uiBuildPROPS::initGrp( CallBacker* )
{
    if ( !props_.isEmpty() )
	setCurDefSel( props_[0]->name() );
}

const char* uiBuildPROPS::avFromDef( const char* nm ) const
{
    const PropertyRef* pr = props_.find( nm );
    if ( !pr ) return 0;
    return PropertyRef::toString( pr->stdType() );
}


class uiEditPropRef : public uiDialog
{ mODTextTranslationClass(uiEditPropRef);
public:

			uiEditPropRef(uiParent*,PropertyRef&,bool,bool);
    bool		acceptOK(CallBacker*);

    PropertyRef&	pr_;
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

};


uiEditPropRef::uiEditPropRef( uiParent* p, PropertyRef& pr, bool isadd,
			      bool supportform )
    : uiDialog(p,uiDialog::Setup(tr("Property definition"),
				 toUiString("%1 '%2' property").arg(isadd ?
				 uiStrings::sAdd():uiStrings::sEdit()).
				 arg( PropertyRef::toString(pr.stdType())),
				 mODHelpKey(mEditPropRefHelpID) ))
    , pr_(pr)
    , defaultmathprop_(pr)
    , definitionmathprop_(pr)
    , withform_(supportform)
    , curunit_(0)
{
    namefld_ = new uiGenInput( this, uiStrings::sName(),
			       StringInpSpec(pr.name()) );

    MnemonicSet& mns = eMNC();
    BufferStringSet mnnames;
    if ( pr_.getMnemonic().isEmpty() )
    {
        MnemonicSet* mnsforpr = mns.getSet( &pr_ );
        mnsforpr->getNames( mnnames );
	if ( mnnames.isEmpty() )
	    mns.getNames( mnnames );
    }
    else
        mnnames.add( pr_.getMnemonic() );

    mnemonicsfld_ = new uiGenInput( this, tr("Mnemonic"),
	   			StringListInpSpec(mnnames) );
    mnemonicsfld_->attach( alignedBelow, namefld_ );
    mAttachCB( mnemonicsfld_->valuechanged, uiEditPropRef::mnemonicSelCB );
    mn_ = mns.find( mnemonicsfld_->text() );

    SeparString ss;
    if ( mn_ )
    {
	for ( int idx=0; idx<mn_->aliases().size(); idx++ )
	    ss += mn_->aliases().get(idx);
    }
    else
    {
	for ( int idx=0; idx<pr_.aliases().size(); idx++ )
	    ss += pr_.aliases().get(idx);
    }

    aliasfld_ = new uiGenInput( this, tr("Aliases (e.g. 'abc, uvw*xyz')"),
				StringInpSpec(ss.buf()) );
    aliasfld_->setElemSzPol( uiObject::Wide );
    aliasfld_->attach( alignedBelow, mnemonicsfld_ );

    if ( !mn_ )
    {
	colfld_ = new uiColorInput( this, uiColorInput::Setup(Color::White())
			    .lbltxt(tr("Default display color")) );
	colfld_->attach( alignedBelow, aliasfld_ );
    }
    else
    {
	colfld_ = new uiColorInput( this,
				    uiColorInput::Setup(mn_->disp_.color_)
				    .lbltxt(tr("Default display color")) );
	colfld_->attach( alignedBelow, aliasfld_ );
    }

    rgfld_ = new uiGenInput( this, tr("Typical value range"),
			     FloatInpIntervalSpec() );
    rgfld_->attach( alignedBelow, colfld_ );

    if ( !mn_ )
	unfld_ = new uiUnitSel( this, pr_.stdType() );
    else
    {
	unfld_ = new uiUnitSel( this, mn_->stdType() );
        unfld_->setUnit( mn_->disp_.unit_ );
    }

    unfld_->inpFld()->setHSzPol( uiObject::MedVar );
    unfld_->attach( rightOf, rgfld_ );
    unfld_->selChange.notify( mCB(this,uiEditPropRef,unitSel) );
    curunit_ = unfld_->getUnit();

    Interval<float> udfintv;
    udfintv.setUdf();
    Interval<float> vintv( mn_ ? mn_->disp_.typicalrange_ : udfintv );
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
				mCB(this,uiEditPropRef,setDefaultForm), false );
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
			    mCB(this,uiEditPropRef,setDefinitionForm), false );
    definitionformbut_->attach( rightOf, definitionfld_ );

    const CallBack defchckcb( mCB(this,uiEditPropRef,definitionChecked) );
    definitionfld_->checked.notify( defchckcb );
    postFinalise().notify( defchckcb );
}


void uiEditPropRef::unitSel( CallBacker* )
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


void uiEditPropRef::mnemonicSelCB( CallBacker* )
{
    mn_ = eMNC().find( mnemonicsfld_->text() );
    if ( mn_ )
    {
	SeparString ss;
	for ( int idx=0; idx<mn_->aliases().size(); idx++ )
            ss += mn_->aliases().get(idx);

	aliasfld_->setText( ss );
	colfld_->setColor( mn_->disp_.color_ );
	unfld_->setMnemonic( *mn_ );
	curunit_ = unfld_->getUnit();
	Interval<float> vintv( mn_->disp_.typicalrange_ );
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


void uiEditPropRef::setForm( bool definition )
{
    PropertyRefSelection prsel = PropertyRefSelection::getAll( true, &pr_ );
    MathProperty& mped = definition ? definitionmathprop_ : defaultmathprop_;
    uiMathPropEdDlg dlg( parent(), mped, prsel );
    if ( !dlg.go() )
	return;

    (definition?definitionfld_:defaultfld_)->setText( mped.formText(true) );
}


void uiEditPropRef::definitionChecked( CallBacker* )
{
    definitionformbut_->setSensitive( definitionfld_->isChecked() );
    defaultformbut_->display( !definitionfld_->isChecked() );
}


#define mErrRet(s,retype) { uiMSG().error(s); return retype; }

bool uiEditPropRef::acceptOK( CallBacker* )
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
	pr_.setMnemonic( mn_->name() );
	mn_->aliases().erase();
	for ( int idx=0; idx<nral; idx++ )
	{
	    if ( mn_ )
		mn_->aliases().add( ss[idx] );
	}
	mn_->disp_.color_ = colfld_->color();
	Interval<float> vintv( rgfld_->getFInterval() );
	if ( !curunit_ )
	{
	    if ( mn_ )
		mn_->disp_.unit_.setEmpty();
	}
	else
	{
	    if ( mn_ )
		mn_->disp_.unit_ = curunit_->name();

	    if ( !mIsUdf(vintv.start) )
		vintv.start = curunit_->getSIValue( vintv.start );

	    if ( !mIsUdf(vintv.stop) )
		vintv.stop = curunit_->getSIValue( vintv.stop );
	}

	mn_->disp_.typicalrange_ = vintv;
    }
    else
    {
	pr_.aliases().erase();
	for ( int idx=0; idx<nral; idx++ )
	    pr_.aliases().add( ss[idx] );
    }

    BufferString defaultstr( defaultfld_->text() );
    defaultstr.trimBlanks();
    if ( defaultstr.isEmpty() )
	{ delete pr_.defval_; pr_.defval_ = 0; }
    else
    {
	if ( !withform_ || defaultstr.isNumber() )
	{
	    float val = defaultstr.toFloat();
	    if ( curunit_ )
		val = curunit_->getSIValue( val );
	    pr_.defval_ = new ValueProperty( pr_, val );
	}
	else
	    pr_.defval_ = new MathProperty( defaultmathprop_ );
    }

    if ( !isfund )
	pr_.setFixedDef( 0 );
    else
	pr_.setFixedDef( definitionmathprop_.clone() );

    if ( mn_ )
    {
	MnemonicSet& mnc = eMNC();
	*mnc.find( mn_->name() ) = *mn_;
	MNC().save();
    }

    return true;
}


void uiBuildPROPS::editReq( bool isadd )
{
    const char* nm = isadd ? curAvSel() : curDefSel();
    if ( !nm || !*nm ) return;

    PropertyRef* pr = 0;
    if ( !isadd )
	pr = props_.find( nm );
    else
    {
	PropertyRef::StdType typ = PropertyRef::Other;
	PropertyRef::parseEnumStdType( nm, typ );
	pr = new PropertyRef( nm, typ );
    }
    if ( !pr ) return;

    uiEditPropRef dlg( this, *pr, isadd, allowmath_ );
    if ( !dlg.go() )
    {
	if ( isadd )
	    delete pr;
    }
    else
    {
	if ( isadd )
	{
	    if ( props_.isPresent(pr->name()) )
		mErrRet( tr("Property with same name '%1' already "
			    " present.").arg(pr->name()),  )
	    props_ += pr;
	}
	handleSuccessfullEdit( isadd, pr->name() );
    }
}


bool uiBuildPROPS::isPropRemovable( int propidx )
{
    const PropertyRef* propref = props_[propidx];
    if ( propref->isThickness() )
	mErrRet( tr("Cannot remove inbuilt property 'Thickness'."), false )
    if ( propref->stdType()==PropertyRef::Den ||
	 propref->stdType()==PropertyRef::Vel )
    {
	ObjectSet<const PropertyRef> subselprs;
	props_.subselect( propref->stdType(), subselprs );
	if ( subselprs.size()<=1 )
	    mErrRet( tr( "Cannot remove this property.Need to have atleast one "
			 "usable property of type '%1'")
			.arg(PropertyRef::toString(propref->stdType())), false )
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
	    delete props_.removeSingle( idx );
	    removeItem();
	}
    }
}


void uiBuildPROPS::itemSwitch( const char* nm1, const char* nm2 )
{
    const int idx1 = props_.indexOf( nm1 );
    const int idx2 = props_.indexOf( nm2 );
    if ( idx1 < 0 || idx2 < 0 ) { pErrMsg("Huh"); return; }
    props_.swap( idx1, idx2 );
}


uiManPROPS::uiManPROPS( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Manage Layer Properties"),
				 tr("Define possible layer properties"),
				 mODHelpKey(mManPROPSHelpID)))
{
    setCtrlStyle( CloseOnly );
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


uiSelectPropRefs::uiSelectPropRefs( uiParent* p,PropertyRefSelection& prs,
				    const char* lbl )
    : uiDialog(p,uiDialog::Setup(toUiString("%1 %2 - %3")
		.arg(uiStrings::sLayer()).arg(uiStrings::sProperties())
		.arg(uiStrings::sSelection()),
		uiStrings::phrSelect(tr("layer properties to use")),
		mODHelpKey(mSelectPropRefsHelpID) ))
{
    proprefgrp_ = new uiSelectPropRefsGrp( this, prs, lbl );
}


bool uiSelectPropRefs::acceptOK( CallBacker* )
{ return proprefgrp_->acceptOK(); }


uiSelectPropRefsVWDlg::uiSelectPropRefsVWDlg(
	uiParent* p, PropertyRefSelection& prs, IOPar& pars, int pos,
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
    proprefgrp_ = new uiSelectPropRefsGrp( this, prs, lbl );
}


bool uiSelectPropRefsVWDlg::acceptOK( CallBacker* )
{ return proprefgrp_->acceptOK(); }



uiSelectPropRefsGrp::uiSelectPropRefsGrp( uiParent* p,PropertyRefSelection& prs,
					  const char* lbl )
    : uiDlgGroup(p,tr("Layer Properties"))
    , props_(PROPS())
    , prsel_(prs)
    , thref_(&PropertyRef::thickness())
    , structchg_(false)
{
    uiListBox::Setup su( OD::ChooseAtLeastOne, mToUiStringTodo(lbl),
			 uiListBox::AboveMid );
    propfld_ = new uiListBox( this, su, "Available properties" );
    fillList();

    uiToolButton* manpropsbut = new uiToolButton( this, "man_props",
					tr("Manage available properties"),
					mCB(this,uiSelectPropRefsGrp,manPROPS));
    manpropsbut->attach( centeredRightOf, propfld_ );
}


void uiSelectPropRefsGrp::fillList()
{
    PropertyRefSelection prs( PropertyRefSelection::getAll(false) );
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


void uiSelectPropRefsGrp::manPROPS( CallBacker* )
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


bool uiSelectPropRefsGrp::acceptOK()
{
    prsel_.erase();
    prsel_.insertAt( &PropertyRef::thickness(), 0 );

    for ( int idx=0; idx<propfld_->size(); idx++ )
    {
	if ( !propfld_->isChosen(idx) )
	    continue;

	const char* pnm = propfld_->textOfItem( idx );
	const PropertyRef* pr = props_.find( pnm );
	if ( !pr ) { pErrMsg("Huh"); structchg_ = true; continue; }
	prsel_ += pr;
    }

    return true;
}
