/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratlaymodtools.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uispinbox.h"
#include "uistratlvlsel.h"
#include "uitoolbutton.h"

#include "keystrs.h"
#include "od_helpids.h"
#include "propertyref.h"
#include "stratlayermodel.h"
#include "stratlevel.h"


static int defNrModels() { return 100; }
static int defNrCalcEach() { return 5; }

const char* uiStratGenDescTools::sKeyNrModels()
{ return "Nr models"; }

const char* uiStratLayModEditTools::sKeyDisplayedProp()
{ return "Displayed property"; }

const char* uiStratLayModEditTools::sKeyDecimation()
{ return "Decimation"; }

const char* uiStratLayModEditTools::sKeySelectedLevel()
{ return "Selected level"; }

const char* uiStratLayModEditTools::sKeySelectedContent()
{return "Selected content";}

const char* uiStratLayModEditTools::sKeyZoomToggle()
{ return "Allow zoom"; }

const char* uiStratLayModEditTools::sKeyDispLith()
{ return "Display lithology"; }


const char* uiStratLayModEditTools::sKeyShowFlattened()
{ return "Display flattened";}


uiStratGenDescTools::uiStratGenDescTools( uiParent* p )
    : uiGroup(p,"Gen Desc Tools")
    , openReq(this)
    , saveReq(this)
    , propEdReq(this)
    , genReq(this)
    , nrModelsChanged(this)
{
    auto* leftgrp = new uiGroup( this, "Left group" );
    auto* opentb = new uiToolButton( leftgrp, "open",
				tr("Open stored generation description"),
				mCB(this,uiStratGenDescTools,openCB) );
    savetb_ = new uiToolButton( leftgrp, "save",
				tr("Save generation description"),
				mCB(this,uiStratGenDescTools,saveCB) );
    savetb_->attach( rightOf, opentb );
    auto* proptb = new uiToolButton( leftgrp, "defprops",
				tr("Select layer properties"),
				mCB(this,uiStratGenDescTools,propEdCB) );
    proptb->attach( rightOf, savetb_ );

    auto* rightgrp = new uiGroup( this, "Right group" );
    nrmodlsfld_ = new uiSpinBox( rightgrp );
    nrmodlsfld_->setInterval( Interval<int>(1,mUdf(int)) );
    nrmodlsfld_->setValue( defNrModels() );
    nrmodlsfld_->setFocusChangeTrigger( false );
    nrmodlsfld_->setStretch( 0, 0 );
    nrmodlsfld_->setToolTip( tr("Number of models to generate") );
    mAttachCB( nrmodlsfld_->valueChanging,
	       uiStratGenDescTools::nrModelsChangedCB );
    mAttachCB( nrmodlsfld_->valueChanged, uiStratGenDescTools::genCB );
    const CallBack gocb( mCB(this,uiStratGenDescTools,genCB) );
    auto* gotb = new uiToolButton( rightgrp, "go",
				tr("Generate this amount of models"), gocb );
    nrmodlsfld_->attach( leftOf, gotb );
    rightgrp->attach( ensureRightOf, leftgrp );
}


uiStratGenDescTools::~uiStratGenDescTools()
{
    detachAllNotifiers();
}


void uiStratGenDescTools::setNrModels( int nrmodels )
{
    NotifyStopper notstop( nrmodlsfld_->valueChanged );
    nrmodlsfld_->setValue( nrmodels );
}


int uiStratGenDescTools::nrModels() const
{
    return nrmodlsfld_->getIntValue();
}


void uiStratGenDescTools::enableSave( bool yn )
{
    savetb_->setSensitive( yn );
}

int uiStratGenDescTools::getNrModelsFromPar( const IOPar& par ) const
{
    int nrmodels = -1;
    par.get( sKeyNrModels(), nrmodels );
    return nrmodels;
}


void uiStratGenDescTools::fillPar( IOPar& par ) const
{
    par.set( sKeyNrModels(), nrModels() );
}


bool uiStratGenDescTools::usePar( const IOPar& par )
{
    int nrmodels = getNrModelsFromPar( par );
    if ( nrmodels<0 )
	nrmodels = defNrModels();

    nrmodlsfld_->setValue( nrmodels );
    return true;
}


void uiStratGenDescTools::genCB( CallBacker* )
{
    if ( !genaskcontinuemsg_.isEmpty() )
    {
	if ( !uiMSG().askContinue(genaskcontinuemsg_) )
	    return;
	genaskcontinuemsg_.setEmpty();
    }

    genReq.trigger();
}


