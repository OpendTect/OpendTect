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
			uiBuildPROPS(uiParent*,PropertyRefSet&);
			~uiBuildPROPS();

    PropertyRefSet&	props_;

    void		initGrp(CallBacker*);

    void		editReq(bool) override;
    void		removeReq() override;
    const char*		avFromDef(const char*) const override;
    void		itemSwitch(const char*,const char*) override;
    bool		isPropRemovable(const PropertyRef*);

};


uiBuildPROPS::uiBuildPROPS( uiParent* p, PropertyRefSet& prs )
    : uiBuildListFromList(p,
	    uiBuildListFromList::Setup(false,"mnemonic","property reference")
	    .withio(false).withtitles(true), "PropertyRef selection group")
    , props_(prs)
{
    BufferStringSet mncnames;
    MNC().getNames( mncnames );
    mncnames.sort();
    setAvailable( mncnames );

    BufferStringSet pnms;
    for ( const auto* pr : prs )
	pnms.add( pr->name() );
    pnms.sort();

    for ( const auto* prnm : pnms )
	addItem( prnm->buf() );

    mAttachCB( postFinalize(), uiBuildPROPS::initGrp );
}


uiBuildPROPS::~uiBuildPROPS()
{
    detachAllNotifiers();
}


void uiBuildPROPS::initGrp( CallBacker* )
{
    if ( !props_.isEmpty() )
	setCurDefSel( props_.first()->name() );

    usrchg_ = false;
}


const char* uiBuildPROPS::avFromDef( const char* nm ) const
{
    const PropertyRef* pr = const_cast<const PropertyRefSet&>(props_)
					.getByName( nm, false );
    return pr ? pr->mn().name().str() : nullptr;
}


class uiEditPropRef : public uiDialog
{ mODTextTranslationClass(uiEditPropRef);
public:

			uiEditPropRef(uiParent*,PropertyRef&,bool isadd);
			~uiEditPropRef();

    bool		acceptOK(CallBacker*) override;

    bool		isChanged() const	{ return changed_; }

private:

    PropertyRef&	pr_;
    bool		changed_ = false;

    uiGenInput*		namefld_;
    uiGenInput*		mnemonicsfld_;
    uiGenInput*		mnaliasfld_;
    uiGenInput*		praliasfld_;
    uiColorInput*	colfld_;
    uiGenInput*		rgfld_;
    uiUnitSel*		unfld_;
    uiGenInput*		defaultfld_;
    uiGenInput*		definitionfld_;
    uiPushButton*	defaultformbut_;
    uiPushButton*	definitionformbut_;

    MathProperty	definitionmathprop_;
    MathProperty	defaultmathprop_;

    void		setForm(bool);
    void		setUpdated()			{ changed_ = true; }

    void		initDlg(CallBacker*);
    void		nameChgCB(CallBacker*);
    void		mnemonicSelCB(CallBacker*);
    void		aliasChgCB(CallBacker*);
    void		colorChgCB(CallBacker*);
    void		rangeChgCB(CallBacker*);
    void		unitSel(CallBacker*);
    void		valueChgCB(CallBacker*);
    void		setDefaultForm(CallBacker*)	{ setForm( false ); }
    void		definitionChecked(CallBacker*);
    void		setDefinitionForm(CallBacker*)	{ setForm( true ); }

};


