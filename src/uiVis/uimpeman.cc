/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpeman.cc,v 1.4 2005-01-21 13:09:38 kristofer Exp $
________________________________________________________________________

-*/

#include "uimpeman.h"

#include "attribsel.h"
#include "emmanager.h"
#include "emhistory.h"
#include "pixmap.h"
#include "uicombobox.h"
#include "uicursor.h"
#include "uislider.h"
#include "uispinbox.h"
#include "uibutton.h"
#include "visinterpret.h"
#include "vismaterial.h"
#include "visselman.h"
#include "visdataman.h"
#include "vissurvsurf.h"
#include "visseedstickeditor.h"
#include "vissurvsurfeditor.h"
#include "vissurvscene.h"
#include "vistexture3.h"
#include "vismpe.h"
#include "uicursor.h"
#include "uivispartserv.h"
#include "uivismenu.h"
#include "mpeengine.h"
#include "uimenu.h"


#define SID visSurvey::SurfaceInterpreterDisplay

#define mAddButton(pm,func,tip,toggle) \
    addButton( ioPixmap( GetDataFileName(pm) ), \
	    	    mCB(this,uiMPEMan,func), tip, toggle )


uiMPEMan::uiMPEMan( uiParent* p, uiVisPartServer* partserv_ )
    : uiToolBar(p,"Tracking controls")
    , seededitor(0)
    , visserv( partserv_ )
{
    seedidx = mAddButton( "trackplane.png", seedMode, "Create seed", true );
    addSeparator();
    extendidx = mAddButton( "trackplane.png", extendModeButtonPushCB,
	    		    "Extend mode", true );
    retrackidx = mAddButton( "retrackplane.png", retrackMode, 
	    		     "ReTrack mode", true );
    eraseidx = mAddButton( "erasingplane.png", eraseMode, "Erase mode", true );
    addSeparator();
    mouseeraseridx = mAddButton( "eraser.png", mouseEraseMode, 
	    			 "Erase with mouse", true );

    addSeparator();
    undoidx = mAddButton( "undo.png", undoPush, "Undo", false );
    redoidx = mAddButton( "redo.png", redoPush, "Redo", false );
    updateButtonSensitivity();

    addSeparator();
    showcubeidx = mAddButton("trackcube.png",showCubeButtonPushCB,
	    		     "Show trackingarea",true);

    addSeparator();
    attribfld = new uiComboBox( this, "Attribute" );
    attribfld->setToolTip( "QC Attribute" );
    attribfld->selectionChanged.notify( mCB(this,uiMPEMan,attribSel) );
    setStretchableWidget( attribfld );

    addSeparator();
    transfld = new uiSlider( this, "Slider", 2 );
    transfld->setMaxValue( 1 );
    transfld->setToolTip( "Transparency" );
    transfld->valueChanged.notify( mCB(this,uiMPEMan,transpChg) );

    addSeparator();
    trackforwardidx = mAddButton("leftarrow.png",trackBackward,
	    			 "Track backward",false);
    trackbackwardidx = mAddButton("rightarrow.png",trackForward,
	    			  "Track forward",false);

    nrstepsbox = new uiSpinBox( this );
    nrstepsbox->setToolTip( "Nr of tracking steps" );
    nrstepsbox->setMinValue( 1 );

    setCloseMode( 2 );
    setResizeEnabled();
    setVerticallyStretchable(false);
    //setSensitive( false );

    EM::EMM().history().changenotifier.notify(
	    		mCB(this,uiMPEMan,updateButtonSensitivity));

    MPE::engine().seedpropertychange.notify(
	    		mCB(this,uiMPEMan,seedPropertyChangeCB ));
}


uiMPEMan::~uiMPEMan()
{
    //if ( trackman )
	//trackman->newdatacb.remove( mCB(this,uiMPEMan,reloadData) );
    EM::EMM().history().changenotifier.remove(
	    		mCB(this,uiMPEMan,updateButtonSensitivity));
    deleteVisObjects();
    MPE::engine().seedpropertychange.remove(
	    		mCB(this,uiMPEMan,seedPropertyChangeCB ));
}


