/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2012
________________________________________________________________________

-*/

#include "uistratlaymodtools.h"

#include "propertyref.h"
#include "stratlayermodel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uispinbox.h"
#include "uistratlvlsel.h"
#include "uitoolbutton.h"
#include "od_helpids.h"

static int defNrModels() { return 100; }

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
    uiGroup* leftgrp = new uiGroup( this, "Left group" );
    uiToolButton* opentb = new uiToolButton( leftgrp, "open",
				tr("Open stored generation description"),
				mCB(this,uiStratGenDescTools,openCB) );
    savetb_ = new uiToolButton( leftgrp, "save",
				tr("Save generation description"),
				mCB(this,uiStratGenDescTools,saveCB) );
    savetb_->attach( rightOf, opentb );
    uiToolButton* proptb = new uiToolButton( leftgrp, "defprops",
				tr("Select layer properties"),
				mCB(this,uiStratGenDescTools,propEdCB) );
    proptb->attach( rightOf, savetb_ );

    auto* rightgrp = new uiGroup( this, "Right group" );
    const CallBack gocb( mCB(this,uiStratGenDescTools,genCB) );
    nrmodlsfld_ = new uiSpinBox( rightgrp );
    nrmodlsfld_->setInterval( Interval<int>(1,mUdf(int)) );
    nrmodlsfld_->setValue( defNrModels() );
    nrmodlsfld_->setFocusChangeTrigger( false );
    nrmodlsfld_->setStretch( 0, 0 );
    nrmodlsfld_->setToolTip( tr("Number of models to generate") );
    nrmodlsfld_->valueChanging.notify(
	    mCB(this,uiStratGenDescTools,nrModelsChangedCB) );
    uiToolButton* runtb = new uiToolButton( rightgrp, "resume",
				tr("Generate this amount of models"), gocb );
    nrmodlsfld_->attach( leftOf, runtb );
    rightgrp->attach( ensureRightOf, leftgrp );
    rightgrp->setFrame( true );
}


int uiStratGenDescTools::nrModels() const
{
    return nrmodlsfld_->getIntValue();
}


void uiStratGenDescTools::setNrModels( int nrmodels )
{
    NotifyStopper notstop( nrmodlsfld_->valueChanged );
    nrmodlsfld_->setValue( nrmodels );
}


