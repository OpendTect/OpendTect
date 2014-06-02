/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
#include "uitoolbutton.h"
#include "uiunitsel.h"
#include "unitofmeasure.h"
#include "od_helpids.h"


class uiBuildPROPS : public uiBuildListFromList
{
public:
			uiBuildPROPS(uiParent*,PropertyRefSet&,bool);

    PropertyRefSet&	props_;
    bool		allowmath_;

    virtual const char*	avFromDef(const char*) const;
    virtual void	editReq(bool);
    virtual void	removeReq();
    virtual void	itemSwitch(const char*,const char*);
    void		initGrp(CallBacker*);

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
{
public:

			uiEditPropRef(uiParent*,PropertyRef&,bool,bool);
    bool		acceptOK(CallBacker*);

    PropertyRef&	pr_;
    const bool		withform_;

    uiGenInput*		namefld_;
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
    void		unitSel(CallBacker*);

};


uiEditPropRef::uiEditPropRef( uiParent* p, PropertyRef& pr, bool isadd,
				bool supportform )
    : uiDialog(p,uiDialog::Setup("Property definition",
		                 BufferString(isadd?"Add '":"Edit '",
		                 PropertyRef::toString(pr.stdType()),
                                 "' property"),
		                 mODHelpKey(mEditPropRefHelpID) ))
    , pr_(pr)
    , defaultmathprop_(pr)
    , definitionmathprop_(pr)
    , withform_(supportform)
    , curunit_(0)
{
    namefld_ = new uiGenInput( this, "Name", StringInpSpec(pr.name()) );
    SeparString ss;
    for ( int idx=0; idx<pr_.aliases().size(); idx++ )
	ss += pr_.aliases().get(idx);
    aliasfld_ = new uiGenInput( this, "Aliases (e.g. 'abc, uvw*xyz')",
				StringInpSpec(ss.buf()) );
    aliasfld_->attach( alignedBelow, namefld_ );

    colfld_ = new uiColorInput( this, uiColorInput::Setup(pr_.disp_.color_)
					.lbltxt("Default display color") );
    colfld_->attach( alignedBelow, aliasfld_ );
    rgfld_ = new uiGenInput( this, "Typical value range",
			     FloatInpIntervalSpec() );
    rgfld_->attach( alignedBelow, colfld_ );
    unfld_ = new uiUnitSel( this, pr_.stdType() );
    unfld_->setUnit( pr_.disp_.unit_ );
    unfld_->attach( rightOf, rgfld_ );
    unfld_->selChange.notify( mCB(this,uiEditPropRef,unitSel) );
    curunit_ = unfld_->getUnit();

    Interval<float> vintv( pr_.disp_.range_ );
    if ( curunit_ )
    {
	if ( !mIsUdf(vintv.start) )
	    vintv.start = curunit_->getUserValueFromSI( vintv.start );
	if ( !mIsUdf(vintv.stop) )
	    vintv.stop = curunit_->getUserValueFromSI( vintv.stop );
    }
    rgfld_->setValue( vintv );

    defaultfld_ = new uiGenInput( this, "Default value" );
    defaultfld_->attach( alignedBelow, rgfld_ );
    if ( !pr_.disp_.defval_ )
	defaultfld_->setValue( pr_.disp_.commonValue() );
    else if ( pr_.disp_.defval_->isValue() )
	defaultfld_->setValue( pr_.disp_.defval_->value() );
    else
    {
	defaultmathprop_.setDef( pr_.disp_.defval_->def() );
	defaultfld_->setText( defaultmathprop_.formText(true) );
    }
    defaultformbut_ = new uiPushButton( this, "&Formula",
				mCB(this,uiEditPropRef,setDefaultForm), false );
    defaultformbut_->attach( rightOf, defaultfld_ );

    definitionfld_ = new uiGenInput( this, "Fixed definition" );
    definitionfld_->attach( alignedBelow, defaultfld_ );
    definitionfld_->setWithCheck( true );
    definitionfld_->setChecked( pr_.hasFixedDef() );
    if ( pr_.hasFixedDef() )
    {
	definitionmathprop_.setDef( pr_.fixedDef().def() );
	definitionfld_->setText( definitionmathprop_.formText(true) );
    }
    definitionformbut_ = new uiPushButton( this, "&Formula",
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
    convUserValue( vintv.start, curunit_, newun );
    convUserValue( vintv.stop, curunit_, newun );
    rgfld_->setValue( vintv );

    curunit_ = newun;
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


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiEditPropRef::acceptOK( CallBacker* )
{
    const BufferString newnm( namefld_->text() );
    if ( newnm.isEmpty() || !isalpha(newnm[0]) || newnm.size() < 2 )
	mErrRet("Please enter a valid name");
    const BufferString definitionstr( definitionfld_->text() );
    const bool isfund = definitionfld_->isChecked();
    if ( isfund && definitionstr.isEmpty() )
	mErrRet("Please un-check the 'Fixed Definition' - or provide one");

    pr_.setName( newnm );
    SeparString ss( aliasfld_->text() ); const int nral = ss.size();
    pr_.aliases().erase();
    for ( int idx=0; idx<nral; idx++ )
	pr_.aliases().add( ss[idx] );
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
    pr_.disp_.range_ = vintv;

    BufferString defaultstr( defaultfld_->text() );
    defaultstr.trimBlanks();
    if ( defaultstr.isEmpty() )
	{ delete pr_.disp_.defval_; pr_.disp_.defval_ = 0; }
    else
    {
	if ( !withform_ || defaultstr.isNumber() )
	    pr_.disp_.defval_ = new ValueProperty( pr_, defaultstr.toFloat() );
	else
	    pr_.disp_.defval_ = new MathProperty( defaultmathprop_ );
    }

    if ( !isfund )
	pr_.setFixedDef( 0 );
    else
	pr_.setFixedDef( definitionmathprop_.clone() );

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
	    props_ += pr;
	handleSuccessfullEdit( isadd, pr->name() );
    }
}


void uiBuildPROPS::removeReq()
{
    const char* prnm = curDefSel();
    if ( prnm && *prnm )
    {
	const int idx = props_.indexOf( prnm );
	if ( idx < 0 ) return;
	delete props_.removeSingle( idx );
	removeItem();
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
    : uiDialog(p,uiDialog::Setup(tr("Layer Properties - Definition"),
				 tr("Define possible layer properties"),
                                 mODHelpKey(mManPROPSHelpID) ))
{
    setCtrlStyle( CloseOnly );
    buildfld_ = new uiBuildPROPS( this, ePROPS(), true );
    const char* strs[] = { "For this survey only",
			   "As default for all surveys",
			   "As default for my user ID only", 0 };
    srcfld_ = new uiGenInput( this, "Store", StringListInpSpec(strs) );
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
	uiMSG().warning( tr("Could not store the definitions to file."
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
    : uiDialog(p,uiDialog::Setup("Layer Properties - Selection",
				"Select layer properties to use",
                                mODHelpKey(mSelectPropRefsHelpID) ))
    , props_(PROPS())
    , prsel_(prs)
    , thref_(&PropertyRef::thickness())
    , structchg_(false)
{
    uiLabeledListBox* llb = 0;
    if ( !lbl || !*lbl )
	propfld_ = new uiListBox( this, "Available properties",
				  OD::ChooseAtLeastOne );
    else
    {
	llb = new uiLabeledListBox( this, lbl, OD::ChooseAtLeastOne,
				    uiLabeledListBox::AboveMid );
	propfld_ = llb->box();
    }
    fillList();

    uiToolButton* manpropsbut = new uiToolButton( this, "man_props",
					"Manage available properties",
					mCB(this,uiSelectPropRefs,manPROPS) );
    if ( llb )
	manpropsbut->attach( centeredRightOf, llb );
    else
	manpropsbut->attach( centeredRightOf, propfld_ );
}


void uiSelectPropRefs::fillList()
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


void uiSelectPropRefs::manPROPS( CallBacker* )
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


bool uiSelectPropRefs::acceptOK( CallBacker* )
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
