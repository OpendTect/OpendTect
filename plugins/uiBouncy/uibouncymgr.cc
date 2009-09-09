
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika / Bruno
 * DATE     : Aug 2009
-*/

static const char* rcsID = "$Id: uibouncymgr.cc,v 1.2 2009-09-09 08:00:31 cvskarthika Exp $";

#include "uibouncymgr.h"
#include "beachballdata.h"
#include "visbeachball.h"
#include "uibouncymain.h"
#include "uibouncysettingsdlg.h"
#include "bouncycontroller.h"

#include "vissurvscene.h"
#include "uivispartserv.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uimenu.h"
#include "uiodmenumgr.h"
#include "uimsg.h"

namespace uiBouncy
{

uiBouncyMgr::uiBouncyMgr( uiODMain* appl )
	: appl_(appl)
	, vbb_(0)
	, sceneid_(0)
	, maindlg_(0)
	, settingsdlg_(0)
	, gamectr_(0)
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    mnumgr.toolsMnu()->insertItem(
	    new uiMenuItem("B&ouncy",mCB(this,uiBouncyMgr,doWork)) );
}

uiBouncyMgr::~uiBouncyMgr()
{
    removeBeachBall();
    
    if ( gamectr_) 
    {
        gamectr_->newPosAvailable.remove(
		mCB(this, uiBouncyMgr, newPosAvailableCB));
	delete gamectr_;
    }

    if ( settingsdlg_ )
    {
	const CallBack propchgCB ( mCB(this, uiBouncyMgr, propertyChangeCB) );
	settingsdlg_->propertyChanged.remove( propchgCB );
	delete settingsdlg_;
    }

    if ( maindlg_ )
	delete maindlg_;
}


void uiBouncyMgr::removeAllBeachBalls( CallBacker *cb )
{
    if (vbb_)
    {
	vbb_->unRef();
	vbb_ = 0;
    }
}

void uiBouncyMgr::zScaleCB( CallBacker *cb )
{
    setBallScale();
}


void uiBouncyMgr::setBallScale()
{
    if ( !vbb_ )
	return;
    mDynamicCastGet( visSurvey::Scene*, scene,
	    ODMainWin()->applMgr().visServer()->getObject( sceneid_ ) );
    const float zscale = scene ? scene->getZStretch()*scene->getZScale() : 1;
    vbb_->setZScale( zscale );
}


void uiBouncyMgr::addBeachBall()
{
    if ( !settingsdlg_ ) return;

    if ( vbb_ ) vbb_->unRef();
    vbb_ = visBase::BeachBall::create();
    vbb_->ref();
    
    mDynamicCastGet( visSurvey::Scene*, scene,
	    ODMainWin()->applMgr().visServer()->getObject( sceneid_ ) );

    visBase::DM().removeallnotify.notify(
	    mCB(this, uiBouncyMgr, removeAllBeachBalls) );

    if ( scene )
	scene->zstretchchange.notify( mCB(this, uiBouncyMgr, zScaleCB ) );

    vbb_->setDisplayTransformation( scene->getUTM2DisplayTransform() );
    setBallScale();

    vbb_->setBallProperties( settingsdlg_->getBallProperties() );

    ODMainWin()->applMgr().visServer()->addObject( vbb_, sceneid_, true );
}


int uiBouncyMgr::removeBeachBall()
{
    if (!vbb_) return 0;
    ODMainWin()->applMgr().visServer()->removeObject( vbb_, sceneid_ );
    visBase::DM().removeallnotify.remove(
	    mCB(this, uiBouncyMgr, removeAllBeachBalls) );
    mDynamicCastGet( visSurvey::Scene*, scene,
	    ODMainWin()->applMgr().visServer()->getObject( sceneid_ ) );
    if ( scene )
	scene->zstretchchange.remove( mCB(this, uiBouncyMgr, zScaleCB ) );
    removeAllBeachBalls(0);
 
    return 0;
}
    

