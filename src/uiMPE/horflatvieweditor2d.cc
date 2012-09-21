/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "horflatvieweditor2d.h"

#include "attribstorprovider.h"
#include "flatposdata.h"
#include "emhorizon2d.h"
#include "emhorizonpainter2d.h"
#include "emobject.h"
#include "emmanager.h"
#include "emseedpicker.h"
#include "emtracker.h"
#include "flatauxdataeditor.h"
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
#include "surv2dgeom.h"
#include "undo.h"

#include "uimsg.h"
#include "uiworld2ui.h"

namespace MPE
{
    
HorizonFlatViewEditor2D::HorizonFlatViewEditor2D( FlatView::AuxDataEditor* ed,
						  const EM::ObjectID& emid )
    : editor_(ed)
    , emid_(emid)
    , horpainter_( new EM::HorizonPainter2D(ed->viewer(),emid) )
    , mehandler_(0)
    , vdselspec_(0)
    , wvaselspec_(0)
    , linenm_(0)
    , lsetid_(-1)
    , seedpickingon_(false)
    , trackersetupactive_(false)
    , updseedpkingstatus_(this)
{
    curcs_.setEmpty();
    horpainter_->abouttorepaint_.notify(
	    mCB(this,HorizonFlatViewEditor2D,horRepaintATSCB) );
    horpainter_->repaintdone_.notify(
	    mCB(this,HorizonFlatViewEditor2D,horRepaintedCB) );
}


HorizonFlatViewEditor2D::~HorizonFlatViewEditor2D()
{
    if ( mehandler_ )
    {
	editor_->removeSelected.remove(
		mCB(this,HorizonFlatViewEditor2D,removePosCB) );
	mehandler_->movement.remove(
		mCB(this,HorizonFlatViewEditor2D,mouseMoveCB) );
	mehandler_->buttonPressed.remove(
		mCB(this,HorizonFlatViewEditor2D,mousePressCB) );
	mehandler_->buttonReleased.remove(
		mCB(this,HorizonFlatViewEditor2D,mouseReleaseCB) );
    }
//	setMouseEventHandler( 0 );
    cleanAuxInfoContainer();
    delete horpainter_;
    deepErase( markeridinfos_ );
}


void HorizonFlatViewEditor2D::setCubeSampling( const CubeSampling& cs )
{
    curcs_ = cs;
    horpainter_->setCubeSampling( cs ); 
}


void HorizonFlatViewEditor2D::setLineName( const char* lnm )
{
    linenm_ = lnm;
    horpainter_->setLineName( lnm );
}


TypeSet<int>& HorizonFlatViewEditor2D::getPaintingCanvTrcNos()
{
    return horpainter_->getTrcNos();
}


TypeSet<float>& HorizonFlatViewEditor2D::getPaintingCanDistances()
{
    return horpainter_->getDistances();
}


void HorizonFlatViewEditor2D::enableLine( bool yn )
{
    horpainter_->enableLine( yn );
}


void HorizonFlatViewEditor2D::enableSeed( bool yn )
{
    horpainter_->enableSeed( yn );
}


void HorizonFlatViewEditor2D::paint()
{
    horpainter_->paint();
    fillAuxInfoContainer();
}


void HorizonFlatViewEditor2D::setSelSpec( const Attrib::SelSpec* as, bool wva )
{
    if ( !wva )
	vdselspec_ = as;
    else
	wvaselspec_ = as;
}


void HorizonFlatViewEditor2D::setMouseEventHandler( MouseEventHandler* meh )
{
    if ( mehandler_ )
    {
	editor_->removeSelected.remove(
		mCB(this,HorizonFlatViewEditor2D,removePosCB) );
	mehandler_->movement.remove(
		mCB(this,HorizonFlatViewEditor2D,mouseMoveCB) );
	mehandler_->buttonPressed.remove(
		mCB(this,HorizonFlatViewEditor2D,mousePressCB) );
	mehandler_->buttonReleased.remove(
		mCB(this,HorizonFlatViewEditor2D,mouseReleaseCB) );
    }

    mehandler_ = meh;

    if ( mehandler_ )
    {
	editor_->removeSelected.notify(
		mCB(this,HorizonFlatViewEditor2D,removePosCB) );
	mehandler_->movement.notify(
		mCB(this,HorizonFlatViewEditor2D,mouseMoveCB) );
	mehandler_->buttonPressed.notify(
		mCB(this,HorizonFlatViewEditor2D,mousePressCB) );
	mehandler_->buttonReleased.notify(
		mCB(this,HorizonFlatViewEditor2D,mouseReleaseCB) );

	if ( MPE::engine().getTrackerByObject(emid_) != -1 )
	{
	    int trackeridx = MPE::engine().getTrackerByObject( emid_ );
	    
	    if ( MPE::engine().getTracker(trackeridx) )
		MPE::engine().setActiveTracker( emid_ );
	}
	else
	    MPE::engine().setActiveTracker( -1 );
    }

    for ( int idx=0; idx<markeridinfos_.size(); idx++ )
	editor_->enablePolySel( markeridinfos_[idx]->merkerid_, mehandler_ );
}


void HorizonFlatViewEditor2D::setSeedPicking( bool yn )
{ seedpickingon_ = yn; }


void HorizonFlatViewEditor2D::mouseMoveCB( CallBacker* )
{
    const MouseEvent& mouseevent = mehandler_->event();
    if ( editor_ && editor_->sower().accept(mouseevent, false) )
	return;

    if ( MPE::engine().getTrackerByObject(emid_) != -1 )
    {
	int trackeridx = MPE::engine().getTrackerByObject( emid_ );
	if ( MPE::engine().getTracker(trackeridx) )
	    MPE::engine().setActiveTracker( emid_ );
    }
    else
	MPE::engine().setActiveTracker( -1 );
}


void HorizonFlatViewEditor2D::mousePressCB( CallBacker* )
{
    if ( editor_ && editor_->sower().accept(mehandler_->event(), false) )
	return;

    if ( curcs_.isEmpty() || !editor_->viewer().appearance().annot_.editable_
			  || editor_->isSelActive() )
	return;

    //if ( !seedpickingon_ ) return;

    MPE::EMTracker* tracker = MPE::engine().getActiveTracker();
    if ( !tracker || tracker->objectID()!=emid_  || !tracker->is2D() )
	return;

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
    if ( !seedpicker || !seedpicker->canAddSeed() ||
	 !seedpicker->canSetSectionID() ||
	 !seedpicker->setSectionID(emobj->sectionID(0)) )
	return;

    bool pickinvd = true;
    if ( !checkSanity(*tracker,*seedpicker,pickinvd) )
	return;

    const MouseEvent& mouseevent = mehandler_->event();
    const Color& prefcol = emobj->preferredColor();

    if ( editor_ )
    {
	editor_->sower().reInitSettings();
	editor_->sower().intersow();
	editor_->sower().reverseSowingOrder();
	if ( editor_->sower().activate(prefcol, mouseevent) )
	    return;
    }
}


void HorizonFlatViewEditor2D::mouseReleaseCB( CallBacker* )
{
    if ( curcs_.isEmpty() || !editor_->viewer().appearance().annot_.editable_
	    || editor_->isSelActive() )
	return;

    MPE::EMTracker* tracker = MPE::engine().getActiveTracker();
    if ( !tracker ) return;

    if ( tracker->objectID() != emid_ ) return;

    if ( !tracker->is2D() ) return;

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);