void uiMPEMan::deleteVisObjects()
{
    TypeSet<int> scenes;
    visserv->getChildIds( -1, scenes );

    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	visSurvey::MPEDisplay* mped = getDisplay(scenes[idx]);
	if ( mped )
	{
	    mped->deSelection()->remove( mCB(this,uiMPEMan,cubeDeselCB) );
	    mped->selection()->remove( mCB(this,uiMPEMan,cubeSelectCB) );
	    visserv->removeObject(mped->id(),scenes[idx]);
	}
    }

    if ( seededitor )
    {
	uiVisMenu* menu = visserv->getMenu(seededitor->id(), false );
	if ( menu )
	{
	    menu->createnotifier.remove(
		    mCB( this, uiMPEMan, createSeedMenuCB ));
	    menu->handlenotifier.remove(
		    mCB( this, uiMPEMan, handleSeedMenuCB ));
	}

	if ( scenes.size() )
	    visserv->removeObject( seededitor->id(), scenes[0] );
	seededitor->unRef();

    }

}


visSurvey::MPEDisplay* uiMPEMan::getDisplay(int sceneid, bool create)
{
    mDynamicCastGet(const visSurvey::Scene*,scene,visserv->getObject(sceneid));
    if ( !scene ) return 0;

    TypeSet<int> displayids;
    visserv->findObject( typeid(visSurvey::MPEDisplay), displayids );

    for ( int idx=0; idx<displayids.size(); idx++ )
    {
	if ( scene->getFirstIdx(displayids[idx])!=-1 )
	    return reinterpret_cast<visSurvey::MPEDisplay*>(
		    visserv->getObject(displayids[idx]) );
    }

    if ( !create ) return 0;

    visSurvey::MPEDisplay* mpedisplay = visSurvey::MPEDisplay::create();

    visserv->addObject( mpedisplay, scene->id(), false );
    mpedisplay->setDraggerTransparency( transfld->getValue() );
    mpedisplay->showDragger(isOn(extendidx));

    //mpedisplay->trackmodechange.notify( mCB(this,uiMPEMan,trackModeChg) );
    mpedisplay->deSelection()->notify( mCB(this,uiMPEMan,cubeDeselCB) );
    mpedisplay->selection()->notify( mCB(this,uiMPEMan,cubeSelectCB) );

    //transfld->setValue( mpedisplay->getTransparency() );
    trackModeChg( 0 );
    setSensitive( true );
    return mpedisplay;
}

void uiMPEMan::seedPropertyChangeCB(CallBacker*)
{
    if ( !seededitor )
	return;

    seededitor->getMaterial()->setColor(MPE::engine().seedcolor);
    seededitor->setSeedSize(MPE::engine().seedsize);
}


void uiMPEMan::createSeedMenuCB(CallBacker* cb)
{
    mDynamicCastGet(uiVisMenu*,menu,cb);

    const int seedidx = seededitor->getSeedIdx(*menu->getPath());
    bool isconnected = seededitor->isClosed(seedidx);

    uiMenuItem* conmenu = new uiMenuItem( isconnected ? "Split" : "Connect" );
    seedmnuid = menu->addItem( conmenu );
    conmenu->setEnabled(seedidx!=-1);


    BufferStringSet trackernames;
    MPE::engine().getAvaliableTrackerTypes(trackernames);
    for ( int idx=0; idx<trackernames.size(); idx++ )
    {
	BufferString txt("Create "); txt += *trackernames[idx];
	menu->addItem( new uiMenuItem((const char*)txt) );
    }
}


void uiMPEMan::handleSeedMenuCB(CallBacker* cb)
{
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(uiVisMenu*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid==seedmnuid )
    {
	const int seedidx = seededitor->getSeedIdx(*menu->getPath());
	bool isconnected = seededitor->isClosed(seedidx);

	seededitor->closeSeed(seedidx,!isconnected);
	menu->setIsHandled(true);
	return;
    }

    BufferStringSet trackernames;
    MPE::engine().getAvaliableTrackerTypes(trackernames);
    const int idx=mnuid-seedmnuid-1;
    if ( idx<0 || idx>=trackernames.size() )
	return;

    destrackertype = *trackernames[idx];
    seededitor->getSeeds( MPE::engine().interactionseeds );
    if ( visserv->sendTrackNewObjectEvent() )
	turnSeedPickingOn(false);

    menu->setIsHandled(true);
}