void uiBouncyMgr::doWork( CallBacker *cb )
{
    bool firsttime = ( !vbb_ );

    sceneid_ = ODMainWin()->sceneMgr().getActiveSceneID();
    mDynamicCastGet( visSurvey::Scene*, scene, 
		ODMainWin()->applMgr().visServer()->getObject( sceneid_ ) );

    if ( !scene ) 
    {
	uiMSG().message( "Cannot start game as there is no scene!" );
	return;
    }

    if ( !maindlg_ )
    {
       maindlg_	= new uiBouncyMain( appl_, &settingsdlg_ );
       const CallBack propchgCB ( mCB(this, uiBouncyMgr, propertyChangeCB) );
       settingsdlg_->propertyChanged.notify( propchgCB );
    } 
   
    visBeachBall::BallProperties currbp;
    if ( vbb_ )
        currbp = vbb_->getBallProperties();
    if ( maindlg_->go() )
    {
	if ( firsttime )
	{
	/*    uiMSG().message( "Welcome to the Bouncy game, ", 
		    maindlg_->getPlayerName(), " !" );*/
	    addBeachBall();
	    startGame();
	}
    }
    else
    {
	if ( firsttime )
	    // remove beachball added for preview
	    removeBeachBall();
	else 
	    // revert settings as user might have changed something
            vbb_->setBallProperties( currbp );
    }
}

void uiBouncyMgr::startGame()
{
    gamectr_ = new Bouncy::BouncyController( "Starting game..." );
    gamectr_->init( vbb_->getCenterPosition() );
    gamectr_->newPosAvailable.notify(mCB(this, uiBouncyMgr, newPosAvailableCB));
    gamectr_->execute();
}


void uiBouncyMgr::stopGame()
{
    gamectr_->stop();
    gamectr_->newPosAvailable.remove(mCB(this, uiBouncyMgr, newPosAvailableCB));
}


void uiBouncyMgr::newPosAvailableCB( CallBacker* )
{
    if ( vbb_ )
    {
	vbb_->setCenterPosition( gamectr_->findNewPos
		( vbb_->getCenterPosition() ) );
	// give rolling effect - add rotate transform
    }
}


void uiBouncyMgr::propertyChangeCB( CallBacker* )
{
    if ( !settingsdlg_ ) return;
    
    if ( !vbb_ )
	addBeachBall();
    else
    {
	if ( vbb_->getBallProperties() != settingsdlg_->getBallProperties() )
	    vbb_->setBallProperties( settingsdlg_->getBallProperties() );
    }
}


void uiBouncyMgr::sessionSaveCB( CallBacker* )
{
/*    IOPar vmbpar;
    manager().fillPar( vmbpar );
    singlegatherupdater_->fillPar ( vmbpar );
    horizonbasedupdater_->fillPar ( vmbpar );
    if ( vismanager_ ) vismanager_->fillPar( vmbpar );
    if ( vertupdatewindow_ ) vertupdatewindow_->fillPar( vmbpar );
    vmbpar.setYN( sKeySingleGatherUpdateWinShown(),
	                      isvertupdatewindowshown_ );
    vmbpar.setYN( sKeyHorUpdateWinShown(),
	                      ishorvertupdatewindowshown_ );
    mainwin_->sessionPars().mergeComp( vmbpar, Manager::sKeyVMB() );*/
}


void uiBouncyMgr::sessionRestoreCB( CallBacker *)
{
/*    PtrMan<IOPar> vmbpar =
	        mainwin_->sessionPars().subselect( Manager::sKeyVMB() );
    if ( !vmbpar ) return;

    getVisManager(); //creates vismanager
    uiTaskRunner taskrunner( mainwin_ );
    if ( !manager().usePar( *vmbpar ) || !vismanager_->usePar( *vmbpar ) ||
	 !singlegatherupdater_->usePar( *vmbpar ) ||
	 !horizonbasedupdater_->usePar( *vmbpar, &taskrunner ) ||
	 !vertupdatewindow_->usePar( *vmbpar ) ||
	 !horvertupdatewindow_->usePar( *vmbpar ) )
    {
	uiMSG().error("Could not restore VMB session" );
    }

    bool tmp;
    if ( vmbpar->getYN( sKeySingleGatherUpdateWinShown(), tmp ) )
	showVerticalUpdateWindow( tmp );
    if ( vmbpar->getYN( sKeyHorUpdateWinShown(), tmp ) )
        showHorVerticalUpdateWindow( tmp );

    toolbar_->updateButtons();*/
}


void uiBouncyMgr::surveyToBeChangedCB( CallBacker* )
{
    // Ask if user wants to quit the game
}


void uiBouncyMgr::surveyChangeCB( CallBacker* )
{
    // reset scores n do other reinit stuff
}


void uiBouncyMgr::shutdownCB( CallBacker* )
{
    // 
}

}
