/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpeman.cc,v 1.7 2005-03-09 16:44:38 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uimpeman.h"

#include "attribsel.h"
#include "emmanager.h"
#include "emhistory.h"
#include "mpeengine.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uicursor.h"
#include "uimenu.h"
#include "uislider.h"
#include "uispinbox.h"
#include "vismaterial.h"
#include "visselman.h"
#include "vistexture3.h"
#include "vissurvscene.h"
#include "visseedstickeditor.h"
#include "vismpe.h"
#include "uivispartserv.h"
#include "uivismenu.h"
#include "pixmap.h"


#define mAddButton(pm,func,tip,toggle) \
    addButton( ioPixmap( GetDataFileName(pm) ), \
	    	    mCB(this,uiMPEMan,func), tip, toggle )

uiMPEMan::uiMPEMan( uiParent* p, uiVisPartServer* ps )
    : uiToolBar(p,"Tracking controls")
    , seededitor(0)
    , visserv(ps)
{
    seedidx = mAddButton( "seedpickmode.png", seedModeCB, "Create seed", true );
    addSeparator();
    extendidx = mAddButton( "trackplane.png", extendModeCB,
	    		    "Extend mode", true );
    retrackidx = mAddButton( "retrackplane.png", retrackModeCB, 
	    		     "ReTrack mode", true );
    eraseidx = mAddButton( "erasingplane.png", eraseModeCB, 
	    		   "Erase mode", true );
    addSeparator();
    mouseeraseridx = mAddButton( "eraser.png", mouseEraseModeCB, 
	    			 "Erase with mouse", true );

    addSeparator();
    undoidx = mAddButton( "undo.png", undoPush, "Undo", false );
    redoidx = mAddButton( "redo.png", redoPush, "Redo", false );
    updateButtonSensitivity();

    addSeparator();
    showcubeidx = mAddButton( "trackcube.png", showCubeCB,
	    		      "Show trackingarea", true );

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
    trackforwardidx = mAddButton( "leftarrow.png", trackBackward,
	    			  "Track backward", false );
    trackbackwardidx = mAddButton( "rightarrow.png", trackForward,
	    			   "Track forward", false );

    nrstepsbox = new uiSpinBox( this );
    nrstepsbox->setToolTip( "Nr of tracking steps" );
    nrstepsbox->setMinValue( 1 );

    setCloseMode( 2 );
    setResizeEnabled();
    setVerticallyStretchable(false);
    //setSensitive( false );
    updateAttribNames();

    EM::EMM().history().changenotifier.notify(
	    		mCB(this,uiMPEMan,updateButtonSensitivity) );
    MPE::engine().seedpropertychange.notify(
	    		mCB(this,uiMPEMan,seedPropertyChangeCB) );
}


uiMPEMan::~uiMPEMan()
{
    EM::EMM().history().changenotifier.remove(
	    		mCB(this,uiMPEMan,updateButtonSensitivity) );
    deleteVisObjects();
    MPE::engine().seedpropertychange.remove(
	    		mCB(this,uiMPEMan,seedPropertyChangeCB) );
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
	uiVisMenu* menu = visserv->getMenu( seededitor->id(), false );
	if ( menu )
	{
	    menu->createnotifier.remove( mCB(this,uiMPEMan,createSeedMenuCB) );
	    menu->handlenotifier.remove( mCB(this,uiMPEMan,handleSeedMenuCB) );
	}

	if ( scenes.size() )
	    visserv->removeObject( seededitor->id(), scenes[0] );

	seededitor->unRef();
    }
}


visSurvey::MPEDisplay* uiMPEMan::getDisplay( int sceneid, bool create )
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
    mpedisplay->showDragger( isOn(extendidx) );

    //mpedisplay->trackmodechange.notify( mCB(this,uiMPEMan,trackModeChg) );
    mpedisplay->deSelection()->notify( mCB(this,uiMPEMan,cubeDeselCB) );
    mpedisplay->selection()->notify( mCB(this,uiMPEMan,cubeSelectCB) );

    transfld->setValue( mpedisplay->getDraggerTransparency() );
    trackModeChg( 0 );
    setSensitive( true );
    return mpedisplay;
}

#define mGetDisplays(create) \
    ObjectSet<visSurvey::MPEDisplay> displays; \
    TypeSet<int> scenes; \
    visserv->getChildIds( -1, scenes ); \
    for ( int idx=0; idx<scenes.size(); idx++ ) \
	displays += getDisplay( scenes[idx], create );


void uiMPEMan::seedPropertyChangeCB( CallBacker* )
{
    if ( !seededitor )
	return;

    seededitor->getMaterial()->setColor( MPE::engine().seedcolor );
    seededitor->setSeedSize( MPE::engine().seedsize );
}


