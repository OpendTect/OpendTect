/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Nanne Hemstra
 * DATE     : May 2009
___________________________________________________________________

-*/

static const char* rcsID = "$Id: horflatvieweditor.cc,v 1.4 2009-07-20 11:51:36 cvsumesh Exp $";

#include "horflatvieweditor.h"

#include "emobject.h"
#include "emmanager.h"
#include "emseedpicker.h"
#include "emtracker.h"
#include "flatauxdataeditor.h"
#include "flatposdata.h"
#include "emhorizon2d.h"
#include "horizon2dseedpicker.h"
#include "ioman.h"
#include "linesetposinfo.h"
#include "mouseevent.h"
#include "mousecursor.h"
#include "mpeengine.h"
#include "posinfo.h"
#include "seis2dline.h"
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

    CubeSampling oldactivevol = MPE::engine().activeVolume();

    CubeSampling newactivevol;
    if ( is2d_ )
	newactivevol.setEmpty();
    else
	newactivevol = curcs_;

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

    // get EM::PosID
    EM::PosID pid;
    bool posidavlble = false;
    if ( is2d_ )
    {
	PtrMan<IOObj> ioobj = IOM().get( lsetid_ );
	if ( ioobj )
	{
	    const Seis2DLineSet lset(ioobj->fullUserExpr(true));
	    PosInfo::LineSet2DData linesetgeom;
	    if ( lset.getGeometry(linesetgeom) )
	    {
		PosInfo::Line2DPos fltpos;
		linesetgeom.getLineData( linenm_ )->getPos( clickedcrd, fltpos,
							   mDefEps );
		mDynamicCastGet(EM::Horizon2D*,hor2d,emobj);

		if ( hor2d )
		{
		    BinID bid;
		    bid.inl = hor2d->geometry().lineIndex( linenm_ );
		    bid.crl = fltpos.nr_;
		    for ( int idx=0; idx<emobj->nrSections(); idx++ )
			if ( emobj->isDefined(emobj->sectionID(idx),
				    	      bid.getSerialized()) )
			{
			    posidavlble = true;
			    pid.setObjectID( emobj->id() );
			    pid.setSectionID( emobj->sectionID(idx) );
			    pid.setSubID( bid.getSerialized() );
			}
		}
	    }
	}
    }
    else
    {
	BinID bid = SI().transform( clickedcrd );
	for ( int idx=0; idx<emobj->nrSections(); idx++ )
	    if ( emobj->isDefined(emobj->sectionID(idx), bid.getSerialized()) )
	    {
		posidavlble = true;
		pid.setObjectID( emobj->id() );
		pid.setSectionID( emobj->sectionID(idx) );
		pid.setSubID( bid.getSerialized() );
	    }
    }

    const int prevevent = EM::EMM().undo().currentEventID();
    MouseCursorManager::setOverride( MouseCursor::Wait );
    emobj->setBurstAlert( true );

    const int trackerid = MPE::engine().getTrackerByObject( emobj->id() );

    const bool ctrlshiftclicked =  mouseevent.ctrlStatus() && 
				   mouseevent.shiftStatus();
    if ( posidavlble )
    {
	if ( ctrlshiftclicked )
	{
	    if ( seedpicker->removeSeed( pid, false, false ) )
		MPE::engine().updateFlatCubesContainer( newactivevol, trackerid,
						      false );
	}
	else if ( mouseevent.ctrlStatus() )
	{
	    if ( seedpicker->removeSeed( pid, true, true ) )
	       MPE::engine().updateFlatCubesContainer( newactivevol, trackerid,
	       					     false );	    
	}
	else if ( mouseevent.shiftStatus() )
	{
	    if ( seedpicker->removeSeed( pid, true, false ) )
		MPE::engine().updateFlatCubesContainer( newactivevol, trackerid,
						      false );
	}
	else
	{
	    if ( seedpicker->addSeed( clickedcrd, false ) )
		MPE::engine().updateFlatCubesContainer( newactivevol, trackerid,
						      true );
	}
    }
    else
    {
	if ( seedpicker->addSeed(clickedcrd,ctrlshiftclicked) )
	    MPE::engine().updateFlatCubesContainer( newactivevol, trackerid, 
		    				    true );
    }

    emobj->setBurstAlert( false );
    MouseCursorManager::restoreOverride();
    const int currentevent = EM::EMM().undo().currentEventID();
    if ( currentevent != prevevent )
	EM::EMM().undo().setUserInteractionEnd(currentevent);

    //TODO restore active volume for proper condition
}


void HorizonFlatViewEditor::movementEndCB( CallBacker* )
{
}


void HorizonFlatViewEditor::removePosCB( CallBacker* )
{
}


} // namespace MPE