void uiStratGenDescTools::nrModelsChangedCB( CallBacker* )
{
    nrModelsChanged.trigger();
}


static void setFldNms( uiComboBox* cb, const BufferStringSet& nms, bool wnone,
			bool wall, int def )
{
    const BufferString selnm( cb->text() );
    NotifyStopper ns( cb->selectionChanged );

    cb->setEmpty();
    if ( wnone )
	cb->addItem( toUiString("---") );

    if ( !nms.isEmpty() )
    {
	cb->addItems( nms );
	if ( wall )
	    cb->addItem( uiStrings::sAll() );

	if ( wnone )
	    def++;

	if ( !selnm.isEmpty() )
	{
	    def = cb->indexOf( selnm );
	    if ( def < 0 )
		def = 0;
	}

	if ( def >= cb->size() )
	    def = cb->size() - 1;

	if ( def >= 0 )
	    cb->setCurrentItem( def );
    }

    if ( selnm != cb->text() )
	cb->selectionChanged.trigger( cb );
}


uiStratLayModEditTools::uiStratLayModEditTools( uiParent* p )
    : uiGroup(p,"Lay Mod Edit Tools")
    , selPropChg(this)
    , selLevelChg(this)
    , selContentChg(this)
    , dispEachChg(this)
    , dispZoomedChg(this)
    , dispLithChg(this)
    , showFlatChg(this)
{
    leftgrp_ = new uiGroup( this, "Left group" );
    propfld_ = new uiComboBox( leftgrp_, "Display property" );
    propfld_->setToolTip( tr("Displayed property") );

    lvlfld_ = new uiStratLevelSel( leftgrp_, false, uiString::empty() );
    lvlfld_->setToolTip( tr("Selected level") );
    lvlfld_->attach( rightOf, propfld_ );

    contfld_ = new uiComboBox( leftgrp_, "Content" );
    contfld_->setToolTip( tr("Marked content") );
    contfld_->attach( rightOf, lvlfld_ );
    contfld_->setHSzPol( uiObject::Small );

    rightgrp_ = new uiGroup( this, "Right group" );

    zoomtb_ = new uiToolButton( rightgrp_, "toggzooming",
			tr("Do not zoom into models when on"),
			mCB(this,uiStratLayModEditTools,dispZoomedChgCB) );
    zoomtb_->setToggleButton( true );
    zoomtb_->setOn( false );

    flattenedtb_ = new uiToolButton( rightgrp_, "flattenseis",
			tr("Show flattened when on"),
			mCB(this,uiStratLayModEditTools,showFlatChgCB) );
    flattenedtb_->setToggleButton( true );
    flattenedtb_->setOn( false );
    flattenedtb_->attach( rightOf, zoomtb_ );

    rightgrp_->attach( rightTo, leftgrp_ );
    rightgrp_->attach( rightBorder );

    mAttachCB( postFinalize(), uiStratLayModEditTools::initGrp );
}


void uiStratLayModEditTools::addEachFld()
{
    if ( eachlbl_ )
	return;

    eachlbl_ = new uiLabel( leftgrp_, tr("each") );
    eachlbl_->attach( rightOf, contfld_ );
    eachfld_ = new uiSpinBox( leftgrp_, 0, "DispEach" );
    eachfld_->setInterval( 1, 10000 );
    eachfld_->setValue( defNrCalcEach() );
    eachfld_->attach( rightOf, eachlbl_ );
}


void uiStratLayModEditTools::addLithFld()
{
    if ( lithtb_ )
	return;

    lithtb_ = new uiToolButton( rightgrp_, "lithologies",
			tr("Show lithology colors when on"),
			mCB(this,uiStratLayModEditTools,dispLithChgCB) );
    lithtb_->setToggleButton( true );
    lithtb_->setOn( true );
    lithtb_->attach( leftOf, zoomtb_ );
}


uiStratLayModEditTools::~uiStratLayModEditTools()
{
    detachAllNotifiers();
}


void uiStratLayModEditTools::initGrp( CallBacker* )
{
    if ( Strat::LVLS().isEmpty() )
	flattenedtb_->setSensitive( false );

    mAttachCB( propfld_->selectionChanged,
	       uiStratLayModEditTools::selPropChgCB );
    mAttachCB( lvlfld_->selChange, uiStratLayModEditTools::selLevelChgCB );
    mAttachCB( contfld_->selectionChanged,
	       uiStratLayModEditTools::selContentChgCB );
    if ( eachfld_ )
	mAttachCB( eachfld_->valueChanging,
		   uiStratLayModEditTools::dispEachChgCB );
}


