/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : May 2009
___________________________________________________________________

-*/

static const char* rcsID = "$Id$";

#include "horflatvieweditor.h"

#include "attribstorprovider.h"
#include "emobject.h"
#include "emmanager.h"
#include "emseedpicker.h"
#include "emtracker.h"
#include "flatauxdataeditor.h"
#include "flatposdata.h"
#include "emhorizon2d.h"
#include "horizon2dseedpicker.h"
#include "ioman.h"
#include "ioobj.h"
#include "linesetposinfo.h"
#include "mouseevent.h"
#include "mousecursor.h"
#include "mpeengine.h"
#include "posinfo.h"
#include "sectionadjuster.h"
#include "sectiontracker.h"
#include "seis2dline.h"
#include "survinfo.h"
#include "surv2dgeom.h"
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
    , seedpickingon_(false)
    , updateoldactivevolinuimpeman(this)
    , restoreactivevolinuimpeman(this)
    , updateseedpickingstatus(this)
    , trackersetupactive_(false)
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


void HorizonFlatViewEditor::setSeedPickingStatus( bool yn )
{ seedpickingon_ = yn; } 


void HorizonFlatViewEditor::mouseMoveCB( CallBacker* )
{
}


void HorizonFlatViewEditor::mousePressCB( CallBacker* )
{
}


void HorizonFlatViewEditor::mouseReleaseCB( CallBacker* )
{
    if ( curcs_.isEmpty() || !editor_->viewer().appearance().annot_.editable_
	    || editor_->isSelActive() )
	return;

    updateseedpickingstatus.trigger();
    if ( !seedpickingon_ ) return;

    MPE::EMTracker* tracker = MPE::engine().getActiveTracker();
    if ( !tracker ) return;

    if ( !canTrack(*tracker) ) return;

    EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    if ( !emobj ) return;

    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;
    
    if ( !seedpicker || !seedpicker->canAddSeed() ) return;
    
    if ( !seedpicker->canSetSectionID() || 
	 !seedpicker->setSectionID(emobj->sectionID(0)) )
	return;

    CubeSampling newactivevol;
    if ( is2d_ )
	newactivevol.setEmpty();
    else
	newactivevol = curcs_;

    if ( MPE::engine().activeVolume() != newactivevol )
	updateoldactivevolinuimpeman.trigger();

    bool pickinvd = true;

    if ( !checkSanity(*tracker,*emobj,*seedpicker,pickinvd) )
	return;

    const FlatDataPack* dp = editor_->viewer().pack( !pickinvd );
    if ( !dp )
	{ uiMSG().error( "No data to choose from" ); return; }

    const MouseEvent& mouseevent = mouseeventhandler_->event();
    const uiRect datarect( editor_->getMouseArea() );
    if ( !datarect.isInside(mouseevent.pos()) ) return;

    const uiWorld2Ui w2u( datarect.size(), editor_->getWorldRect(mUdf(int)) );
    const uiWorldPoint wp = w2u.transform( mouseevent.pos()-datarect.topLeft());

    const FlatPosData& pd = dp->posData();
    const IndexInfo ix = pd.indexInfo( true, wp.x );
    const IndexInfo iy = pd.indexInfo( false, wp.y );
    Coord3 clickedcrd = dp->getCoord( ix.nearest_, iy.nearest_ );
    clickedcrd.z = wp.y;

    if ( !prepareTracking(pickinvd,*tracker,*seedpicker,*dp) )
	return;

    const int prevevent = EM::EMM().undo().currentEventID();
    MouseCursorManager::setOverride( MouseCursor::Wait );
    emobj->setBurstAlert( true );

    const int trackerid = MPE::engine().getTrackerByObject( emobj->id() );
    
    bool action = doTheSeed( *emobj, *seedpicker, clickedcrd, mouseevent );
    
    engine().updateFlatCubesContainer( newactivevol, trackerid, action );
    
    emobj->setBurstAlert( false );
    MouseCursorManager::restoreOverride();
    const int currentevent = EM::EMM().undo().currentEventID();
    if ( currentevent != prevevent )
	EM::EMM().undo().setUserInteractionEnd(currentevent);

    restoreactivevolinuimpeman.trigger();
}