void uiMPEMan::createSeedMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiVisMenu*,menu,cb);

    const int seedidx = seededitor->getSeedIdx( *menu->getPath() );
    bool isconnected = seededitor->isClosed( seedidx );

    uiMenuItem* conmenu = new uiMenuItem( isconnected ? "Split" : "Connect" );
    seedmnuid = menu->addItem( conmenu );
    conmenu->setEnabled( seedidx!=-1 );


    BufferStringSet trackernames;
    MPE::engine().getAvaliableTrackerTypes( trackernames );
    for ( int idx=0; idx<trackernames.size(); idx++ )
    {
	BufferString txt("Create "); txt += *trackernames[idx];
	menu->addItem( new uiMenuItem(txt) );
    }
}


void uiMPEMan::handleSeedMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(uiVisMenu*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid==seedmnuid )
    {
	const int seedidx = seededitor->getSeedIdx( *menu->getPath() );
	bool isconnected = seededitor->isClosed( seedidx );

	seededitor->closeSeed( seedidx, !isconnected );
	menu->setIsHandled( true );
	return;
    }

    BufferStringSet trackernames;
    MPE::engine().getAvaliableTrackerTypes( trackernames );
    const int idx = mnuid-seedmnuid-1;
    if ( idx<0 || idx>=trackernames.size() )
	return;

    destrackertype = *trackernames[idx];
    seededitor->getSeeds( MPE::engine().interactionseeds );
    if ( visserv->sendTrackNewObjectEvent() )
	turnSeedPickingOn( false );

    menu->setIsHandled( true );
}


void uiMPEMan::updateAttribNames()
{
    BufferString oldsel = attribfld->text();
    attribfld->empty();
    attribfld->addItem( "No attribute" );

    ObjectSet<const AttribSelSpec> attribspecs;
    MPE::engine().getNeededAttribs( attribspecs );
    for ( int idx=0; idx<attribspecs.size(); idx++ )
    {
	const AttribSelSpec* spec = attribspecs[idx];
	attribfld->addItem( spec->userRef() );
    }

    mGetDisplays(false)
    for ( int idx=0; idx<displays.size(); idx++ )
        attribfld->setCurrentItem( displays[idx]->getSelSpec()->userRef() );

    attribSel(0);
}


void uiMPEMan::turnSeedPickingOn( bool yn )
{
    turnOn( seedidx, yn );
    seedModeCB(0);
}

#define mSelCBImpl( sel ) \
    mGetDisplays(false) \
    for ( int idx=0; idx<displays.size(); idx++ ) \
	displays[idx]->showManipulator( sel ); \
    turnOn( showcubeidx, sel )

void uiMPEMan::cubeSelectCB( CallBacker* )	{ mSelCBImpl( true ); }
void uiMPEMan::cubeDeselCB( CallBacker* )	{ mSelCBImpl( false ); }


void uiMPEMan::showCubeCB( CallBacker* )
{
    const bool isshown = isOn( showcubeidx );
    mGetDisplays(isshown)
    for ( int idx=0; idx<displays.size(); idx++ )
    {
	if ( isshown )
	    displays[idx]->select();
	else
	    displays[idx]->deSelect();
    }

    setToolTip( showcubeidx, isshown ? "Show trackingarea"
	    			     : "Hide trackingarea" );
}


void uiMPEMan::attribSel( CallBacker* )
{
    uiCursorChanger cursorchanger( uiCursor::Wait );

    mGetDisplays(false)
    if ( !attribfld->currentItem() )
    {
	for ( int idx=0; idx<displays.size(); idx++ )
	{
	    displays[idx]->setSelSpec( AttribSelSpec() );
	    displays[idx]->updateTexture();
	}

	return;
    }

    ObjectSet<const AttribSelSpec> attribspecs;
    MPE::engine().getNeededAttribs( attribspecs );
    for ( int idx=0; idx<attribspecs.size(); idx++ )
    {
	const AttribSelSpec* spec = attribspecs[idx];
	if ( strcmp(spec->userRef(),attribfld->text()) )
	    continue;

	for ( int idx=0; idx<displays.size(); idx++ )
	{
	    displays[idx]->setSelSpec( *spec );
	    displays[idx]->updateTexture();
	}
	break;
    }
}


void uiMPEMan::transpChg( CallBacker* )
{
    mGetDisplays(false)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->setDraggerTransparency( transfld->getValue() );
}


void uiMPEMan::undoPush( CallBacker* )
{
    EM::EMM().history().unDo( 1, mEMHistoryUserInteractionLevel );
    updateButtonSensitivity(0);
}