void uiStratLayModEditTools::dispEachChgCB( CallBacker* )
{
	// fix for spinbox possibly giving 2 notifications
    const int newdispeach = dispEach();
    if ( newdispeach == prevdispeach_ )
	return;
    prevdispeach_ = newdispeach;
    dispEachChg.trigger();
}


void uiStratLayModEditTools::setProps( const BufferStringSet& nms )
{
    setFldNms( propfld_, nms, false, false, 0 );
}


void uiStratLayModEditTools::setContentNames( const BufferStringSet& nms )
{
    setFldNms( contfld_, nms, true, true, -1 );
}


int uiStratLayModEditTools::selPropIdx() const
{
    return propfld_->isEmpty() ? -1 : propfld_->currentItem();
}


const char* uiStratLayModEditTools::selProp() const
{
    return propfld_->isEmpty() ? nullptr : propfld_->text();
}


Strat::Level uiStratLayModEditTools::selLevel() const
{
    return lvlfld_->selected();
}


Strat::LevelID uiStratLayModEditTools::selLevelID() const
{
    return lvlfld_->getID();
}


int uiStratLayModEditTools::selLevelIdx() const
{
    return Strat::LVLS().indexOf( lvlfld_->getID() );
}


BufferString uiStratLayModEditTools::selLevelName() const
{
    return Strat::levelNameOf( selLevelID() );
}


OD::Color uiStratLayModEditTools::selLevelColor() const
{
    return Strat::LVLS().colorOf( selLevelID() );
}


const char* uiStratLayModEditTools::selContent() const
{
    return contfld_->isEmpty() ? nullptr : contfld_->text();
}


int uiStratLayModEditTools::dispEach() const
{
    return eachfld_ ? eachfld_->getIntValue() : 1;
}


bool uiStratLayModEditTools::dispZoomed() const
{
    return !zoomtb_->isOn();
}


bool uiStratLayModEditTools::dispLith() const
{
    return lithtb_ && lithtb_->isOn();
}


bool uiStratLayModEditTools::showFlattened() const
{
    return flattenedtb_->isOn() && selLevelID().isValid();
}


void uiStratLayModEditTools::setSelProp( const char* sel, bool donotif )
{
    PtrMan<NotifyStopper> ns =
			  donotif ? nullptr : new NotifyStopper( selPropChg );
    propfld_->setText( sel );
}


void uiStratLayModEditTools::setSelLevel( const char* sel, bool donotif )
{
    PtrMan<NotifyStopper> ns =
			  donotif ? nullptr : new NotifyStopper( selLevelChg );
    lvlfld_->setName( sel );
}


void uiStratLayModEditTools::setSelContent( const char* sel, bool donotif )
{
    PtrMan<NotifyStopper> ns =
			  donotif ? nullptr : new NotifyStopper( selContentChg);
    contfld_->setText( sel );
}


void uiStratLayModEditTools::setMaxDispEach( int maxnr, bool donotif )
{
    if ( !eachfld_ )
	return;

    const int cureachval = eachfld_->getIntValue();
    if ( cureachval > maxnr )
	setDispEach( maxnr, donotif );

    eachfld_->setMaxValue( maxnr );
}


void uiStratLayModEditTools::setDispEach( int nr, bool donotif )
{
    if ( !eachfld_ )
	return;

    PtrMan<NotifyStopper> ns =
			  donotif ? nullptr : new NotifyStopper( dispEachChg );
    eachfld_->setValue( nr );
    prevdispeach_ = nr;
}


void uiStratLayModEditTools::setDispZoomed( bool yn, bool donotif )
{
    PtrMan<NotifyStopper> ns =
			  donotif ? nullptr : new NotifyStopper( dispZoomedChg);
    zoomtb_->setOn( !yn );
}


void uiStratLayModEditTools::setDispLith( bool yn, bool donotif )
{
    PtrMan<NotifyStopper> ns =
			  donotif ? nullptr : new NotifyStopper( dispLithChg );
    lithtb_->setOn( yn );
}


void uiStratLayModEditTools::setShowFlattened( bool yn, bool donotif )
{
    PtrMan<NotifyStopper> ns =
			  donotif ? nullptr : new NotifyStopper( showFlatChg );
    flattenedtb_->setOn( yn );
}


void uiStratLayModEditTools::setFlatTBSensitive( bool yn )
{
    flattenedtb_->setSensitive( yn );
}