void uiMPEMan::updateAttribNames()
{
    /*
    if ( !trackman ) return;
    BufferString oldsel = attribfld->text();
    attribfld->empty();
    attribfld->addItem( "No attribute" );
    for ( int idx=0; idx<trackman->getAttribSpecs().size(); idx++ )
    {
	AttribSelSpec* spec = trackman->getAttribSpecs()[idx];
	attribfld->addItem( spec->userRef() );
    }

    if ( mpedisplay )
	attribfld->setCurrentItem( mpedisplay->getSelSpec()->userRef() );

    attribSel(0);
    */
}


void uiMPEMan::turnSeedPickingOn(bool yn)
{
    turnOn( seedidx, yn);
    seedMode(0);
}

#define mSelCBImpl( sel ) \
    TypeSet<int> scenes; \
    visserv->getChildIds( -1, scenes ); \
    for ( int idx=0; idx<scenes.size(); idx++  ) \
    { \
	visSurvey::MPEDisplay* mped = getDisplay(scenes[idx],false); \
	if ( mped ) mped->showManipulator( sel ); \
    } \
    turnOn( showcubeidx, sel )

void uiMPEMan::cubeSelectCB( CallBacker* ) { mSelCBImpl( true ); }


void uiMPEMan::cubeDeselCB( CallBacker* ) { mSelCBImpl( false ); }


void uiMPEMan::showCubeButtonPushCB( CallBacker* cb )
{
    const bool isshown = isOn( showcubeidx );

    TypeSet<int> scenes;
    visserv->getChildIds( -1, scenes );

    for ( int idx=0; idx<scenes.size(); idx++  )
    {
	visSurvey::MPEDisplay* mped = getDisplay(scenes[idx],isshown);
	if ( mped )
	{
	    if ( isshown ) mped->select();
	    else mped->deSelect();
	}
    }

    setToolTip( showcubeidx, isshown ? "Show trackingarea":"Hide trackingarea");
}


void uiMPEMan::attribSel( CallBacker* )
{
    /*
    if ( !interpreter ) return;
    const int sel = attribfld->currentItem();
    if ( !sel )
    {
	interpreter->setTexture( 0 );
	interpreter->setSelSpec( AttribSelSpec() );
	return;
    }

    uiCursorChanger cursorchanger( uiCursor::Wait );

    for ( int idx=0; idx<trackman->getAttribSpecs().size(); idx++ )
    {
	const AttribSelSpec* spec = trackman->getAttribSpecs()[idx];
	if ( strcmp(spec->userRef(),attribfld->text()) )
	    continue;

	interpreter->setSelSpec( *spec );
	interpreter->updateTexture(0);
	break;
    }
    */
}


void uiMPEMan::transpChg( CallBacker* )
{
    TypeSet<int> scenes;
    visserv->getChildIds( -1, scenes );

    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	visSurvey::MPEDisplay* mped = getDisplay(scenes[idx]);
	if ( mped ) { mped->setDraggerTransparency( transfld->getValue() ); }
    }
}


void uiMPEMan::undoPush( CallBacker* )
{
    EM::EMM().history().unDo( 1, mEMHistoryUserInteractionLevel );
    updateButtonSensitivity(0);
}


void uiMPEMan::mouseEraseMode( CallBacker* )
{
    /*
    if ( !interpreter ) return;
    const bool ison = isOn( mouseeraseridx );
    if ( ison )
    {
	setSensitive( extendidx, false );
	setSensitive( retrackidx, false );
	setSensitive( eraseidx, false );

	trackerwasonbeforemouseerase = interpreter->isOn();

	showTracker( false, (int) interpreter->getTrackMode() );

	ioBitmap bitmap(GetDataFileName("eraserbitmap.png"));
	ioBitmap bitmapmask(GetDataFileName("eraserbitmapmask.png"));
	uiCursor::setOverride(&bitmap,&bitmapmask,2,25);
    }
    else
    {
	setSensitive( extendidx, true );
	setSensitive( retrackidx, true );
	setSensitive( eraseidx, true );

	showTracker( trackerwasonbeforemouseerase,
		     (int) interpreter->getTrackMode() );

	uiCursor::restoreOverride();
    }

    TypeSet<int> surfvisids;
    visBase::DM().getIds( typeid(visSurvey::SurfaceDisplay), surfvisids );

    for ( int idx=0; idx<surfvisids.size(); idx++ )
    {
	mDynamicCastGet( visSurvey::SurfaceDisplay*, sd,
		visBase::DM().getObj(surfvisids[idx]));
	if ( !sd->getEditor() ) continue;

	sd->getEditor()->setEraseWithMouse(ison);
    }
    */
}