void uiMPEMan::mouseEraseModeCB( CallBacker* )
{
    mGetDisplays(false)
    /*
    if ( !mped ) return;
    const bool ison = isOn( mouseeraseridx );
    if ( ison )
    {
	setSensitive( extendidx, false );
	setSensitive( retrackidx, false );
	setSensitive( eraseidx, false );

	trackerwasonbeforemouseerase = mped->isOn();

	showTracker( false, (int)mped->getTrackMode() );

	ioBitmap bitmap(GetDataFileName("eraserbitmap.png"));
	ioBitmap bitmapmask(GetDataFileName("eraserbitmapmask.png"));
	uiCursor::setOverride(&bitmap,&bitmapmask,2,25);
    }
    else
    {
	setSensitive( extendidx, true );
	setSensitive( retrackidx, true );
	setSensitive( eraseidx, true );

	showTracker( trackerwasonbeforemouseerase, (int)mped->getTrackMode() );
	uiCursor::restoreOverride();
    }

    TypeSet<int> surfvisids;
    visBase::DM().getIds( typeid(visSurvey::SurfaceDisplay), surfvisids );

    for ( int idx=0; idx<surfvisids.size(); idx++ )
    {
	mDynamicCastGet( visSurvey::SurfaceDisplay*, sd,
		visBase::DM().getObject(surfvisids[idx]));
	if ( !sd->getEditor() ) continue;

	sd->getEditor()->setEraseWithMouse(ison);
    }
    */
}


void uiMPEMan::seedModeCB( CallBacker* )
{
    const bool ison = isOn( seedidx );
    if ( !ison && seededitor )
    {
	seededitor->turnOn( false );
	seededitor->getSeeds( MPE::engine().interactionseeds );
	return;
    }

    if ( ison )
    {
	if ( seededitor )
	    seededitor->reset();
	else
	{
	    seededitor = visSurvey::SeedEditor::create();
	    seededitor->ref();

	    TypeSet<int> scenes;
	    visserv->getChildIds( -1, scenes );
	    visserv->addObject( seededitor, scenes[0], false );

	    uiVisMenu* menu = visserv->getMenu( seededitor->id(), true );
	    menu->createnotifier.notify( mCB(this,uiMPEMan,createSeedMenuCB) );
	    menu->handlenotifier.notify( mCB(this,uiMPEMan,handleSeedMenuCB) );
	}

	seededitor->turnOn(true);
	seededitor->select();
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
    uiCursorChanger cursorlock( uiCursor::Wait );
    const int nrsteps = nrstepsbox->getValue();
    mGetDisplays(false)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->moveMPEPlane( nrsteps );
}


void uiMPEMan::trackBackward( CallBacker* )
{
    uiCursorChanger cursorlock( uiCursor::Wait );
    const int nrsteps = nrstepsbox->getValue();
    mGetDisplays(false)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->moveMPEPlane( -nrsteps );
}


void uiMPEMan::trackModeChg( CallBacker* )
{
    mGetDisplays(false);
    if ( !displays.size() ) return;

    visSurvey::MPEDisplay::TrackMode tm = displays[0]->getTrackMode();
    bool extend = tm == visSurvey::MPEDisplay::Extend;
    bool retrack = tm == visSurvey::MPEDisplay::ReTrack;
    bool erase = tm == visSurvey::MPEDisplay::Erode;
    setTrackButton( extend, retrack, erase );
}


void uiMPEMan::setTrackButton( bool extend, bool retrack, bool erase )
{
    turnOn( extendidx, extend );
    turnOn( retrackidx, retrack );
    turnOn( eraseidx, erase );
}


void uiMPEMan::showTracker( bool yn, int tm )
{
    setSensitive( showcubeidx, yn );
    setSensitive( trackforwardidx, yn );
    setSensitive( trackbackwardidx, yn );
    attribfld->setSensitive( yn );
    transfld->setSensitive( yn );
    nrstepsbox->setSensitive( yn );

    visSurvey::MPEDisplay::TrackMode trackmode =
				(visSurvey::MPEDisplay::TrackMode)tm;

    setTrackButton( yn && trackmode==visSurvey::MPEDisplay::Extend, 
	    	    yn && trackmode==visSurvey::MPEDisplay::ReTrack,
	    	    yn && trackmode==visSurvey::MPEDisplay::Erode );

    mGetDisplays(false)
    for ( int idx=0; idx<displays.size(); idx++ )
    {
	displays[idx]->turnOn( yn );
	if ( yn )
	    displays[idx]->setTrackMode( trackmode );
    }
}


void uiMPEMan::extendModeCB( CallBacker* )
{
    const bool isshown = isOn( extendidx );
    mGetDisplays(isshown)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->showDragger( isshown );
}


void uiMPEMan::retrackModeCB( CallBacker* )
{
    mGetDisplays(false)
    if ( !displays.size() ) return;

    const bool turnon = displays[0]->isOn() ||
		displays[0]->getTrackMode()!=visSurvey::MPEDisplay::ReTrack;
    showTracker( turnon, visSurvey::MPEDisplay::ReTrack );
}


void uiMPEMan::eraseModeCB( CallBacker* )
{
    mGetDisplays(false)
    if ( !displays.size() ) return;

    const bool turnon = displays[0]->isOn() ||
		displays[0]->getTrackMode()!=visSurvey::MPEDisplay::Erode;
    showTracker( turnon, visSurvey::MPEDisplay::Erode );
}