    if ( !seedpicker || !seedpicker->canAddSeed() ) return;

    if ( !seedpicker->canSetSectionID() ||
	 !seedpicker->setSectionID(emobj->sectionID(0)) )
	return;

    //if ( !seedpickingon_ ) return;

    bool pickinvd = true;

    if ( !checkSanity(*tracker,*seedpicker,pickinvd) )
	return;

    const MouseEvent& mouseevent = mehandler_->event();

    if ( editor_ )
    {
	const bool sequentsowing = editor_->sower().mode() ==
				   FlatView::Sower::SequentSowing;
	seedpicker->setSowerMode( sequentsowing );
	if ( editor_->sower().accept(mouseevent,true) )
	    return;
    }

    const FlatDataPack* dp = editor_->viewer().pack( !pickinvd );
    if ( !dp ) return;

    const uiRect datarect( editor_->getMouseArea() );
    if ( !datarect.isInside(mouseevent.pos()) ) return;

    const Geom::Point2D<int> mousepos = mouseevent.pos();
    const Geom::Point2D<double>* markerpos = editor_->markerPosAt( mousepos );

    const uiWorld2Ui w2u( datarect.size(), editor_->getWorldRect(mUdf(int)) );
    const uiWorldPoint wp = markerpos ? *markerpos :
				w2u.transform( mousepos-datarect.topLeft());

    const FlatPosData& pd = dp->posData();
    const IndexInfo ix = pd.indexInfo( true, wp.x );
    const IndexInfo iy = pd.indexInfo( false, wp.y );
    Coord3 clickedcrd = dp->getCoord( ix.nearest_, iy.nearest_ );
    clickedcrd.z = wp.y;

    if ( !prepareTracking(pickinvd,*tracker,*seedpicker,*dp) )
	return;

