/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpeman.cc,v 1.52 2005-10-06 20:22:49 cvskris Exp $
________________________________________________________________________

-*/

#include "uimpeman.h"

#include "attribsel.h"
#include "attribslice.h"
#include "cubicbeziercurve.h"
#include "draw.h"
#include "emhistory.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "emseedpicker.h"
#include "emsurfacegeometry.h"
#include "emtracker.h"
#include "mpeengine.h"
#include "oddirs.h"
#include "pixmap.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uicursor.h"
#include "uiexecutor.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uislider.h"
#include "uispinbox.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"
#include "viscolortab.h"
#include "visdataman.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vismpe.h"
#include "vismpeseedcatcher.h"
#include "vispicksetdisplay.h"
#include "visplanedatadisplay.h"
#include "visseedstickeditor.h"
#include "visselman.h"
#include "vissurvscene.h"
#include "vistexture3.h"
#include "vistransform.h"

using namespace MPE;


#define mAddButton(pm,func,tip,toggle) \
    addButton( ioPixmap( GetDataFileName(pm) ), \
	    	    mCB(this,uiMPEMan,func), tip, toggle )

uiMPEMan::uiMPEMan( uiParent* p, uiVisPartServer* ps )
    : uiToolBar(p,"Tracking controls")
    , clickcatcher(0)
    , visserv(ps)
    , init(false)
    , createmnuitem("Create")
    , seedpicker( 0 )
    , colbardlg( 0 )
    , seedclickobject( -1 )
    , blockattribsel( false )
{
    seedidx = mAddButton( "seedpickmode.png", seedModeCB, "Create seed", true );
    addSeparator();
    
    clrtabidx = mAddButton( "colorbar.png", setColorbarCB,
			    "Set track plane colorbar", true );
    moveplaneidx = mAddButton( "moveplane.png", movePlaneCB,
			       "Move track plane", true );
    extendidx = mAddButton( "trackplane.png", extendModeCB,
	    		    "Extend mode", true );
    retrackidx = mAddButton( "retrackplane.png", retrackModeCB, 
	    		     "ReTrack mode", true );
    eraseidx = mAddButton( "erasingplane.png", eraseModeCB, 
	    		   "Erase mode", true );
/*
    addSeparator();
    mouseeraseridx = mAddButton( "eraser.png", mouseEraseModeCB, 
	    			 "Erase with mouse", true );
*/

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
    trackinvolidx = mAddButton( "track_seed.png", trackInVolume,
    				"Auto-tracking", false );
    
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
    //engine().seedpropertychange.notify(
	    		//mCB(this,uiMPEMan,seedPropertyChangeCB) );
    visBase::DM().selMan().selnotifier.notify( mCB(this,uiMPEMan,selChangeCB) );
    visBase::DM().selMan().deselnotifier.notify(mCB(this,uiMPEMan,selChangeCB));
}


uiMPEMan::~uiMPEMan()
{
    EM::EMM().history().changenotifier.remove(
	    		mCB(this,uiMPEMan,updateButtonSensitivity) );
    deleteVisObjects();
    //engine().seedpropertychange.remove(
	    		//mCB(this,uiMPEMan,seedPropertyChangeCB) );
    visBase::DM().selMan().selnotifier.remove( mCB(this,uiMPEMan,selChangeCB) );
    visBase::DM().selMan().deselnotifier.remove(mCB(this,uiMPEMan,selChangeCB));
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

    if ( clickcatcher )
    {
	if ( scenes.size() )
	    visserv->removeObject( clickcatcher->id(), scenes[0] );

	clickcatcher->click.remove(mCB(this,uiMPEMan,seedClick));
	clickcatcher->unRef();
	clickcatcher = 0;
    }
}


