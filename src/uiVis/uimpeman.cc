/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpeman.cc,v 1.77 2006-03-15 13:19:48 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uimpeman.h"

#include "attribsel.h"
#include "attribslice.h"
#include "cubicbeziercurve.h"
#include "draw.h"
#include "emhistory.h"
#include "emobject.h"
#include "emmanager.h"
#include "emseedpicker.h"
#include "emsurfacegeometry.h"
#include "emsurfacetr.h"
#include "emtracker.h"
#include "horizonseedpicker.h"
#include "mpeengine.h"
#include "oddirs.h"
#include "pixmap.h"
#include "survinfo.h"
#include "keystrs.h"
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
#include "vistransmgr.h"

using namespace MPE;

#define mAddButton(pm,func,tip,toggle) \
    addButton( ioPixmap( GetIconFileName(pm) ), \
	    	    mCB(this,uiMPEMan,func), tip, toggle )

#define mGetDisplays(create) \
    ObjectSet<visSurvey::MPEDisplay> displays; \
    TypeSet<int> scenes; \
    visserv->getChildIds( -1, scenes ); \
    for ( int idx=0; idx<scenes.size(); idx++ ) \
	displays += getDisplay( scenes[idx], create );



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
    , didtriggervolchange( false )
    , seedpickwason( false )
{
    seedmodefld = new uiComboBox( this, "Seed mode" );
    seedmodefld->setToolTip( "Select seed mode" );
    seedmodefld->selectionChanged.notify( mCB(this,uiMPEMan,seedModeSel) );

    seedidx = mAddButton( "seedpickmode.png", addSeedCB, "Create seed", true );
    addSeparator();
    
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

    addSeparator();
    showcubeidx = mAddButton( "trackcube.png", showCubeCB,
	    		      "Show trackingarea", true );

    addSeparator();
    attribfld = new uiComboBox( this, "Attribute" );
    attribfld->setToolTip( "QC Attribute" );
    attribfld->selectionChanged.notify( mCB(this,uiMPEMan,attribSel) );
    setStretchableWidget( attribfld );

    clrtabidx = mAddButton( "colorbar.png", setColorbarCB,
			    "Set track plane colorbar", true );

    addSeparator();
    transfld = new uiSlider( this, "Slider", 2 );
    transfld->setOrientation( uiSlider::Horizontal );
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
    updateAttribNames();

    EM::EMM().history().changenotifier.notify(
	    		mCB(this,uiMPEMan,updateButtonSensitivity) );
    engine().trackplanechange.notify(
	    		mCB(this,uiMPEMan,updateButtonSensitivity) );
    visBase::DM().selMan().selnotifier.notify(
	    mCB(this,uiMPEMan,updateButtonSensitivity) );
    visBase::DM().selMan().deselnotifier.notify(
	    mCB(this,uiMPEMan,updateButtonSensitivity));

    updateButtonSensitivity();
}


uiMPEMan::~uiMPEMan()
{
    EM::EMM().history().changenotifier.remove(
	    		mCB(this,uiMPEMan,updateButtonSensitivity) );
    deleteVisObjects();
    engine().trackplanechange.remove(
	    		mCB(this,uiMPEMan,updateButtonSensitivity) );
    visBase::DM().selMan().selnotifier.remove(
	    mCB(this,uiMPEMan,updateButtonSensitivity) );
    visBase::DM().selMan().deselnotifier.remove(
	    mCB(this,uiMPEMan,updateButtonSensitivity) );
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
	    mped->boxDraggerStatusChange.remove(
		mCB(this,uiMPEMan,boxDraggerStatusChangeCB) );
	    visserv->removeObject(mped->id(),scenes[idx]);
	}
    }

    if ( clickcatcher )
    {
	if ( scenes.size() )
	    visserv->removeObject( clickcatcher->id(), scenes[0] );

	clickcatcher->click.remove( mCB(this,uiMPEMan,seedClick) );
	clickcatcher->unRef();
	clickcatcher = 0;
    }

    init = false;
}