bool HorizonFlatViewEditor::canTrack( const EMTracker& tracker ) const
{
    if ( tracker.is2D() && !is2d_ )
    {
	uiMSG().error( "2D tracking cannot handle picks on 3D lines.");
	return false;
    }
    else if ( !tracker.is2D() && is2d_ )
    {
	uiMSG().error( "3D tracking cannot handle picks on 2D lines.");
	return false;
    }

    return true;
}


bool HorizonFlatViewEditor::prepareTracking( bool picinvd,
					     const EMTracker& trker,
					     EMSeedPicker& seedpicker,
					     const FlatDataPack& dp ) const
{
    const Attrib::SelSpec* as = 0;
    as = picinvd ? vdselspec_ : wvaselspec_;
    
    if ( trker.is2D() )
    {
	MPE::engine().setActive2DLine( lsetid_, linenm_ );
	mDynamicCastGet( MPE::Horizon2DSeedPicker*, h2dsp, &seedpicker );
	if ( h2dsp )
	    h2dsp->setSelSpec( as );

	if ( dp.id() > DataPack::cNoID() )
	    MPE::engine().setAttribData( *as, dp.id() );

	if ( !h2dsp || !h2dsp->canAddSeed(*as) )
	    return false;

	h2dsp->setLine( lsetid_, linenm_ );

	if ( !h2dsp->startSeedPick() )
	    return false;
    }
    else
    {
	if ( !seedpicker.startSeedPick() )
	    return false;

	NotifyStopper notifystopper( MPE::engine().activevolumechange );
	MPE::engine().setActiveVolume( curcs_ );
	notifystopper.restore();

	seedpicker.setSelSpec( as );

	MPE::engine().setOneActiveTracker( &trker );
	if ( !MPE::engine().cacheIncludes(*as,curcs_) )
	    if ( dp.id() > DataPack::cNoID() )
		MPE::engine().setAttribData( *as, dp.id() );

	MPE::engine().activevolumechange.trigger();
    }

    return true;
}