void uiMPEMan::seedClick( CallBacker* )
{
    MPE::Engine& engine = MPE::engine();
    MPE::EMTracker* tracker = engine.getTracker(engine.highestTrackerID());
    if ( !tracker )
	return;

    EM::EMObject* emobject = EM::EMM().getObject(tracker->objectID());
    mDynamicCastGet( EM::Horizon*, horizon, emobject );
    if ( !horizon )
	return;

    if ( clickcatcher->ctrlClickedNode().objectID()!=-1 )
    {
	const EM::PosID pid = clickcatcher->ctrlClickedNode();
	if ( !seedpicker || !seedpicker->canRemoveSeed() ||
	     pid.objectID()!=horizon->id() )
	    return;

	seedpicker->removeSeed(pid);
	return;
    }

    const int clickedobject = clickcatcher->clickedObjectID();
    if ( clickedobject==-1 )
	return;


    if ( seedclickobject==-1 )
    {
	didtriggervolchange = false;
	seedpicker = tracker->getSeedPicker(true);
	if ( !seedpicker || !seedpicker->canSetSectionID() ||
	     !seedpicker->setSectionID(horizon->sectionID(0)) ||
	     !seedpicker->startSeedPick() )
	{
	    seedpicker = 0;
	    return;
	}

	const bool haddefaultvol =
	    engine.activeVolume()==engine.getDefaultActiveVolume();

	CubeSampling newvolume = clickcatcher->clickedObjectCS();
	if ( !haddefaultvol )
	    newvolume.limitTo(engine.activeVolume());

	if ( newvolume.isEmpty() )
	    return;

	oldactivevol = engine.activeVolume();
	NotifyStopper notifystopper( engine.activevolumechange );
	engine.setActiveVolume(newvolume);
	notifystopper.restore();
	seedclickobject = clickedobject;

	if ( haddefaultvol )
	{
	    const Attrib::SliceSet* cached =
		clickcatcher->clickedObjectData();

	    if ( cached )
	    {
		cached->ref();
		engine.setAttribData( *clickcatcher->clicedObjectDataSelSpec(),
				      cached );
	    }

	    didtriggervolchange = true;
	    engine.activevolumechange.trigger();
	}
    }
    else if ( clickedobject!=seedclickobject )
	return;


    if ( !seedpicker )
	return;

    const Coord3 disppos =
	visSurvey::SPM().getZScaleTransform()->
		transformBack(clickcatcher->clickedPos());
    const Coord3 pos =
	visSurvey::SPM().getUTM2DisplayTransform()->transformBack(disppos);

    uiCursor::setOverride( uiCursor::Wait );
    seedpicker->addSeed( pos );
    uiCursor::restoreOverride();
}


visSurvey::MPEDisplay* uiMPEMan::getDisplay( int sceneid, bool create )
{
    mDynamicCastGet(const visSurvey::Scene*,scene,visserv->getObject(sceneid));
    if ( !scene ) return 0;

    TypeSet<int> displayids;
    visserv->findObject( typeid(visSurvey::MPEDisplay), displayids );

    for ( int idx=0; idx<displayids.size(); idx++ )
    {
	if ( scene->getFirstIdx(displayids[idx]) == -1 )
	    continue;

	visBase::DataObject* dobj = visserv->getObject( displayids[idx] );
	return reinterpret_cast<visSurvey::MPEDisplay*>( dobj );
    }

    if ( !create ) return 0;

    visSurvey::MPEDisplay* mpedisplay = visSurvey::MPEDisplay::create();

    visserv->addObject( mpedisplay, scene->id(), false );
    mpedisplay->setDraggerTransparency( transfld->getValue() );
    mpedisplay->showDragger( isOn(extendidx) );

    mpedisplay->deSelection()->notify( mCB(this,uiMPEMan,cubeDeselCB) );
    mpedisplay->selection()->notify( mCB(this,uiMPEMan,cubeSelectCB) );

    setSensitive( true );
    return mpedisplay;
}


#define mGetDisplays(create) \
    ObjectSet<visSurvey::MPEDisplay> displays; \
    TypeSet<int> scenes; \
    visserv->getChildIds( -1, scenes ); \
    for ( int idx=0; idx<scenes.size(); idx++ ) \
	displays += getDisplay( scenes[idx], create );


void uiMPEMan::updateAttribNames()
{
    BufferString oldsel = attribfld->text();
    attribfld->empty();
    attribfld->addItem( "No attribute" );

    ObjectSet<const Attrib::SelSpec> attribspecs;
    engine().getNeededAttribs( attribspecs );
    for ( int idx=0; idx<attribspecs.size(); idx++ )
    {
	const Attrib::SelSpec* spec = attribspecs[idx];
	attribfld->addItem( spec->userRef() );
    }

    mGetDisplays(false)
    for ( int idx=0; idx<displays.size(); idx++ )
        attribfld->setCurrentItem( displays[idx]->getSelSpecUserRef() );

    if ( !init && attribfld->size()>1 && attribspecs.size() &&
	 engine().getAttribCache(*attribspecs[0]) )
    {
	init = true;
	turnOn( extendidx, true );
	engine().setTrackMode( TrackPlane::Extend );
	showTracker( true );
	attribfld->setCurrentItem( (int)1 );
    }

    attribSel(0);
}