void uiMPEMan::seedClick( CallBacker* )
{
    MPE::Engine& engine = MPE::engine();
    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker )
	return;

    const EM::EMObject* emobject = EM::EMM().getObject( tracker->objectID() );
    if ( !emobject )
	return;

    if ( clickcatcher && clickcatcher->ctrlClickedNode().objectID()!=-1 )
    {
	const EM::PosID pid = clickcatcher->ctrlClickedNode();
	if ( seedpicker && seedpicker->canRemoveSeed() && 
					     pid.objectID()==emobject->id() )
	    seedpicker->removeSeed( pid );
	return;
    }

    const int clickedobject = clickcatcher ? 
			      clickcatcher->clickedObjectID() : -1 ;
    if ( clickedobject==-1 )
	return;

    if ( seedclickobject==-1 )
    {
	didtriggervolchange = false;
	seedpicker = tracker->getSeedPicker(true);
	if ( !seedpicker || !seedpicker->canSetSectionID() ||
	     !seedpicker->setSectionID(emobject->sectionID(0)) ||
	     !seedpicker->startSeedPick() )
	{
	    seedpicker = 0;
	    return;
	}
	oldactivevol = engine.activeVolume(); 
    }
    
    const CubeSampling newvolume = clickcatcher->clickedObjectCS();
    if ( newvolume.isEmpty() )
	return;
    
    if ( newvolume != engine.activeVolume() )
    {
	NotifyStopper notifystopper( engine.activevolumechange );
	engine.setActiveVolume( newvolume );
	notifystopper.restore();
	seedclickobject = clickedobject;

	RefMan<const Attrib::DataCubes> cached = 
	    				clickcatcher->clickedObjectData();

	if ( cached )
	{
	    cached->ref();
	    engine.setAttribData( *clickcatcher->clicedObjectDataSelSpec(),
				  cached );
	}

	didtriggervolchange = true;

	mGetDisplays( false );
	for ( int idx=0; idx<displays.size(); idx++ )
	    displays[idx]->turnOn( false );

	engine.activevolumechange.trigger();
    }

    if ( !seedpicker )
	return;
    
    visSurvey::Scene* scene = visSurvey::STM().currentScene();
    const Coord3 disppos = scene->getZScaleTransform()->
		transformBack( clickcatcher->clickedPos() );
    const Coord3 pos = scene->getUTM2DisplayTransform()->transformBack(disppos);

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

    mpedisplay->boxDraggerStatusChange.notify(
	    mCB(this,uiMPEMan,boxDraggerStatusChangeCB) );

    return mpedisplay;
}


void uiMPEMan::updateAttribNames()
{
    BufferString oldsel = attribfld->text();
    attribfld->empty();
    attribfld->addItem( sKeyNoAttrib() );

    ObjectSet<const Attrib::SelSpec> attribspecs;
    engine().getNeededAttribs( attribspecs );
    for ( int idx=0; idx<attribspecs.size(); idx++ )
    {
	const Attrib::SelSpec* spec = attribspecs[idx];
	attribfld->addItem( spec->userRef() );
    }
    attribfld->setCurrentItem( oldsel );

    updateSelectedAttrib();

    if ( !init && attribfld->size()>1 && attribspecs.size() &&
	 engine().getAttribCache(*attribspecs[0]) )
    {
	MPE::EMTracker* tracker = getSelectedTracker();
	seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
	if ( !seedpicker || seedpicker->isInVolumeMode() )
	{
	    init = true;
	    engine().setTrackMode( TrackPlane::Extend );
	    showTracker( true );
	    attribfld->setCurrentItem( (int)1 );
	}
    }

    updateButtonSensitivity(0);
}


void uiMPEMan::updateSelectedAttrib()
{
    mGetDisplays(false);

    if ( !displays.size() )
	return;

    ObjectSet<const Attrib::SelSpec> attribspecs;
    engine().getNeededAttribs( attribspecs );

    const char* userref = displays[0]->getSelSpecUserRef();
    if ( !userref && attribspecs.size() )
    {
	for ( int idx=0; idx<displays.size(); idx++ )
	    displays[idx]->setSelSpec( 0, *attribspecs[0] );

	userref = displays[0]->getSelSpecUserRef();
    }
    else if ( userref==sKey::None )
	userref = sKeyNoAttrib();
    
    if ( userref ) 	
	attribfld->setCurrentItem( userref );
}


void uiMPEMan::turnSeedPickingOn( bool yn )
{
    blockattribsel = yn;

/*    if ( seedpicker )
    {
	seedpicker->stopSeedPick();
	seedpicker = 0;
    }
*/
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

	clickcatcher->turnOn( true );
	oldactivevol.setEmpty();
    }
    else
    {
	if ( clickcatcher ) clickcatcher->turnOn( false );
	if ( !oldactivevol.isEmpty() )
	{
	    //Restore old volume if it has been changed.
	    NotifyStopper notifystopper( MPE::engine().activevolumechange );
	    MPE::engine().setActiveVolume(oldactivevol);
	    notifystopper.restore();

	    if ( didtriggervolchange )
	    {
		mGetDisplays( false );
		for ( int idx=0; idx<displays.size(); idx++ )
		    displays[idx]->turnOn( true );

		MPE::engine().activevolumechange.trigger();
	    }

	    didtriggervolchange = false;
	}
    }
}