    const int prevevent = EM::EMM().undo().currentEventID();
    MouseCursorManager::setOverride( MouseCursor::Wait );

    doTheSeed( *seedpicker, clickedcrd, mouseevent );
    if ( editor_ && !editor_->sower().moreToSow() )
    {
	emobj->setBurstAlert( true );
	emobj->setBurstAlert( false );
    }

    MouseCursorManager::restoreOverride();

    const int currentevent = EM::EMM().undo().currentEventID();
    if ( currentevent != prevevent )
    {
	if ( !editor_ || !editor_->sower().moreToSow() )
	    EM::EMM().undo().setUserInteractionEnd(currentevent);
    }
}


bool HorizonFlatViewEditor2D::checkSanity( EMTracker& tracker,
					   const EMSeedPicker& spk,
					   bool& pickinvd ) const
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return false;

    const Attrib::SelSpec* atsel = 0;

    const MPE::SectionTracker* sectiontracker =
	tracker.getSectionTracker(emobj->sectionID(0), true);

    const Attrib::SelSpec* trackedatsel = sectiontracker
			? sectiontracker->adjuster()->getAttributeSel(0)
			: 0;

    Attrib::SelSpec newatsel;

    if ( trackedatsel )
    {
	newatsel = *trackedatsel;

	if ( matchString(Attrib::StorageProvider::attribName(),
		    	 trackedatsel->defString()) )
	{
	    LineKey lk( trackedatsel->userRef() );
	    if ( lk.attrName().isEmpty() )
		lk.setAttrName( LineKey::sKeyDefAttrib() );
	    newatsel.setUserRef( lk.attrName().isEmpty()
		    		 ? LineKey::sKeyDefAttrib()
				 : lk.attrName().buf() );
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

	atsel = pickinvd ? vdselspec_ : wvaselspec_;

	if ( !trackersetupactive_ && atsel && trackedatsel &&
	     (newatsel!=*atsel) &&
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
	    BufferString warnmsg( "Setup suggests tracking is done on '" );
	    warnmsg.add( newatsel.userRef() ).add( "'.\n" )
		   .add(  "But what you see is: '" );
	    if ( vdselspec_ && pickinvd )
		warnmsg += vdselspec_->userRef();
	    else if ( wvaselspec_ && !pickinvd )
		warnmsg += wvaselspec_->userRef();
	    warnmsg.add( "'.\n" )
		   .add( "To continue seed picking either " )
		   .add( "change displayed attribute or\n" )
		   .add( "change input data in Tracking Setup." );

	    uiMSG().error( warnmsg.buf() );
	    return false;
	}
    }

    return true;
}


bool HorizonFlatViewEditor2D::prepareTracking( bool picinvd, 
					       const EMTracker& trker,
					       EMSeedPicker& seedpicker,
					       const FlatDataPack& dp ) const
{
    const Attrib::SelSpec* as = 0;
    as = picinvd ? vdselspec_ : wvaselspec_;

    if ( !seedpicker.canAddSeed() )
	return false;

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
    
    return true;
}


bool HorizonFlatViewEditor2D::doTheSeed( EMSeedPicker& spk, const Coord3& crd,
					 const MouseEvent& mev ) const
{
    EM::PosID pid;
    getPosID( crd, pid );

    const bool ctrlshiftclicked = mev.ctrlStatus() && mev.shiftStatus();
    const bool ismarker = editor_->markerPosAt( mev.pos() );

    if ( !ismarker || (ismarker && !mev.ctrlStatus() && !mev.shiftStatus()) )
    {
	const bool drop = ismarker ? false : ctrlshiftclicked;
	if ( spk.addSeed(crd, drop, Coord3(mev.x(),mev.y(),0)) )
	    return true;
    }
    else
    {
	bool env = false;
	bool retrack = false;

	if ( !ctrlshiftclicked )
	{
	    if ( mev.ctrlStatus() )
	    {
		env = true;
		retrack = true;
	    }
	    else if ( mev.shiftStatus() )
		env = true;
	}

	if ( spk.removeSeed(pid,env,retrack) )
	    return false;
    }

    return true;
}


void HorizonFlatViewEditor2D::cleanAuxInfoContainer()
{
    for ( int idx=0; idx<markeridinfos_.size(); idx++ )
	editor_->removeAuxData( markeridinfos_[idx]->merkerid_ );

    if ( markeridinfos_.size() )
	deepErase( markeridinfos_ );
}