void uiMPEMan::turnSeedPickingOn( bool yn )
{
    blockattribsel = yn;

    if ( seedpicker )
    {
	seedpicker->stopSeedPick();
	seedpicker = 0;
    }

    turnOn( seedidx, yn );
    if ( yn )
    {
	seedclickobject = -1;
	if ( !clickcatcher )
	{
	    clickcatcher = visSurvey::MPEClickCatcher::create();
	    clickcatcher->ref();

	    TypeSet<int> scenes;
	    visserv->getChildIds( -1, scenes );
	    visserv->addObject( clickcatcher, scenes[0], false );
	    clickcatcher->click.notify(mCB(this,uiMPEMan,seedClick));
	}

	clickcatcher->turnOn(true);
	clickcatcher->select();
	oldactivevol.setEmpty();

    }
    else
    {
	if ( clickcatcher ) clickcatcher->turnOn(false);
	if ( !oldactivevol.isEmpty() )
	{
	    //Restore old volume if it has been changed.
	    NotifyStopper notifystopper( MPE::engine().activevolumechange );
	    MPE::engine().setActiveVolume(oldactivevol);
	    notifystopper.restore();

	    if ( didtriggervolchange )
		MPE::engine().activevolumechange.trigger();
	}

	clickcatcher->deSelect();
    }
}


void uiMPEMan::selChangeCB(CallBacker*)
{
    bool hastracker = false;
    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size()==1 )
    {
	const MultiID mid = visserv->getMultiID(selectedids[0]);
	if ( !(mid==-1) )
	{
	    const EM::ObjectID oid = EM::EMM().getObjectID(mid);
	    const int trackerid = MPE::engine().getTrackerByObject(oid);
	    if ( trackerid!=-1 ) hastracker = true;
	}
    }

    
    setSensitive( seedidx, hastracker );
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
    if ( blockattribsel )
	return;

    uiCursorChanger cursorchanger( uiCursor::Wait );

    mGetDisplays(false)
    if ( !attribfld->currentItem() )
    {
	for ( int idx=0; idx<displays.size(); idx++ )
	{
	    displays[idx]->setSelSpec( Attrib::SelSpec() );
	    displays[idx]->updateTexture();
	}

	return;
    }

    ObjectSet<const Attrib::SelSpec> attribspecs;
    engine().getNeededAttribs( attribspecs );
    for ( int idx=0; idx<attribspecs.size(); idx++ )
    {
	const Attrib::SelSpec* spec = attribspecs[idx];
	if ( strcmp(spec->userRef(),attribfld->text()) )
	    continue;

	for ( int idy=0; idy<displays.size(); idy++ )
	{
	    displays[idy]->setSelSpec( *spec );
	    displays[idy]->updateTexture();
	}
	break;
    }	

    if ( colbardlg && displays.size() )
    {
	const int coltabid = displays[0]->getTexture()
	    ?  displays[0]->getTexture()->getColorTab().id()
	    : -1;

	colbardlg->setColTab( coltabid );
    }
}


void uiMPEMan::transpChg( CallBacker* )
{
    mGetDisplays(false)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->setDraggerTransparency( transfld->getValue() );
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
    if ( isOn( seedidx ) )
	visserv->sendAddSeedEvent();
}


void uiMPEMan::undoPush( CallBacker* )
{
    if ( !EM::EMM().history().unDo( 1, mEMHistoryUserInteractionLevel ) )
	uiMSG().error("Could not undo everything.");

    updateButtonSensitivity(0);
}


void uiMPEMan::redoPush( CallBacker* )
{
    if ( !EM::EMM().history().reDo( 1, mEMHistoryUserInteractionLevel) )
	uiMSG().error("Could not redo everything.");

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
    const int currentevent = EM::EMM().history().currentEventNr();
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->moveMPEPlane( nrsteps );
    setHistoryLevel(currentevent);
}


void uiMPEMan::trackBackward( CallBacker* )
{
    uiCursorChanger cursorlock( uiCursor::Wait );
    const int nrsteps = nrstepsbox->getValue();
    mGetDisplays(false)
    const int currentevent = EM::EMM().history().currentEventNr();
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->moveMPEPlane( -nrsteps );
    setHistoryLevel(currentevent);
}