void uiMPEMan::boxDraggerStatusChangeCB( CallBacker* cb )
{
    mGetDisplays(false);
    bool ison = false;
    for ( int idx=0; idx<displays.size(); idx++ )
    {
	if ( cb!=displays[idx] )
	    continue;

	ison = displays[idx]->isBoxDraggerShown();
	if ( !ison )
	    displays[idx]->updateMPEActiveVolume();
    }

    turnOn( showcubeidx, ison );
}


void uiMPEMan::showCubeCB( CallBacker* )
{
    const bool isshown = isOn( showcubeidx );
    mGetDisplays(isshown)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->showBoxDragger( isshown );

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
	    Attrib::SelSpec spec( 0, Attrib::SelSpec::cNoAttrib() );
	    displays[idx]->setSelSpec( 0, spec );
	    displays[idx]->updateTexture();
	}
    }
    else
    {
	ObjectSet<const Attrib::SelSpec> attribspecs;
	engine().getNeededAttribs( attribspecs );
	for ( int idx=0; idx<attribspecs.size(); idx++ )
	{
	    const Attrib::SelSpec* spec = attribspecs[idx];
	    if ( strcmp(spec->userRef(),attribfld->text()) )
		continue;

	    for ( int idy=0; idy<displays.size(); idy++ )
	    {
		displays[idy]->setSelSpec( 0, *spec );
		displays[idy]->updateTexture();
	    }
	    break;
	}	
    }

    if ( colbardlg && displays.size() )
    {
	const int coltabid =
	    displays[0]->getTexture() && displays[0]->getTexture()->isOn()
		?  displays[0]->getTexture()->getColorTab().id()
		: -1;

	colbardlg->setColTab( coltabid );
    }

    updateButtonSensitivity();
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

	ioBitmap bitmap(GetIconFileName("eraserbitmap.png"));
	ioBitmap bitmapmask(GetIconFileName("eraserbitmapmask.png"));
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


void uiMPEMan::addSeedCB( CallBacker* )
{
    turnSeedPickingOn( isOn(seedidx) );

//    if ( isOn( seedidx ) )
//	visserv->sendAddSeedEvent();
}

void uiMPEMan::seedModeSel( CallBacker* )
{
    if ( seedpicker )
    {
	seedpicker->setSeedMode( seedmodefld->currentItem() ); 
    }
    updateButtonSensitivity(0);
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


MPE::EMTracker* uiMPEMan::getSelectedTracker() const
{
    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size()!=1 )
	return 0;
    const MultiID mid = visserv->getMultiID(selectedids[0]);
    if ( mid==-1 )
	return 0;
    const EM::ObjectID oid = EM::EMM().getObjectID(mid);
    const int trackerid = MPE::engine().getTrackerByObject(oid);
    if ( trackerid==-1 )
	return 0;
    return MPE::engine().getTracker( trackerid );
}


void uiMPEMan::updateButtonSensitivity( CallBacker* ) 
{
    //Undo/Redo
    uiToolBar::setSensitive( undoidx, EM::EMM().history().canUnDo() );
    uiToolBar::setSensitive( redoidx, EM::EMM().history().canReDo() );

    //Seed button

    updateSeedModeState();

    const bool isvoltrack = seedpicker ? seedpicker->isInVolumeMode() : true; 
    
    uiToolBar::setSensitive( undoidx, isvoltrack );
    uiToolBar::setSensitive( redoidx, isvoltrack );
    uiToolBar::setSensitive( extendidx, isvoltrack );
    uiToolBar::setSensitive( retrackidx, isvoltrack );
    uiToolBar::setSensitive( eraseidx, isvoltrack );
    uiToolBar::setSensitive( moveplaneidx, isvoltrack );
    uiToolBar::setSensitive( showcubeidx, isvoltrack );
    uiToolBar::setSensitive( trackinvolidx, isvoltrack );
    
    //Track forward, backward, attrib, trans, nrstep
    mGetDisplays(false);
    const bool trackerisshown = displays.size() &&
				displays[0]->isDraggerShown();

    uiToolBar::setSensitive( trackforwardidx, trackerisshown && isvoltrack );
    uiToolBar::setSensitive( trackbackwardidx, trackerisshown && isvoltrack );
    attribfld->setSensitive( trackerisshown && isvoltrack );
    transfld->setSensitive( trackerisshown && isvoltrack );
    nrstepsbox->setSensitive( trackerisshown && isvoltrack );

    //coltab
    uiToolBar::setSensitive( clrtabidx, trackerisshown && !colbardlg &&
			     attribfld->currentItem()>0 && isvoltrack );


    //trackmode buttons
    const TrackPlane::TrackMode tm = engine().trackPlane().getTrackMode();
    turnOn( extendidx, trackerisshown && tm==TrackPlane::Extend );
    turnOn( retrackidx, trackerisshown && tm==TrackPlane::ReTrack );
    turnOn( eraseidx, trackerisshown && tm==TrackPlane::Erase );
    turnOn( moveplaneidx, trackerisshown && tm==TrackPlane::Move );
}