void HorizonFlatViewEditor2D::fillAuxInfoContainer()
{
    ObjectSet<EM::HorizonPainter2D::Marker2D> disphormrkinfos;
    horpainter_->getDisplayedHor( disphormrkinfos );

    for ( int idx=0; idx<disphormrkinfos.size(); idx++ )
    {
	Hor2DMarkerIdInfo* markeridinfo = new Hor2DMarkerIdInfo;
	markeridinfo->merkerid_ = editor_->addAuxData(
					disphormrkinfos[idx]->marker_, true );
	markeridinfo->marker_ = disphormrkinfos[idx]->marker_;
	markeridinfo->sectionid_ = disphormrkinfos[idx]->sectionid_;
	editor_->enableEdit( markeridinfo->merkerid_, false, false, false );
	editor_->enablePolySel( markeridinfo->merkerid_, mehandler_ );

	markeridinfos_ += markeridinfo;
    }
}


void HorizonFlatViewEditor2D::horRepaintATSCB( CallBacker* )
{
    cleanAuxInfoContainer();
}


void HorizonFlatViewEditor2D::horRepaintedCB( CallBacker* )
{
    fillAuxInfoContainer();
}


FlatView::AuxData* HorizonFlatViewEditor2D::getAuxData( int markid )
{
    for ( int idx=0; idx<markeridinfos_.size(); idx++ )
    {
	if ( markeridinfos_[idx]->merkerid_ == markid )
	    return markeridinfos_[idx]->marker_;
    }

    return 0;
}


EM::SectionID HorizonFlatViewEditor2D::getSectionID( int markid )
{
    for ( int idx=0; idx<markeridinfos_.size(); idx++ )
    {
	if ( markeridinfos_[idx]->merkerid_ == markid )
	    return markeridinfos_[idx]->sectionid_;
    }

    return -1;
}


bool HorizonFlatViewEditor2D::getPosID( const Coord3& crd,
					EM::PosID& pid ) const
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return false;

    PtrMan<IOObj> ioobj = IOM().get( lsetid_ );
    if ( !ioobj ) return false;

    const Seis2DLineSet lset( ioobj->fullUserExpr(true) );
    S2DPOS().setCurLineSet( lset.name() );
    PosInfo::LineSet2DData linesetgeom;
    for ( int idx=0; idx<lset.nrLines(); idx++ )
    {
	PosInfo::Line2DData& linegeom = linesetgeom.addLine(lset.lineName(idx));
	linegeom.setLineName( lset.lineName(idx) );
	S2DPOS().getGeometry( linegeom );
	if ( linegeom.positions().isEmpty() )
	{
	    linesetgeom.removeLine( lset.lineName(idx) );
	    continue;
	}
    }

    PosInfo::Line2DPos pos;
    if ( !linesetgeom.getLineData( linenm_ ) )
	return false;

    linesetgeom.getLineData( linenm_ )->getPos( crd, pos, mUdf(double) );
    mDynamicCastGet(const EM::Horizon2D*,hor2d,emobj);

    if ( !hor2d ) return false;

    BinID bid;
    bid.inl = hor2d->geometry().lineIndex( linenm_ );
    bid.crl = pos.nr_;

    for ( int idx=0; idx<emobj->nrSections(); idx++ )
    {
	if ( emobj->isDefined(emobj->sectionID(idx),bid.toInt64()) )
	{
	    pid.setObjectID( emobj->id() );
	    pid.setSectionID( emobj->sectionID(idx) );
	    pid.setSubID( bid.toInt64() );
	    return true;
	}
    }

    return false;
}


void HorizonFlatViewEditor2D::movementEndCB( CallBacker* )
{}


void HorizonFlatViewEditor2D::removePosCB( CallBacker* )
{
    TypeSet<int> selectedids;
    TypeSet<int> selectedidxs;
    editor_->getPointSelections( selectedids, selectedidxs );

    if ( !selectedids.size() ) return;

    RefMan<EM::EMObject> emobj = EM::EMM().getObject( emid_ );
    if ( !emobj ) return;

    mDynamicCastGet(EM::Horizon2D*,hor2d,emobj.ptr());
    if ( !hor2d ) return;

    hor2d->setBurstAlert( true );

    BinID bid;

    for ( int ids=0; ids<selectedids.size(); ids++ )
    {
	bid.inl = hor2d->geometry().lineIndex( linenm_ );

	int posidx = horpainter_->getDistances().indexOf(
				    getAuxData(selectedids[ids])->
				    poly_[selectedidxs[ids]].x );
	bid.crl = horpainter_->getTrcNos()[posidx];

	EM::PosID posid( emid_, getSectionID(selectedids[ids]), bid.toInt64() );
	emobj->unSetPos( posid, false );
    }

    hor2d->setBurstAlert( false );
}

}//namespace MPE