void uiStratLayModEditTools::fillPar( IOPar& par ) const
{
    par.set( sKeyDisplayedProp(), selProp() );
    par.set( sKeySelectedLevel(), selLevelName() );
    par.set( sKeySelectedContent(), selContent() );
    par.set( sKeyDecimation(), dispEach() );
    par.setYN( sKeyDispLith(), dispLith() );
    par.setYN( sKeyZoomToggle(), dispZoomed() );
    par.setYN( sKeyShowFlattened(), showFlattened() );
}

#define mSetYN( func, key ) \
{ \
    bool yn; \
    if ( par.getYN(key(),yn) ) \
	func( yn, notif ); \
}


bool uiStratLayModEditTools::usePar( const IOPar& par, bool notif )
{
    setSelProp( par.find(sKeyDisplayedProp()), notif );
    setSelContent( par.find(sKeySelectedContent()), notif );
    const BufferString res = par.find( sKeySelectedLevel() );
    if ( !res.isEmpty() )
	lvlfld_->setName( res );

    int decimation;
    if ( eachfld_ && par.get(sKeyDecimation(),decimation) )
	setDispEach( decimation, notif );

    if ( lithtb_ )
	mSetYN( setDispLith, sKeyDispLith );

    mSetYN( setDispZoomed, sKeyZoomToggle );
    mSetYN( setShowFlattened, sKeyShowFlattened );

    return true;
}


// uiStratLayModFRPropSelector::Setup

uiStratLayModFRPropSelector::Setup::Setup( bool needswave, bool needpor,
					   bool needinitsat, bool needfinalsat )
    : withswave_(needswave)
    , withpor_(needpor)
    , withinitsat_(needinitsat)
    , withfinalsat_(needfinalsat)
{
}


uiStratLayModFRPropSelector::Setup::~Setup()
{
}


// uiStratLayModFRPropSelector

uiStratLayModFRPropSelector::uiStratLayModFRPropSelector( uiParent* p,
					const PropertyRefSelection& proprefsel,
				const uiStratLayModFRPropSelector::Setup& set )
    : uiDialog(p,uiDialog::Setup(tr("Property Selector"),
				 tr("There are multiple properties "
				    "referenced with the same type. \n"
				    "Please specify which one to use as: "),
		 mODHelpKey(mStratSynthLayerModFRPPropSelectorHelpID) ) )
{
    uiLabeledComboBox* lastbox = nullptr;
    denfld_ = createPropSelFld( tr("Reference for Density"), proprefsel,
				Mnemonic::defDEN(), lastbox );
    vpfld_ = createPropSelFld( tr("Reference for Vp"), proprefsel,
			       Mnemonic::defPVEL(), lastbox );
    ePROPS().ensureHasElasticProps( set.withswave_ );
    if ( set.withswave_ )
    {
	vsfld_ = createPropSelFld( tr("Reference for Vs"), proprefsel,
				   Mnemonic::defSVEL(), lastbox );
	const PropertyRef* svelpr =
				proprefsel.getByMnemonic( Mnemonic::defSVEL() );
	if ( svelpr )
	    setVSProp( svelpr->name() );
    }

    if ( set.withinitsat_ )
    {
	initialsatfld_ = createPropSelFld(
				tr("Reference for Initial Saturation"),
				proprefsel, Mnemonic::defSW(), lastbox );
	const PropertyRef* swpr = proprefsel.getByMnemonic( Mnemonic::defSW() );
	if ( swpr )
	    setInitialSatProp( swpr->name() );
    }

    if ( set.withfinalsat_ )
    {
	finalsatfld_ = createPropSelFld( tr("Reference for Final Saturation"),
				proprefsel, Mnemonic::defSW(), lastbox );
	const PropertyRef* swpr = proprefsel.getByMnemonic( Mnemonic::defSW() );
	if ( swpr )
	    setFinalSatProp( swpr->name() );
    }

    if ( set.withpor_ )
    {
	porosityfld_ = createPropSelFld( tr("Reference for Porosity"),
				proprefsel, Mnemonic::defPHI(), lastbox );
	const PropertyRef* porpr = proprefsel.getByMnemonic(Mnemonic::defPHI());
	if ( porpr )
	    setPorProp( porpr->name() );
	else
	{
	    const MnemonicSelection mnsel = MnemonicSelection::getAllPorosity();
	    if ( !mnsel.isEmpty() )
	    {
		porpr = proprefsel.getByMnemonic( *mnsel.first() );
		if ( porpr )
		    setPorProp( porpr->name() );
	    }
	}
    }

    const PropertyRef* denpr = proprefsel.getByMnemonic( Mnemonic::defDEN() );
    if ( denpr )
	setDenProp( denpr->name() );
    else
	errmsg_ = tr( "No reference to density found" );

    const PropertyRef* pvelpr =
			    proprefsel.getByMnemonic( Mnemonic::defPVEL() );
    if ( pvelpr )
	setVPProp( pvelpr->name() );
    else
	errmsg_ = tr( "No reference to P wave velocity found" );
}