void uiMPEMan::trackInVolume( CallBacker* )
{
    const bool ison = isOn( seedidx );
    const TrackPlane::TrackMode tm = engine().trackPlane().getTrackMode();
    engine().setTrackMode(TrackPlane::Extend);
    setTrackButton();
    
    uiCursor::setOverride( uiCursor::Wait );
    PtrMan<Executor> exec = engine().trackInVolume();
    if ( exec )
    {
	/*
	const char* msg = "Press \"Save\" for next step.\n"
			  "Press \"Don't Save\" to continue automaticly\n"
			  "Press \"Cancel\" to cancel ";
	while ( int res=uiMSG().notSaved(msg)>=0 )
	{
	    int execres;
	    while ( (execres=exec->doStep()) > 0 )
	    {
		EM::History& history = EM::EMM().history();
		const int currentevent = history.currentEventNr();
		history.setLevel(currentevent,mEMHistoryUserInteractionLevel);

		if ( res!=1 )
		    break;
	    }

	    if ( execres<1 )
		break;

	}
	*/
	const int currentevent = EM::EMM().history().currentEventNr();
	uiExecutor uiexec( this, *exec );
	if ( !uiexec.go() )
	{
	    uiMSG().error(engine().errMsg());
	}
	setHistoryLevel(currentevent);
    }

    uiCursor::restoreOverride();
    engine().setTrackMode(tm);
    setTrackButton();
}


void uiMPEMan::setTrackButton()
{
    const TrackPlane::TrackMode tm = engine().trackPlane().getTrackMode();
    const bool extend = tm==TrackPlane::Extend;
    const bool retrack = tm==TrackPlane::ReTrack;
    const bool erase = tm==TrackPlane::Erase;
    const bool move = tm==TrackPlane::Move;
    turnOn( extendidx, extend );
    turnOn( retrackidx, retrack );
    turnOn( eraseidx, erase );
    turnOn( moveplaneidx, move );
}


void uiMPEMan::showTracker( bool yn )
{
    setSensitive( trackinvolidx,
	    engine().trackPlane().getTrackMode()==TrackPlane::Extend );

    setSensitive( trackforwardidx, yn );
    setSensitive( trackbackwardidx, yn );
    attribfld->setSensitive( yn );
    transfld->setSensitive( yn );
    nrstepsbox->setSensitive( yn );
    setTrackButton();

    mGetDisplays(true)
    setSensitive( clrtabidx, displays.size() > 0 );
    for ( int idx=0; idx<displays.size(); idx++ )
    {
	displays[idx]->showDragger( yn );
	if ( yn ) displays[idx]->updatePlaneColor();
    }
}


void uiMPEMan::setColorbarCB(CallBacker*)
{
    if ( colbardlg || !isOn(clrtabidx) )
	return;

    setSensitive( clrtabidx, false );
    mGetDisplays(false);

    if ( displays.size() < 1 )	return;

    const int coltabid = displays[0]->getTexture()
	?  displays[0]->getTexture()->getColorTab().id()
	: -1;

    colbardlg = new uiColorBarDialog( this, coltabid, "Track plane colorbar" );
    colbardlg->winClosing.notify( mCB( this,uiMPEMan,onColTabClosing ) );
    colbardlg->go();
}


void uiMPEMan::onColTabClosing( CallBacker* )
{
    setSensitive( clrtabidx, true );
    turnOn( clrtabidx, false );
    colbardlg = 0;
}


void uiMPEMan::movePlaneCB( CallBacker* )
{
    const bool ison = isOn( moveplaneidx );
    engine().setTrackMode( ison ? TrackPlane::Move : TrackPlane::None );
    showTracker( ison );
}


void uiMPEMan::extendModeCB( CallBacker* )
{
    const bool ison = isOn( extendidx );
    engine().setTrackMode( ison ? TrackPlane::Extend : TrackPlane::None );
    showTracker( ison );
}


void uiMPEMan::retrackModeCB( CallBacker* )
{
    const bool ison = isOn( retrackidx );
    engine().setTrackMode( ison ? TrackPlane::ReTrack : TrackPlane::None );
    showTracker( ison );
}


void uiMPEMan::eraseModeCB( CallBacker* )
{
    const bool ison = isOn( eraseidx );
    engine().setTrackMode( ison ? TrackPlane::Erase : TrackPlane::None );
    showTracker( ison );
}


void uiMPEMan::initFromDisplay()
{
    mGetDisplays(false)
    for ( int idx=0; idx<displays.size(); idx++ )
    {
	displays[idx]->deSelection()->notify( mCB(this,uiMPEMan,cubeDeselCB) );
	displays[idx]->selection()->notify( mCB(this,uiMPEMan,cubeSelectCB) );

	if ( idx==0 )
	{
	    transfld->setValue( displays[idx]->getDraggerTransparency() );
	    turnOn( extendidx, displays[idx]->isDraggerShown() );
	}
    }
}


void uiMPEMan::setHistoryLevel(int preveventnr)
{
    EM::History& history = EM::EMM().history();
    const int currentevent = history.currentEventNr();
    if ( currentevent != preveventnr )
	history.setLevel(currentevent,mEMHistoryUserInteractionLevel);
}
