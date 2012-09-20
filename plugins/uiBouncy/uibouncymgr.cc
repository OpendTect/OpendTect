
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika / Bruno
 * DATE     : Aug 2009
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "uibouncymgr.h"
#include "beachballdata.h"
#include "visbeachball.h"
#include "uibouncymain.h"
#include "uibouncysettingsdlg.h"
#include "visbouncydisplay.h"
#include "bouncycontroller.h"

#include "vissurvscene.h"
#include "uivispartserv.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uimenu.h"
#include "uiodmenumgr.h"
#include "uimsg.h"
#include "survinfo.h"

namespace uiBouncy
{

uiBouncyMgr::uiBouncyMgr( uiODMain* appl )
	: appl_(appl)
        , bouncydisp_(0)
	, maindlg_(0)
	, settingsdlg_(0)
	, gamecontroller_(new Bouncy::BouncyController)
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    mnumgr.toolsMnu()->insertItem(
	    new uiMenuItem("B&ouncy",mCB(this,uiBouncyMgr,doWork)) );

    visBase::DM().removeallnotify.notify(
	    mCB(this, uiBouncyMgr, destroyAllBounciesCB) );
    maindlg_ = new uiBouncyMain( appl_, &settingsdlg_ );
    const CallBack propchgCB ( mCB(this, uiBouncyMgr, propertyChangeCB) );
    settingsdlg_->propertyChanged.notify( propchgCB );
}


uiBouncyMgr::~uiBouncyMgr()
{
    destroyAllBounciesCB( 0 );
    visBase::DM().removeallnotify.remove(
		   mCB(this, uiBouncyMgr, destroyAllBounciesCB) ); 

    gamecontroller_->newPosAvailable.remove(
	    mCB(this, uiBouncyMgr, newPosAvailableCB));
    delete gamecontroller_;

    const CallBack propchgCB ( mCB(this, uiBouncyMgr, propertyChangeCB) );
    settingsdlg_->propertyChanged.remove( propchgCB );
    delete settingsdlg_;
    
    delete maindlg_;
}


void uiBouncyMgr::createBouncy()
{
    destroyBouncy();
    bouncydisp_ = uiBouncy::BouncyDisplay::create();
    bouncydisp_->ref();
    bouncydisp_->setSceneID( sceneid_ );
    bouncydisp_->addBouncy( settingsdlg_->getBallProperties() );
    ODMainWin()->applMgr().visServer()->addObject( 
	    bouncydisp_, sceneid_, true );
}


void uiBouncyMgr::destroyBouncy()
{
    if ( bouncydisp_ )
    {
	bouncydisp_->removeBouncy();
	ODMainWin()->applMgr().visServer()->removeObject( 
		bouncydisp_, sceneid_ );
	bouncydisp_->unRef();
	bouncydisp_->newEvent.remove( mCB(this, uiBouncyMgr, neweventCB) );
	bouncydisp_ = 0;
    }
}


void uiBouncyMgr::destroyAllBounciesCB( CallBacker *cb )
{
    // later, destroy all bouncies
    destroyBouncy();
}


void uiBouncyMgr::doWork( CallBacker *cb )
{
    bool firsttime = ( !bouncydisp_ );

    sceneid_ = ODMainWin()->sceneMgr().getActiveSceneID();
    mDynamicCastGet( visSurvey::Scene*, scene, 
		ODMainWin()->applMgr().visServer()->getObject( sceneid_ ) );

    if ( !scene ) 
    {
	uiMSG().message( "Cannot start game because there is no scene!" );
	return;
    }

    visBeachBall::BallProperties currbp;
    if ( bouncydisp_ )
        currbp = bouncydisp_->getBallProperties();

    if ( maindlg_->go() )
    {
	if ( firsttime )
	{
	    uiMSG().message( "Welcome to the Bouncy game, ", 
		    maindlg_->getPlayerName(), " !" );
	    if ( !bouncydisp_ )
		createBouncy();
	    startGame();
	}
    }
    else if ( bouncydisp_ )
    {
	if ( firsttime )
	{
	    // remove beachball added for preview
	    destroyBouncy();
	}
	else 
	    // revert settings as user might have changed something
            bouncydisp_->setBallProperties( currbp );
    }
}


void uiBouncyMgr::startGame()
{
    if ( !bouncydisp_ )
	return;

    bool work = true;
    gamecontroller_->init( bouncydisp_->getBallPosition(), 
	    SI().minCoord( work ), SI().maxCoord( work ), 
	    SI().zRange( work ).start, SI().zRange( work ).stop, true );
    gamecontroller_->newPosAvailable.notify(
	    mCB(this, uiBouncyMgr, newPosAvailableCB));
    
    bouncydisp_->newEvent.notify( mCB(this, uiBouncyMgr, neweventCB) );
    bouncydisp_->start();
}


void uiBouncyMgr::stopGame()
{
    gamecontroller_->stop();
    gamecontroller_->newPosAvailable.remove(
	    mCB(this, uiBouncyMgr, newPosAvailableCB));
	
    bouncydisp_->newEvent.remove( mCB(this, uiBouncyMgr, neweventCB) );
}


void uiBouncyMgr::pauseGame( bool pause )
{ 
    gamecontroller_->pause( pause );
}


void uiBouncyMgr::newPosAvailableCB( CallBacker* )
{
    if ( bouncydisp_ )
	bouncydisp_->setBallPosition( gamecontroller_->getPos() );
}


void uiBouncyMgr::propertyChangeCB( CallBacker* )
{
    if ( !bouncydisp_ )
	createBouncy();
    else
    {
	if ( bouncydisp_->getBallProperties() != 
		settingsdlg_->getBallProperties() )
	    bouncydisp_->setBallProperties( 
		    settingsdlg_->getBallProperties() );
    }
}


void uiBouncyMgr::neweventCB( CallBacker* )
{
    if ( !bouncydisp_ )
	return;

    if ( bouncydisp_->isstopped() )
    {
	stopGame();
	pErrMsg( "stopped game" );
    }
    else if ( bouncydisp_->ispaused() )
    {
	pauseGame( bouncydisp_->ispaused() );
	pErrMsg( " pause/resume" );
    }
}


void uiBouncyMgr::sessionSaveCB( CallBacker* )
{
}


void uiBouncyMgr::sessionRestoreCB( CallBacker *)
{
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