void uiMPEMan::updateSeedModeState()
{
    seedmodefld->empty();

    MPE::EMTracker* tracker = getSelectedTracker();
    uiToolBar::setSensitive( seedidx, tracker );
    seedmodefld->setSensitive( tracker );

    if ( !tracker )
    {
	seedmodefld->addItem("No seed mode");
	if ( isOn(seedidx) )
	{
	    turnSeedPickingOn( false );
	    seedpickwason = true;
	}
	return;
    }
    if ( isOn(seedidx) )
	seedpickwason = false;
    if ( seedpickwason )
	turnSeedPickingOn( true );

    const EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    if ( emobj && emobj->getTypeStr() == EMHorizonTranslatorGroup::keyword )
    {
	// items should be in order of MPE::HorizonSeedPicker::SeedModeOrder
	seedmodefld->addItem("Volume track");
	seedmodefld->addItem("Line tracking");
	seedmodefld->addItem("Line manual");
    }

    seedpicker = tracker->getSeedPicker(true);
    seedmodefld->setCurrentItem( seedpicker->getSeedMode() );
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
    mGetDisplays(false);
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->updateMPEActiveVolume();

    const TrackPlane::TrackMode tm = engine().trackPlane().getTrackMode();
    engine().setTrackMode(TrackPlane::Extend);
    updateButtonSensitivity();
   
    NotifyStopper selstopper( EM::EMM().history().changenotifier );
    uiCursor::setOverride( uiCursor::Wait );
    PtrMan<Executor> exec = engine().trackInVolume();
    if ( exec )
    {
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
    updateButtonSensitivity();
}


void uiMPEMan::showTracker( bool yn )
{
    if ( didtriggervolchange )
	yn = false;

    mGetDisplays(true)
    for ( int idx=0; idx<displays.size(); idx++ )
	displays[idx]->showDragger( yn );
}


void uiMPEMan::setColorbarCB(CallBacker*)
{
    if ( colbardlg || !isOn(clrtabidx) )
	return;

    mGetDisplays(false);

    if ( displays.size()<1 )
	return;

    const int coltabid = displays[0]->getTexture()
	?  displays[0]->getTexture()->getColorTab().id()
	: -1;

    colbardlg = new uiColorBarDialog( this, coltabid, "Track plane colorbar" );
    colbardlg->winClosing.notify( mCB( this,uiMPEMan,onColTabClosing ) );
    colbardlg->go();

    updateButtonSensitivity();
}


void uiMPEMan::onColTabClosing( CallBacker* )
{
    turnOn( clrtabidx, false );
    colbardlg = 0;

    updateButtonSensitivity();
}


void uiMPEMan::movePlaneCB( CallBacker* )
{
    const bool ison = isOn( moveplaneidx );
    showTracker( ison );
    engine().setTrackMode( ison ? TrackPlane::Move : TrackPlane::None );
}


void uiMPEMan::extendModeCB( CallBacker* )
{
    const bool ison = isOn( extendidx );
    showTracker( ison );
    engine().setTrackMode( ison ? TrackPlane::Extend : TrackPlane::None );
}


void uiMPEMan::retrackModeCB( CallBacker* )
{
    const bool ison = isOn( retrackidx );
    showTracker( ison );
    engine().setTrackMode( ison ? TrackPlane::ReTrack : TrackPlane::None );
}


void uiMPEMan::eraseModeCB( CallBacker* )
{
    const bool ison = isOn( eraseidx );
    showTracker( ison );
    engine().setTrackMode( ison ? TrackPlane::Erase : TrackPlane::None );
}


void uiMPEMan::initFromDisplay()
{
    mGetDisplays(false)
    for ( int idx=0; idx<displays.size(); idx++ )
    {
	displays[idx]->boxDraggerStatusChange.notify(
		mCB(this,uiMPEMan,boxDraggerStatusChangeCB) );

	if ( idx==0 )
	{
	    transfld->setValue( displays[idx]->getDraggerTransparency() );
	}
    }

    updateSelectedAttrib();
    updateButtonSensitivity(0);
}


void uiMPEMan::setHistoryLevel(int preveventnr)
{
    EM::History& history = EM::EMM().history();
    const int currentevent = history.currentEventNr();
    if ( currentevent != preveventnr )
	history.setLevel(currentevent,mEMHistoryUserInteractionLevel);
}