void uiMPEMan::seedMode( CallBacker* )
{
    const bool ison = isOn( seedidx );
    if ( ison )
    {
	if ( !seededitor ) 
	{
	    seededitor = visSurvey::SeedEditor::create();
	    seededitor->ref();

	    TypeSet<int> scenes;
	    visserv->getChildIds( -1, scenes );
	    visserv->addObject( seededitor, scenes[0], false );

	    uiVisMenu* menu = visserv->getMenu(seededitor->id(), true );
	    menu->createnotifier.notify(
		    mCB( this, uiMPEMan, createSeedMenuCB ));
	    menu->handlenotifier.notify(
		    mCB( this, uiMPEMan, handleSeedMenuCB ));

	}
	else
	{
	    seededitor->reset();
	}

	seededitor->turnOn(true);
	seededitor->select();
    }
    else
    {
	if ( seededitor )
	{
	    seededitor->turnOn(false);
	    seededitor->getSeeds( MPE::engine().interactionseeds );
	}
    }
}


void uiMPEMan::redoPush( CallBacker* )
{
    EM::EMM().history().reDo( 1, mEMHistoryUserInteractionLevel );
    updateButtonSensitivity(0);
}


void uiMPEMan::updateButtonSensitivity( CallBacker* ) 
{
    setSensitive( undoidx, EM::EMM().history().canUnDo() );
    setSensitive( redoidx, EM::EMM().history().canReDo() );
}


void uiMPEMan::trackForward( CallBacker* )
{
    /*
    if ( !interpreter ) return;
    int nrsteps = nrstepsbox->getValue();
    uiCursorChanger cursorlock( uiCursor::Wait );
    interpreter->moveMPEPlane( nrsteps );
    */
}


void uiMPEMan::trackBackward( CallBacker* )
{
    /*
    if ( !interpreter ) return;
    int nrsteps = nrstepsbox->getValue();
    uiCursorChanger cursorlock( uiCursor::Wait );
    interpreter->moveMPEPlane( -nrsteps );
    */
}


void uiMPEMan::trackModeChg( CallBacker* )
{
    /*
    if ( !interpreter ) return;
    SID::TrackMode tm = interpreter->getTrackMode();
    bool extend = tm == SID::Extend;
    bool retrack = tm == SID::ReTrack;
    bool erase = tm == SID::Erode;
    setTrackButton( extend, retrack, erase );
    */
}


void uiMPEMan::setTrackButton( bool extend, bool retrack, bool erase )
{
    turnOn( extendidx, extend );
    turnOn( retrackidx, retrack );
    turnOn( eraseidx, erase );
}


void uiMPEMan::showTracker( bool yn, int trackmode )
{
    /*
    setSensitive( showcubeidx, yn );
    setSensitive( trackforwardidx, yn );
    setSensitive( trackbackwardidx, yn );
    attribfld->setSensitive(yn);
    transfld->setSensitive(yn);
    nrstepsbox->setSensitive(yn);

    interpreter->turnOn( yn );
    setTrackButton( yn&&trackmode==SID::Extend, yn&&trackmode==SID::ReTrack,
	    	    yn&&trackmode==SID::Erode );
    if ( yn )
	interpreter->setTrackMode( (SID::TrackMode) trackmode );
    */
}


void uiMPEMan::extendModeButtonPushCB( CallBacker* )
{
    const bool isshown = isOn(extendidx);

    TypeSet<int> scenes;
    visserv->getChildIds( -1, scenes );

    for ( int idx=0; idx<scenes.size(); idx++  )
    {
	visSurvey::MPEDisplay* mped = getDisplay(scenes[idx],isshown);
	if ( mped ) mped->showDragger(isshown);
    }
}


void uiMPEMan::retrackMode( CallBacker* )
{
    /*
    if ( !interpreter ) return;
    const bool turnon = !interpreter->isOn() ||
			 interpreter->getTrackMode()!=SID::ReTrack; 
    showTracker( turnon, SID::ReTrack );
    */
}


void uiMPEMan::eraseMode( CallBacker* )
{
    /*
    if ( !interpreter ) return;
    const bool turnon = !interpreter->isOn() ||
			 interpreter->getTrackMode()!=SID::Erode; 
    showTracker( turnon, SID::Erode );
    */
}