uiStratLayModFRPropSelector::~uiStratLayModFRPropSelector()
{
}


uiComboBox* uiStratLayModFRPropSelector::createPropSelFld(
			      const uiString& lbl,
			      const PropertyRefSelection& proprefsel,
			      const Mnemonic& mn,
			      uiLabeledComboBox*& prevlblbox )
{
    auto* lblbox = new uiLabeledComboBox( this, lbl );
    auto* propfld = lblbox->box();

    const MnemonicSelection mnsel = MnemonicSelection::getGroupFor( mn );
    const PropertyRefSelection subselprs = proprefsel.subselect( mn.stdType() );
    for ( const auto* pr : subselprs )
    {
	if ( pr && mnsel.isPresent(&pr->mn()) )
	    propfld->addItem( pr->name() );
    }

    if ( prevlblbox )
	lblbox->attach( alignedBelow, prevlblbox );

    prevlblbox = lblbox;

    return propfld;
}


bool uiStratLayModFRPropSelector::isOK() const
{
    if ( !errmsg_.isEmpty() )
	return false;

    if ( denfld_->isEmpty() || vpfld_->isEmpty() ||
	 (vsfld_ && vsfld_->isEmpty()) )
	return false;

    if ( vsfld_ &&
	 BufferString(vpfld_->text()) == BufferString(vsfld_->text()) )
    {
	uiStratLayModFRPropSelector& modsel =
			const_cast<uiStratLayModFRPropSelector&>( *this );
	modsel.errmsg_ = tr( "Selected property for P wave velocity and "
			     "S wave velocity should be different." );
    }

    if ( (porosityfld_ && porosityfld_->isEmpty()) ||
	 (initialsatfld_ && initialsatfld_->isEmpty()) ||
	 (finalsatfld_ && finalsatfld_->isEmpty()) )
	return false;

    return errmsg_.isEmpty();
}


#define mSetPropFromNm(propfunc,propnm) \
    void uiStratLayModFRPropSelector::set##propfunc##Prop( const char* nm ) \
    { \
	if ( !propnm##fld_ ) \
	    return; \
	\
	if ( propnm##fld_->isPresent(nm) ) \
	    propnm##fld_->setCurrentItem( nm ); \
	else \
	    propnm##fld_->setCurrentItem(0); \
    } \

mSetPropFromNm(Den,den);
mSetPropFromNm(VP,vp);
mSetPropFromNm(VS,vs);
mSetPropFromNm(Por,porosity);
mSetPropFromNm(InitialSat,initialsat);
mSetPropFromNm(FinalSat,finalsat);


bool uiStratLayModFRPropSelector::needsDisplay() const
{
    if ( denfld_->size() > 1 )
	return true;

    if ( vpfld_->size()>1 && (vsfld_ && vsfld_->size()>1) &&
	 BufferString(getSelVPName()) == BufferString(getSelVSName()) )
	return true;

    if ( (porosityfld_ && porosityfld_->size() > 1) ||
	 (initialsatfld_ && initialsatfld_->size() > 1) ||
	 (finalsatfld_ && finalsatfld_->size() > 1) )
	return true;

    return !isOK();
}


bool uiStratLayModFRPropSelector::acceptOK( CallBacker* )
{
    return isOK();
}


const char* uiStratLayModFRPropSelector::getSelVPName() const
{ return vpfld_->text(); }

const char* uiStratLayModFRPropSelector::getSelVSName() const
{ return vsfld_ ? vsfld_->text() : nullptr; }

const char* uiStratLayModFRPropSelector::getSelDenName() const
{ return denfld_->text(); }

const char* uiStratLayModFRPropSelector::getSelPorName() const
{ return porosityfld_ ? porosityfld_->text() : nullptr; }

const char* uiStratLayModFRPropSelector::getSelInitialSatName() const
{ return initialsatfld_ ? initialsatfld_->text() : nullptr; }

const char* uiStratLayModFRPropSelector::getSelFinalSatName() const
{ return finalsatfld_ ? finalsatfld_->text() : nullptr; }