uiEditPropRef::uiEditPropRef( uiParent* p, PropertyRef& pr, bool isadd )
    : uiDialog(p,uiDialog::Setup(tr("Property definition"),
				 toUiString("%1 '%2' property").arg(isadd ?
				 uiStrings::sAdd():uiStrings::sEdit()).
				 arg( Mnemonic::toString(pr.stdType())),
				 mODHelpKey(mEditPropRefHelpID) ))
    , pr_(pr)
    , defaultmathprop_(pr)
    , definitionmathprop_(pr)
{
    namefld_ = new uiGenInput( this, uiStrings::sName(),
			       StringInpSpec(pr.name()) );
    mAttachCB( namefld_->valuechanged, uiEditPropRef::nameChgCB );

    const MnemonicSelection mnsel( pr.stdType() );
    BufferStringSet mnnames;
    for ( const auto* mnc : mnsel )
	mnnames.add( mnc->name() );

    const Mnemonic& mn = pr.mn_;
    mnemonicsfld_ = new uiGenInput( this, tr("Mnemonic"),
				    StringListInpSpec(mnnames) );
    mnemonicsfld_->setText( mn.name() );
    mnemonicsfld_->attach( alignedBelow, namefld_ );
    mAttachCB( mnemonicsfld_->valuechanged, uiEditPropRef::mnemonicSelCB );

    SeparString mnss, prss;
    for ( const auto* mnalias : mn.aliases() )
	mnss += mnalias->buf();
    for ( const auto* pralias : pr.propaliases_ )
	prss += pralias->buf();

    mnaliasfld_ = new uiGenInput( this, tr("Mnemonic aliases"),
				  StringInpSpec(mnss.buf()) );
    mnaliasfld_->setElemSzPol( uiObject::Wide );
    mnaliasfld_->setReadOnly();
    mnaliasfld_->attach( alignedBelow, mnemonicsfld_ );

    praliasfld_ = new uiGenInput( this, tr("Aliases (e.g. 'abc, uvw*xyz')"),
				StringInpSpec(prss.buf()) );
    praliasfld_->setElemSzPol( uiObject::Wide );
    praliasfld_->attach( alignedBelow, mnaliasfld_ );
    mAttachCB( praliasfld_->valuechanged, uiEditPropRef::aliasChgCB );

    colfld_ = new uiColorInput( this, uiColorInput::Setup(pr_.disp_.color_)
					.lbltxt(tr("Default display color")) );
    colfld_->attach( alignedBelow, praliasfld_ );
    mAttachCB( colfld_->colorChanged, uiEditPropRef::colorChgCB );

    rgfld_ = new uiGenInput( this, tr("Typical value range"),
			     FloatInpIntervalSpec(pr.disp_.typicalrange_) );
    rgfld_->attach( alignedBelow, colfld_ );
    mAttachCB( rgfld_->valuechanged, uiEditPropRef::rangeChgCB );

    uiUnitSel::Setup ussu( pr_.stdType() );
    ussu.variableszpol( true );
    unfld_ = new uiUnitSel( this, ussu );
    unfld_->attach( rightOf, rgfld_ );
    unfld_->setUnit( pr_.unit() );
    mAttachCB( unfld_->selChange, uiEditPropRef::unitSel );

    defaultfld_ = new uiGenInput( this, tr("Default value") );
    defaultfld_->attach( alignedBelow, rgfld_ );
    if ( pr_.disp_.defval_ )
    {
	if ( pr_.disp_.defval_->isValue() )
	{
	    const float val = pr_.disp_.defval_->value();
	    defaultfld_->setValue( val );
	}
	else if ( pr.disp_.defval_->isFormula() )
	{
	    defaultmathprop_.setDef( pr_.disp_.defval_->def() );
	    defaultfld_->setText( defaultmathprop_.formText(true) );
	}
    }
    mAttachCB( defaultfld_->valuechanged, uiEditPropRef::valueChgCB );

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

    mAttachCB( postFinalize(), uiEditPropRef::initDlg );
}


uiEditPropRef::~uiEditPropRef()
{
    detachAllNotifiers();
}


void uiEditPropRef::initDlg( CallBacker* )
{
    mAttachCB( definitionfld_->checked, uiEditPropRef::definitionChecked );
}


void uiEditPropRef::nameChgCB( CallBacker* )
{
    setUpdated();
}


void uiEditPropRef::mnemonicSelCB( CallBacker* )
{
    const Mnemonic* mn = MNC().getByName( mnemonicsfld_->text(), false );
    if ( !mn || mn == &pr_.mn_ )
	return;

    SeparString mnss;
    for ( const auto* mnalias : mn->aliases() )
	mnss += mnalias->buf();
    mnaliasfld_->setText( mnss );
    setUpdated();
}


void uiEditPropRef::aliasChgCB( CallBacker* )
{
    setUpdated();
}


void uiEditPropRef::colorChgCB( CallBacker* )
{
    setUpdated();
}


void uiEditPropRef::rangeChgCB( CallBacker* )
{
    setUpdated();
}


void uiEditPropRef::unitSel( CallBacker* cb )
{
    if ( !cb )
	return;

    mCBCapsuleUnpack(const UnitOfMeasure*,prevuom,cb);
    const UnitOfMeasure* newun = unfld_->getUnit();
    if ( newun == prevuom )
	return;

    Interval<double> vintv( rgfld_->getDInterval() );
    convValue( vintv.start, prevuom, newun );
    convValue( vintv.stop, prevuom, newun );
    rgfld_->setValue( vintv );
    if ( pr_.disp_.defval_ && pr_.disp_.defval_->isValue() )
    {
	float val = defaultfld_->getDValue();
	convValue( val, prevuom, newun );
	defaultfld_->setValue( val );
    }

    setUpdated();
}


void uiEditPropRef::valueChgCB( CallBacker* )
{
    setUpdated();
}


void uiEditPropRef::definitionChecked( CallBacker* )
{
    definitionformbut_->setSensitive( definitionfld_->isChecked() );
    defaultformbut_->display( !definitionfld_->isChecked() );
    setUpdated();
}