bool HorizonFlatViewEditor::checkSanity( EMTracker& tracker,
					const EM::EMObject& emobj,
				        const EMSeedPicker& spk,
				        bool& pickinvd ) const
{
    const Attrib::SelSpec* as = 0;

    const MPE::SectionTracker* sectiontracker =
	tracker.getSectionTracker(emobj.sectionID(0), true);
    const Attrib::SelSpec* trackedatsel = sectiontracker
	? sectiontracker->adjuster()->getAttributeSel(0) :0;

    Attrib::SelSpec newatsel;

    if ( trackedatsel )
    {
	newatsel = *trackedatsel;
	if ( tracker.is2D() &&
		matchString(Attrib::StorageProvider::attribName(),
		    	    trackedatsel->defString()) )
	{
	    LineKey lk( trackedatsel->userRef() );
	    newatsel.setUserRef( lk.attrName().isEmpty() ?
		    LineKey::sKeyDefAttrib() : lk.attrName().buf() );
	}
    }

    if ( spk.nrSeeds() < 1 )
    {
	if ( editor_->viewer().pack(false) && editor_->viewer().pack(true) )
	{
	    if ( !uiMSG().question("Which one is your seed data.",
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
	    return false;
	}

	as = pickinvd ? vdselspec_ : wvaselspec_;
	if ( !trackersetupactive_ && as && trackedatsel && (newatsel!=*as) &&
	      (spk.getSeedConnectMode()!=spk.DrawBetweenSeeds) )
	{
	    uiMSG().error( "Saved setup has different attribute. \n"
		    	   "Either change setup attribute or change\n"
			   "display attribute you want to track on" );
	    return false;
	}
    }
    else
    {
	if ( vdselspec_ && trackedatsel && (newatsel==*vdselspec_) )
	    pickinvd = true;
	else if ( wvaselspec_ && trackedatsel && (newatsel==*wvaselspec_) )
	    pickinvd = false;
	else if ( spk.getSeedConnectMode() !=spk.DrawBetweenSeeds )
	{
	    BufferString warnmsg( "Setup suggests tracking is done on " );
	    if ( vdselspec_ && pickinvd )
		warnmsg += vdselspec_->userRef();
	    else if ( wvaselspec_ && !pickinvd )
		warnmsg += wvaselspec_->userRef();
	    warnmsg += "\n but what you see is ";
	    warnmsg += newatsel.userRef();
	    warnmsg += ".\n\n To continue seeds picking either\n";
	    warnmsg += "change displayed attribute or\n";
	    warnmsg += "change input data in Tracking Setup";

	    uiMSG().error( warnmsg.buf() );
	    return false;
	}
    }

    return true;
}


bool HorizonFlatViewEditor::doTheSeed( EM::EMObject& emobj, EMSeedPicker& spk, 
				       const Coord3& crd, 
				       const MouseEvent& mev ) const
{
    EM::PosID pid;
    bool posidavlble = getPosID( emobj, crd, pid );

    const bool ctrlshiftclicked =  mev.ctrlStatus() && mev.shiftStatus();
    bool addseed = !posidavlble || ( posidavlble && !mev.ctrlStatus() &&
	    			     !mev.shiftStatus() );

    if ( addseed )
    {
	if ( spk.addSeed(crd,posidavlble ? false : ctrlshiftclicked) )
		return true;
    }
    else
    {
	bool env = false;
	bool retrack = false;

	if ( mev.ctrlStatus() )
	{
	    env = true;
	    retrack = true;
	}
	else if ( mev.shiftStatus() )
	    env = true;

	if ( spk.removeSeed(pid,env,retrack) )
	    return false;
    }

    return true;
}


bool HorizonFlatViewEditor::getPosID( const EM::EMObject& emobj,
				      const Coord3& crd, EM::PosID& pid ) const
{
    BinID bid;
    if ( !is2d_ )
	bid = SI().transform( crd );
    else
    {
	PtrMan<IOObj> ioobj = IOM().get( lsetid_ );
	if ( !ioobj ) return false;

	const Seis2DLineSet lset( ioobj->fullUserExpr(true) );
	S2DPOS().setCurLineSet( lset.name() );
	PosInfo::LineSet2DData linesetgeom;
	for ( int idx=0; idx<lset.nrLines(); idx++ )
	{
	    PosInfo::Line2DData& linegeom =
		linesetgeom.addLine(lset.lineName(idx));
	    S2DPOS().getGeometry( linegeom );
	    if ( linegeom.positions().isEmpty() )
	    {
		linesetgeom.removeLine( lset.lineName(idx) );
		return false;
	    }
	}

	PosInfo::Line2DPos pos;
	linesetgeom.getLineData( linenm_ )->getPos( crd, pos, mDefEps );
	mDynamicCastGet(const EM::Horizon2D*,hor2d,&emobj);

	if ( !hor2d ) return false;
	
	bid.inl = hor2d->geometry().lineIndex( linenm_ );
	bid.crl = pos.nr_;
    }

    for ( int idx=0; idx<emobj.nrSections(); idx++ )
    {
	if ( emobj.isDefined(emobj.sectionID(idx),bid.toInt64()) )
	{
	    pid.setObjectID( emobj.id() );
	    pid.setSectionID( emobj.sectionID(idx) );
	    pid.setSubID( bid.toInt64() );
	    return true;
	}
    }

    return false;
}


void HorizonFlatViewEditor::movementEndCB( CallBacker* )
{
}


void HorizonFlatViewEditor::removePosCB( CallBacker* )
{
}

} // namespace MPE