int uiStratGenDescTools::getNrModelsFromPar( const IOPar& par ) const
{
    int nrmodels = -1;
    par.get( sKeyNrModels(), nrmodels );
    return nrmodels;
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


void uiStratGenDescTools::enableSave( bool yn )
{
    savetb_->setSensitive( yn );
}

void uiStratGenDescTools::fillPar( IOPar& par ) const
{
    par.set( sKeyNrModels(), nrModels() );
}


bool uiStratGenDescTools::usePar( const IOPar& par )
{
    int nrmodels;
    if ( par.get( sKeyNrModels(), nrmodels ) )
	nrmodlsfld_->setValue( nrmodels );

    return true;
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
    Strat::LVLS().getNames( lvlnms_ );

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

    flattenedtb_ = new uiToolButton( rightgrp_, "flattenseis",
			tr("Show flattened when on"),
			mCB(this,uiStratLayModEditTools,showFlatChgCB) );
    flattenedtb_->setToggleButton( true );
    flattenedtb_->setOn( false );

    zoomtb_ = new uiToolButton( rightgrp_, "toggzooming",
			tr("Do not zoom into models when on"),
			mCB(this,uiStratLayModEditTools,dispZoomedChgCB) );
    zoomtb_->setToggleButton( true );
    zoomtb_->setOn( false );
    zoomtb_->attach( leftOf, flattenedtb_ );

    rightgrp_->attach( rightTo, leftgrp_ );
    rightgrp_->attach( rightBorder );

    postFinalise().notify( mCB(this,uiStratLayModEditTools,initGrp) );
}

void uiStratLayModEditTools::addEachFld()
{
    eachfld_ = new uiSpinBox( leftgrp_, 0, "DispEach" );
    eachfld_->setInterval( 1, 1000 );
    eachfld_->setPrefix( uiStrings::sEach().toLower()
			 .postFixWord(toUiString(" ")) );
    eachfld_->attach( rightOf, contfld_ );
}


void uiStratLayModEditTools::addLithFld()
{
    lithtb_ = new uiToolButton( rightgrp_, "lithologies",
			tr("Show lithology colors when on"),
			mCB(this,uiStratLayModEditTools,dispLithChgCB) );
    lithtb_->setToggleButton( true );
    lithtb_->setOn( true );
    lithtb_->attach( leftOf, zoomtb_ );
}


#define mPropNotif propfld_->selectionChanged
#define mLvlNotif lvlfld_->selChange
#define mContNotif contfld_->selectionChanged
#define mEachNotif eachfld_->valueChanged
#define mZoomNotif zoomtb_->activated
#define mFlatNotif flattenedtb_->activated
#define mLithNotif lithtb_->activated

void uiStratLayModEditTools::initGrp( CallBacker* )
{
    if ( lvlnms_.isEmpty() )
	flattenedtb_->setSensitive( false );

#define mSLMETCB( fn ) mCB(this,uiStratLayModEditTools,fn)
    mPropNotif.notify( mSLMETCB(selPropChgCB) );
    mLvlNotif.notify( mSLMETCB(selLevelChgCB) );
    mContNotif.notify( mSLMETCB(selContentChgCB) );
    if ( eachfld_ )
	mEachNotif.notify( mSLMETCB(dispEachChgCB) );
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
    return propfld_->isEmpty() ? 0 : propfld_->text();
}


Strat::Level uiStratLayModEditTools::selLevel() const
{
    return lvlfld_->selected();
}


Strat::Level::ID uiStratLayModEditTools::selLevelID() const
{
    return lvlfld_->getID();
}


int uiStratLayModEditTools::selLevelIdx() const
{
    return Strat::LVLS().indexOf( lvlfld_->getID() );
}


BufferString uiStratLayModEditTools::selLevelName() const
{
    return selLevelID().name();
}


Color uiStratLayModEditTools::selLevelColor() const
{
    return Strat::LVLS().colorOf( selLevelID() );
}


const char* uiStratLayModEditTools::selContent() const
{
    return contfld_->isEmpty() ? 0 : contfld_->text();
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


#define mPrepNotif(notifier) \
    PtrMan<NotifyStopper> ns = donotif ? 0 : new NotifyStopper( notifier );

void uiStratLayModEditTools::setSelProp( const char* sel, bool donotif )
{
    mPrepNotif( mPropNotif );
    propfld_->setText( sel );
}


void uiStratLayModEditTools::setSelLevel( const char* sel, bool donotif )
{
    mPrepNotif( mLvlNotif )
    lvlfld_->setName( sel );
}


void uiStratLayModEditTools::setSelContent( const char* sel, bool donotif )
{
    mPrepNotif( mContNotif )
    contfld_->setText( sel );
}


void uiStratLayModEditTools::setDispEach( int nr, bool donotif )
{
    if ( eachfld_ )
    {
	mPrepNotif( mEachNotif )
	eachfld_->setValue( nr );
	prevdispeach_ = nr;
    }
}


void uiStratLayModEditTools::setDispZoomed( bool yn, bool donotif )
{
    mPrepNotif( mZoomNotif )
    zoomtb_->setOn( !yn );
}


void uiStratLayModEditTools::setDispLith( bool yn, bool donotif )
{
    if ( lithtb_ )
    {
	mPrepNotif( mLithNotif )
	lithtb_->setOn( yn );
    }
}


void uiStratLayModEditTools::setShowFlattened( bool yn, bool donotif )
{
    mPrepNotif( mFlatNotif )
    flattenedtb_->setOn( yn );
}


void uiStratLayModEditTools::setFlatTBSensitive( bool yn )
{
    flattenedtb_->setSensitive( yn );
}


void uiStratLayModEditTools::fillPar( IOPar& par ) const
{
    par.set( sKeyDisplayedProp(), selProp() );
    par.set( sKeyDecimation(), dispEach() );
    par.set( sKeySelectedLevel(), selLevelName() );
    par.set( sKeySelectedContent(), selContent() );
    par.setYN( sKeyZoomToggle(), dispZoomed() );
    par.setYN( sKeyDispLith(), dispLith() );
    par.setYN( sKeyShowFlattened(), showFlattened() );
}

#define mSetCombo( fld, key ) \
{ \
    FixedString selprop = par.find( key() ); \
    if ( selprop && fld->isPresent( selprop ) ) \
    { \
	fld->setCurrentItem( selprop ); \
	fld->selectionChanged.trigger(); \
    } \
}

#define mSetYN( func, key ) \
{ \
    bool yn; \
    if ( par.getYN( key(), yn ) ) \
	func( yn, true ); \
}


bool uiStratLayModEditTools::usePar( const IOPar& par )
{
    setSelProp( par.find(sKeyDisplayedProp()), true );
    setSelContent( par.find(sKeySelectedContent()), true );
    const char* res = par.find( sKeySelectedLevel() );
    if ( res && *res )
	lvlfld_->setName( res );

    int decimation;
    if ( par.get( sKeyDecimation(), decimation ) )
    {
	if ( decimation > dispEach() )
	    setDispEach( decimation, true );
    }

    mSetYN( setDispZoomed, sKeyZoomToggle );
    mSetYN( setDispLith, sKeyDispLith );
    mSetYN( setShowFlattened, sKeyShowFlattened );

    return true;
}


//-----------------------------------------------------------------------------

#define mCreatePropSelFld( propnm, txt, prop, prevbox ) \
    uiLabeledComboBox* lblbox##propnm = new uiLabeledComboBox( this, txt ); \
    propnm##fld_ = lblbox##propnm->box(); \
    const PropertyRefSelection subsel##propnm = proprefsel.subselect( prop );\
    for ( int idx=0; idx<subsel##propnm.size(); idx++ )\
	if ( subsel##propnm[idx] )\
	    propnm##fld_->addItem( toUiString(subsel##propnm[idx]->name()) );\
    if ( prevbox )\
	lblbox##propnm->attach( alignedBelow, prevbox );


uiStratLayModFRPropSelector::uiStratLayModFRPropSelector( uiParent* p,
					const PropertyRefSelection& proprefsel,
				const uiStratLayModFRPropSelector::Setup& set )
	: uiDialog(p,uiDialog::Setup(tr("Property Selector"),
				     tr("There are multiple properties "
				        "referenced with the same type. \n"
				        "Please specify which one to use as: "),
		     mODHelpKey(mStratSynthLayerModFRPPropSelectorHelpID) ) )
	, vsfld_(0)
	, porosityfld_(0)
	, initialsatfld_(0)
	, finalsatfld_(0)
{
    mCreatePropSelFld(den, tr("Reference for Density"), PropertyRef::Den, 0);
    mCreatePropSelFld(vp, tr("Reference for Vp"), PropertyRef::Vel, lblboxden);
    uiLabeledComboBox* prevfld = lblboxvp;
    if ( set.withswave_ )
    {
	mCreatePropSelFld(vs, tr("Reference for Vs"), PropertyRef::Vel,prevfld);
	prevfld = lblboxvs;
	if ( proprefsel.find(PropertyRef::standardSVelStr()) >=0 ||
	     proprefsel.find(PropertyRef::standardSVelAliasStr()) >= 0 )
	    setVSProp( PropertyRef::standardSVelStr() );
    }

    if ( set.withinitsat_ )
    {
	mCreatePropSelFld( initialsat, tr("Reference for Initial Saturation"),
			   PropertyRef::Volum, prevfld );
	prevfld = lblboxinitialsat;
	if ( proprefsel.find("Water Saturation") >=0 )
	    setInitialSatProp( "Water Saturation" );
    }

    if ( set.withfinalsat_ )
    {
	mCreatePropSelFld( finalsat, tr("Reference for Final Saturation"),
			   PropertyRef::Volum, prevfld );
	prevfld = lblboxfinalsat;
	if ( proprefsel.find("Water Saturation") >=0 )
	    setFinalSatProp( "Water Saturation" );
    }

    if ( set.withpor_ )
    {
	mCreatePropSelFld( porosity, tr("Reference for Porosity"),
			   PropertyRef::Volum, prevfld );
	if ( proprefsel.find("Porosity") >= 0 )
	    setPorProp( "Porosity" );
    }

    const bool haspwave =
	    proprefsel.find(PropertyRef::standardPVelStr()) >=0 ||
	    proprefsel.find(PropertyRef::standardPVelAliasStr()) >= 0;
    if ( haspwave )
	setVPProp( PropertyRef::standardPVelStr() );
    else
	errmsg_ = tr( "No reference to P wave velocity found" );
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


bool uiStratLayModFRPropSelector::acceptOK()
{
    return isOK();
}


const char* uiStratLayModFRPropSelector::getSelVPName() const
{ return vpfld_->text(); }

const char* uiStratLayModFRPropSelector::getSelVSName() const
{ return vsfld_ ? vsfld_->text() : 0; }

const char* uiStratLayModFRPropSelector::getSelDenName() const
{ return denfld_->text(); }

const char* uiStratLayModFRPropSelector::getSelPorName() const
{ return porosityfld_ ? porosityfld_->text() : 0; }

const char* uiStratLayModFRPropSelector::getSelInitialSatName() const
{ return initialsatfld_ ? initialsatfld_->text() : 0; }

const char* uiStratLayModFRPropSelector::getSelFinalSatName() const
{ return finalsatfld_ ? finalsatfld_->text() : 0; }
