/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Nanne Hemstra
 * DATE     : May 2009
___________________________________________________________________

-*/

static const char* rcsID = "$Id: horflatvieweditor.cc,v 1.3 2009-07-07 09:05:36 cvsumesh Exp $";

#include "horflatvieweditor.h"

#include "emobject.h"
#include "emmanager.h"
#include "emseedpicker.h"
#include "emtracker.h"
#include "flatauxdataeditor.h"
#include "flatposdata.h"
#include "horizon2dseedpicker.h"
#include "mouseevent.h"
#include "mousecursor.h"
#include "mpeengine.h"
#include "survinfo.h"
#include "undo.h"

#include "uimsg.h"
#include "uiworld2ui.h"


namespace MPE 
{

HorizonFlatViewEditor::HorizonFlatViewEditor( FlatView::AuxDataEditor* ed )
    : editor_(ed)
    , mouseeventhandler_(0)
    , vdselspec_(0)
    , wvaselspec_(0)
    , linenm_(0)
    , lsetid_(-1)
    , is2d_(false)
{
    curcs_.setEmpty();
    editor_->movementFinished.notify(
	    mCB(this,HorizonFlatViewEditor,movementEndCB) );
    editor_->removeSelected.notify(
	    mCB(this,HorizonFlatViewEditor,removePosCB) );
}


HorizonFlatViewEditor::~HorizonFlatViewEditor()
{
    setMouseEventHandler( 0 );
}


void HorizonFlatViewEditor::setCubeSampling( const CubeSampling& cs )
{ curcs_ = cs; }


void HorizonFlatViewEditor::setSelSpec( const Attrib::SelSpec* as, bool wva )
{
    if ( !wva )
	vdselspec_ = as;
    else
	wvaselspec_ = as;
}


void HorizonFlatViewEditor::swapSelSpec()
{
    const Attrib::SelSpec* tempas = 0;
    tempas = vdselspec_;
    vdselspec_ = wvaselspec_;
    wvaselspec_ = tempas;
}


void HorizonFlatViewEditor::setMouseEventHandler( MouseEventHandler* meh )
{
    if ( mouseeventhandler_ )
    {
       mouseeventhandler_->movement.remove(
	       mCB(this,HorizonFlatViewEditor,mouseMoveCB) );
       mouseeventhandler_->buttonPressed.remove(
	       mCB(this,HorizonFlatViewEditor,mousePressCB) );
       mouseeventhandler_->buttonReleased.remove(
	       mCB(this,HorizonFlatViewEditor,mouseReleaseCB) );
    }

    mouseeventhandler_ = meh;

    if ( mouseeventhandler_ )
    {
       mouseeventhandler_->movement.notify(
	       mCB(this,HorizonFlatViewEditor,mouseMoveCB) );
       mouseeventhandler_->buttonPressed.notify(
	       mCB(this,HorizonFlatViewEditor,mousePressCB) );
       mouseeventhandler_->buttonReleased.notify(
	       mCB(this,HorizonFlatViewEditor,mouseReleaseCB) );
    }
}


void HorizonFlatViewEditor::mouseMoveCB( CallBacker* )
{
}


void HorizonFlatViewEditor::mousePressCB( CallBacker* )
{
}


void HorizonFlatViewEditor::mouseReleaseCB( CallBacker* )
{
    if ( curcs_.isEmpty() ) return;

    MPE::EMTracker* tracker = MPE::engine().getActiveTracker();
    if ( !tracker ) return;

    if ( tracker->is2D() && !is2d_ )
    {
	uiMSG().error( "2D tracking cannot handle picks on 3D lines.");
	return;
    }
    else if ( !tracker->is2D() && is2d_ )
    {
	uiMSG().error( "3D tracking cannot handle picks on 2D lines.");
	return;
    }

    EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    if ( !emobj ) return;

    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    if ( !seedpicker || !seedpicker->canAddSeed() )
	return;
    if ( !seedpicker->canSetSectionID() || 
	 !seedpicker->setSectionID(emobj->sectionID(0)) )
	return;

    const MouseEvent& mouseevent = mouseeventhandler_->event();
    const uiRect datarect( editor_->getMouseArea() );
    const uiWorld2Ui w2u( datarect.size(), editor_->getWorldRect(mUdf(int)) );
    const uiWorldPoint wp =
	w2u.transform( mouseevent.pos()-datarect.topLeft() );

    const FlatDataPack* dp = 0;
    const Attrib::SelSpec* as = 0;
    bool pickinvd = true;

    if ( seedpicker->nrSeeds() < 1 )
    {
	if ( editor_->viewer().pack(false) && editor_->viewer().pack(true) )
	{
	    if ( !uiMSG().question("Which one is your seed data",
		       		   "VD", "Wiggle") )
		pickinvd = false;
	}
	else if ( editor_->viewer().pack(false) )
	    pickinvd = true;
	else if ( editor_->viewer().pack(true) )
	    pickinvd = false;
	else
	{
	    uiMSG().error( "No data to choose from" );
	    return;
	}
    }
    else
    {
	if ( vdselspec_ && (*seedpicker->getSelSpec() == *vdselspec_) )
	    pickinvd = true;
	else if ( wvaselspec_ && (*seedpicker->getSelSpec() == *wvaselspec_) )
	    pickinvd = false;
	else
	{
	    uiMSG().error( "Horizon has been tracked on different attribute ");
	    return;
	}	    
    }
    dp = editor_->viewer().pack( !pickinvd );
    if ( !dp )
    {
	uiMSG().error( "No data to choose from" );
	return;
    }
    const FlatPosData& pd = dp->posData();
    const IndexInfo ix = pd.indexInfo( true, wp.x );
    const IndexInfo iy = pd.indexInfo( false, wp.y );
    Coord3 clickedcrd = dp->getCoord( ix.nearest_, iy.nearest_ );
    clickedcrd.z = wp.y;

    if ( pickinvd )
	as = vdselspec_;
    else
	as = wvaselspec_;

    if ( tracker->is2D() )
    {
	MPE::engine().setActive2DLine( lsetid_, linenm_ );
	
	mDynamicCastGet( MPE::Horizon2DSeedPicker*, h2dsp, seedpicker );
	if ( h2dsp )
	    h2dsp->setSelSpec( as );
	if ( dp->id() > DataPack::cNoID() )
	    MPE::engine().setAttribData( *as, dp->id() );

	if ( !h2dsp || !h2dsp->canAddSeed(*as) )
	    return;

	h2dsp->setLine( lsetid_, linenm_ );
	if ( !h2dsp->startSeedPick() )
	    return;
    }
    else
    {
	if ( !seedpicker->startSeedPick() )
	    return;

	NotifyStopper notifystopper( MPE::engine().activevolumechange );
	MPE::engine().setActiveVolume( curcs_ );
	notifystopper.restore();

	seedpicker->setSelSpec( as );
	MPE::engine().setOneActiveTracker( tracker );
	if ( MPE::engine().cacheIncludes(*as,curcs_) )
	    if ( dp->id() > DataPack::cNoID() )
		MPE::engine().setAttribData( *as, dp->id() );

	MPE::engine().activevolumechange.trigger();
    }

    const int prevevent = EM::EMM().undo().currentEventID();
    MouseCursorManager::setOverride( MouseCursor::Wait );
    emobj->setBurstAlert( true );
    seedpicker->addSeed( clickedcrd);
    emobj->setBurstAlert( false );
    MouseCursorManager::restoreOverride();
    const int currentevent = EM::EMM().undo().currentEventID();
    if ( currentevent != prevevent )
	EM::EMM().undo().setUserInteractionEnd(currentevent);
}


void HorizonFlatViewEditor::movementEndCB( CallBacker* )
{
}


void HorizonFlatViewEditor::removePosCB( CallBacker* )
{
}


} // namespace MPE