void uiEditPropRef::setForm( bool definition )
{
    PropertyRefSelection prsel( true, &pr_ );
    MathProperty& mped = definition ? definitionmathprop_ : defaultmathprop_;
    uiMathPropEdDlg dlg( parent(), mped, prsel );
    if ( !dlg.go() )
	return;

    (definition?definitionfld_:defaultfld_)->setText( mped.formText(true) );
    setUpdated();
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

    const Mnemonic* mn = MNC().getByName( mnemonicsfld_->text(), false );
    if ( mn && mn != &pr_.mn_ )
    {
	const PropertyRef newpr( *mn, newnm );
	pr_ = newpr;
    }
    else
	pr_.setName( newnm );

    pr_.propaliases_.setEmpty();
    const SeparString ss( praliasfld_->text() );
    const int nral = ss.size();
    for ( int idx=0; idx<nral; idx++ )
	pr_.propaliases_.add( ss[idx] );

    pr_.disp_.color_ = colfld_->color();
    NotifyStopper ns( pr_.unitChanged );
    pr_.disp_.typicalrange_ = rgfld_->getFInterval();
    pr_.setUnit( unfld_->getUnitName() );

    BufferString defaultstr( defaultfld_->text() );
    defaultstr.trimBlanks();
    delete pr_.disp_.defval_;
    if ( defaultstr.isEmpty() )
	pr_.disp_.defval_ = nullptr;
    else
    {
	if ( defaultstr.isNumber() )
	{
	    const float val = defaultstr.toFloat();
	    pr_.disp_.defval_ = new ValueProperty( pr_, val );
	}
	else
	    pr_.disp_.defval_ = new MathProperty( defaultmathprop_ );
    }

    pr_.setFixedDef( isfund ? &definitionmathprop_ : nullptr );

    return true;
}


void uiBuildPROPS::editReq( bool isadd )
{
    const char* nm = isadd ? curAvSel() : curDefSel();
    if ( !nm || !*nm ) return;

    PropertyRef* pr = nullptr;
    const PropertyRefSet& props = const_cast<const PropertyRefSet&>( props_ );
    if ( isadd )
    {
	const Mnemonic* mn = MNC().getByName( nm, false );
	if ( !mn )
	    return;

	BufferString propnm;
	if ( !props.getByName(mn->name(),false) )
	    propnm.set( mn->name() );

	pr = new PropertyRef( *mn, propnm.buf() );
    }
    else
	pr = const_cast<PropertyRef*>( props.getByName(nm,false) );

    if ( !pr )
	return;

    uiEditPropRef dlg( this, *pr, isadd );
    if ( !dlg.go() )
    {
	if ( isadd )
	    delete pr;
    }
    else
    {
	if ( isadd )
	{
	    if ( props.getByName(pr->name(),false) )
	    {
		const BufferString propnm( pr->name() );
		delete pr;
		mErrRet( tr("Property with same name '%1' already "
			    " present.").arg(propnm),  )
	    }

	    props_ += pr;
	}

	usrchg_ = usrchg_ || dlg.isChanged();
	handleSuccessfullEdit( isadd, pr->name() );
    }
}


bool uiBuildPROPS::isPropRemovable( const PropertyRef* propref )
{
    if ( !propref )
	return false;

    if ( propref->isThickness() )
	mErrRet( tr("Cannot remove inbuilt property 'Thickness'."), false )

    if ( propref->stdType() == Mnemonic::Den ||
	 propref->stdType() == Mnemonic::Vel )
    {
	PropertyRefSelection subselprs;
	props_.subselect( propref->stdType(), subselprs );
	if ( subselprs.size()<=1 )
	    mErrRet( tr( "Cannot remove this property. "
			 "Need to have at least one "
			 "usable property of type '%1'")
			.arg(Mnemonic::toString(propref->stdType())), false )
    }

    return true;
}


void uiBuildPROPS::removeReq()
{
    const char* prnm = curDefSel();
    const PropertyRef* pr = const_cast<const PropertyRefSet&>( props_ )
						.getByName( prnm, false );
    if ( isPropRemovable(pr) )
    {
	props_ -= const_cast<PropertyRef*>( pr );
	delete pr;
    }
}


void uiBuildPROPS::itemSwitch( const char* nm1, const char* nm2 )
{
    const PropertyRefSet& props = const_cast<const PropertyRefSet&>( props_ );
    const PropertyRef* pr1 = props.getByName( nm1, false );
    const PropertyRef* pr2 = props.getByName( nm2, false );
    if ( !pr1 || !pr2 )
    {
	pErrMsg("Huh");
	return;
    }

    props_.swap( props.indexOf(pr1), props.indexOf(pr2) );
}


uiManPROPS::uiManPROPS( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Manage Layer Properties"),
				 tr("Define possible layer properties"),
				 mODHelpKey(mManPROPSHelpID)))
{
    setCtrlStyle( CloseOnly );
    buildfld_ = new uiBuildPROPS( this, ePROPS() );
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


uiSelectPropRefs::uiSelectPropRefs( uiParent* p, PropertyRefSelection& prs,
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
{
    return proprefgrp_->acceptOK();
}


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
{
    return proprefgrp_->acceptOK();
}



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
    PropertyRefSelection prs( false, nullptr );
    BufferStringSet dispnms; addNames( prs, dispnms );
    if ( dispnms.isEmpty() )
	return;

    dispnms.sort();
    propfld_->addItems( dispnms );

    int firstsel = -1;
    for ( int idx=0; idx<dispnms.size(); idx++ )
    {
	const char* nm = dispnms.get( idx ).buf();
	const bool issel = prsel_.getByName( nm, false );
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
	const PropertyRef* pr = props_.getByName( pnm, false );
	if ( !pr ) { pErrMsg("Huh"); structchg_ = true; continue; }
	prsel_ += pr;
    }

    return true;
}
