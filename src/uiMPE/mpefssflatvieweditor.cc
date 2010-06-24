
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
 RCS:           $Id: mpefssflatvieweditor.cc,v 1.9 2010-06-24 08:49:56 cvsumesh Exp $
________________________________________________________________________

-*/

#include "mpefssflatvieweditor.h"

#include "emeditor.h"
#include "emfaultstickset.h"
#include "emfaultstickpainter.h"
#include "emmanager.h"
#include "faultstickseteditor.h"
#include "flatauxdataeditor.h"
#include "flatposdata.h"
#include "mouseevent.h"
#include "mousecursor.h"
#include "mpeengine.h"
#include "survinfo.h"

#include "uiworld2ui.h"

namespace MPE
{

FaultStickSetFlatViewEditor::FaultStickSetFlatViewEditor(
				FlatView::AuxDataEditor* ed,
				const EM::ObjectID& oid )
    : EM::FaultStickSetFlatViewEditor(ed)
    , editor_(ed)
    , fsspainter_( new EM::FaultStickPainter(ed->viewer(),oid) )
    , meh_(0)
    , activestickid_(-1)
    , seedhasmoved_(false)
    , mousepid_(-1)
{
    ed->movementStarted.notify(
	    mCB(this,FaultStickSetFlatViewEditor,seedMovementStartedCB) );
    ed->movementFinished.notify(
	    mCB(this,FaultStickSetFlatViewEditor,seedMovementFinishedCB) );
    fsspainter_->abouttorepaint_.notify(
	    mCB(this,FaultStickSetFlatViewEditor,fssRepaintATSCB) );
    fsspainter_->repaintdone_.notify( 
	    mCB(this,FaultStickSetFlatViewEditor,fssRepaintedCB) );
}


FaultStickSetFlatViewEditor::~FaultStickSetFlatViewEditor()
{
    editor_->movementStarted.remove(
	    mCB(this,FaultStickSetFlatViewEditor,seedMovementStartedCB) );
    editor_->movementFinished.remove(
	    mCB(this,FaultStickSetFlatViewEditor,seedMovementFinishedCB) );
    setMouseEventHandler( 0 );
    delete fsspainter_;
    deepErase( markeridinfo_ );
}


void FaultStickSetFlatViewEditor::setMouseEventHandler( MouseEventHandler* meh )
{
    if ( meh_ )
    {
	meh_->movement.remove(
		mCB(this,FaultStickSetFlatViewEditor,mouseMoveCB) );
	meh_->buttonPressed.remove(
		mCB(this,FaultStickSetFlatViewEditor,mousePressCB) );
	meh_->buttonReleased.remove(
		mCB(this,FaultStickSetFlatViewEditor,mouseReleaseCB) );
    }

    meh_ = meh;

    if ( meh_ )
    {
	meh_->movement.notify(
		mCB(this,FaultStickSetFlatViewEditor,mouseMoveCB) );
	meh_->buttonPressed.notify(
		mCB(this,FaultStickSetFlatViewEditor,mousePressCB) );
	meh_->buttonReleased.notify(
		mCB(this,FaultStickSetFlatViewEditor,mouseReleaseCB) );
    }
}


void FaultStickSetFlatViewEditor::setCubeSampling( const CubeSampling& cs )
{
    EM::FaultStickSetFlatViewEditor::setCubeSampling( cs );
    fsspainter_->setCubeSampling( cs, true );
}


void FaultStickSetFlatViewEditor::drawFault()
{
    fsspainter_->paint();
}


void FaultStickSetFlatViewEditor::enablePainting( bool yn )
{
    fsspainter_->enableKnots( yn );
    fsspainter_->enableLine( yn );
}


void FaultStickSetFlatViewEditor::enableKnots( bool yn )
{
    fsspainter_->enableKnots( yn );
}


void FaultStickSetFlatViewEditor::updateActStkContainer()
{
    cleanActStkContainer();

    if ( MPE::engine().getActiveFSSObjID() != -1 &&
	 (MPE::engine().getActiveFSSObjID()==fsspainter_->getFaultSSID()) )
	fillActStkContainer();
}


void FaultStickSetFlatViewEditor::cleanActStkContainer()
{
    for ( int idx=0; idx<markeridinfo_.size(); idx++ )
	editor_->removeAuxData( markeridinfo_[idx]->merkerid_ );

    if ( markeridinfo_.size() )
	deepErase( markeridinfo_ );
}


void FaultStickSetFlatViewEditor::fillActStkContainer()
{
    ObjectSet<EM::FaultStickPainter::StkMarkerInfo> dispdstkmrkinfos;
    fsspainter_->getDisplayedSticks( dispdstkmrkinfos );

    for ( int idx=0; idx<dispdstkmrkinfos.size(); idx++ )
    {
	StkMarkerIdInfo* merkeridinfo = new  StkMarkerIdInfo;
	merkeridinfo->merkerid_ = editor_->addAuxData(
			    dispdstkmrkinfos[idx]->marker_, true );
	merkeridinfo->stickid_ = dispdstkmrkinfos[idx]->stickid_;
	editor_->enableEdit( merkeridinfo->merkerid_, false, true, false );

	markeridinfo_ += merkeridinfo;
    }
}


void FaultStickSetFlatViewEditor::fssRepaintATSCB( CallBacker* )
{ 
    cleanActStkContainer(); 
}


void FaultStickSetFlatViewEditor::fssRepaintedCB( CallBacker* )
{
    if ( MPE::engine().getActiveFSSObjID() != -1 &&
	 (MPE::engine().getActiveFSSObjID()==fsspainter_->getFaultSSID()) )
	fillActStkContainer();

    activestickid_ = mUdf(int);
}


void FaultStickSetFlatViewEditor::seedMovementStartedCB( CallBacker* cb )
{
    int edidauxdataid = editor_->getSelPtDataID();
    int knotid = -1;
    if ( editor_->getSelPtIdx().size() > 0 )
	knotid = editor_->getSelPtIdx()[0];

    if ( (edidauxdataid==-1) || (knotid==-1) )
	return;

    int selstickid = -1;

    for ( int idx=0; idx<markeridinfo_.size(); idx++ )
    {
	if ( markeridinfo_[idx]->merkerid_ == edidauxdataid )
	{ 
	    selstickid = markeridinfo_[idx]->stickid_;
	    break;
	}
    }

    if ( selstickid == -1 )
	return;

    if ( selstickid == fsspainter_->getActiveStickId() )
	return;

    const Geom::Point2D<double> pos = editor_->getSelPtPos();

    EM::ObjectID emid = MPE::engine().getActiveFSSObjID();
    if ( emid == -1 ) return;

    if ( emid !=fsspainter_->getFaultSSID() )
	return;

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss )
	return;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, true );
    mDynamicCastGet( MPE::FaultStickSetEditor*, fsseditor, editor.ptr() );
    if ( !fsseditor )
	return;

    seedhasmoved_ = true;
}


void FaultStickSetFlatViewEditor::seedMovementFinishedCB( CallBacker* cb )
{
    int edidauxdataid = editor_->getSelPtDataID();
    int displayedknotid = -1;
    if ( editor_->getSelPtIdx().size() > 0 )
	displayedknotid = editor_->getSelPtIdx()[0];

    if ( (edidauxdataid==-1) || (displayedknotid==-1) )
	return;

    const Geom::Point2D<double> pos = editor_->getSelPtPos();

    const CubeSampling& cs = fsspainter_->getCubeSampling();
    
    const FlatDataPack* dp = editor_->viewer().pack( false );
    if ( !dp )
	dp = editor_->viewer().pack( true );

    if ( !dp ) return;

    const FlatPosData& pd = dp->posData();
    const IndexInfo ix = pd.indexInfo( true, pos.x );
    const IndexInfo iy = pd.indexInfo( false, pos.y );
    Coord3 coord3 = dp->getCoord( ix.nearest_, iy.nearest_ );
    coord3.z = ( !cs_.isEmpty() && cs_.nrZ() == 1) ? cs_.zrg.start : pos.y;

    EM::ObjectID emid = MPE::engine().getActiveFSSObjID();
    if ( emid == -1 ) return; 

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss )
	return;

    int sid = emfss->sectionID( 0 );
    mDynamicCastGet( const Geometry::FaultStickSet*, fss, 
		     emfss->sectionGeometry( sid ) );

    StepInterval<int> colrg = fss->colRange( fsspainter_->getActiveStickId() );
    const int knotid = colrg.start + displayedknotid*colrg.step;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, true );
    mDynamicCastGet( MPE::FaultStickSetEditor*, fsseditor, editor.ptr() );
    if ( !fsseditor )
	return;

    const RowCol knotrc( fsspainter_->getActiveStickId(), knotid );

    EM::PosID pid( emid,0,knotrc.toInt64() );

    emfss->setPos(pid,coord3,true);
    seedhasmoved_ = true;
}


void FaultStickSetFlatViewEditor::mouseMoveCB( CallBacker* cb )
{
    if ( seedhasmoved_ )
	return;

    EM::ObjectID emid = MPE::engine().getActiveFSSObjID();
    if ( emid == -1 ) return; 

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss )
	return;

    if ( emfss->isEmpty() )
	return;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, true );
    mDynamicCastGet( MPE::FaultStickSetEditor*, fsseditor, editor.ptr() );
    if ( !fsseditor )
	return;

    const FlatDataPack* dp = editor_->viewer().pack( false );
    if ( !dp )
	dp = editor_->viewer().pack( true );

    if ( !dp ) return;

    const MouseEvent& mouseevent = meh_->event();
    const uiRect datarect( editor_->getMouseArea() );
    if ( !datarect.isInside(mouseevent.pos()) ) return;

    const uiWorld2Ui w2u( datarect.size(), editor_->getWorldRect(mUdf(int)) );
    const uiWorldPoint wp = w2u.transform( mouseevent.pos()-datarect.topLeft());

    const FlatPosData& pd = dp->posData();
    const IndexInfo ix = pd.indexInfo( true, wp.x );
    const IndexInfo iy = pd.indexInfo( false, wp.y );
    Coord3 pos = dp->getCoord( ix.nearest_, iy.nearest_ );
    pos.z = ( !cs_.isEmpty() && cs_.nrZ() == 1) ? cs_.zrg.start : wp.y;

    EM::PosID pid;
    fsseditor->getInteractionInfo( pid, &fsspainter_->getLineSetID(),
			fsspainter_->getLineName(), pos, SI().zScale() );

    if ( pid.isUdf() )
	return; 

    const int sticknr = pid.isUdf() ? mUdf(int) : RowCol(pid.subID()).row;

    if ( activestickid_ != sticknr )
	activestickid_ = sticknr;

    if( fsspainter_->hasDiffActiveStick(&pid) )
	fsspainter_->setActiveStick( pid );
}


void FaultStickSetFlatViewEditor::mousePressCB( CallBacker* cb )
{
    bool active = editor_->viewer().appearance().annot_.editable_;
    bool sel = editor_->isSelActive();
    if ( !editor_->viewer().appearance().annot_.editable_
	 || editor_->isSelActive() )
	return;

    mousepid_.setObjectID( -1 );
    int edidauxdataid = editor_->getSelPtDataID();
    int displayedknotid = -1;
    if ( editor_->getSelPtIdx().size() > 0 )
	displayedknotid = editor_->getSelPtIdx()[0];

    if ( (edidauxdataid==-1) || (displayedknotid==-1) )
	return;

    EM::ObjectID emid = MPE::engine().getActiveFSSObjID();
    if ( emid == -1 ) return;

    int stickid = -1;

    for ( int idx=0; idx<markeridinfo_.size(); idx++ )
    {
	if ( markeridinfo_[idx]->merkerid_ == edidauxdataid )
	{ 
	    stickid = markeridinfo_[idx]->stickid_;
	    break;
	}
    }

    if ( stickid == -1 ) return;

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );

    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss ) return;

    int sid = emfss->sectionID( 0 );
    mDynamicCastGet( const Geometry::FaultStickSet*, fss, 
		     emfss->sectionGeometry( sid ) );

    RowCol rc;
    rc.row = stickid;
    int knotid = -1;

    StepInterval<int> colrg = fss->colRange( rc.row );
    knotid = colrg.start + displayedknotid*colrg.step;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, true );
    mDynamicCastGet( MPE::FaultStickSetEditor*, fsseditor, editor.ptr() );
    if ( !fsseditor )
	return;

    EM::PosID mousepid( emid, 0, RowCol(stickid,knotid).toInt64() );
    fsseditor->setLastClicked( mousepid );
    activestickid_ = stickid;
    fsspainter_->setActiveStick( mousepid );
    mousepid_ = mousepid;
}


void FaultStickSetFlatViewEditor::mouseReleaseCB( CallBacker* cb )
{
    if ( !editor_->viewer().appearance().annot_.editable_ 
	 || editor_->isSelActive() )
	return;

    if ( seedhasmoved_ )
    {
	seedhasmoved_ = false;
	return;
    }
     EM::ObjectID emid = MPE::engine().getActiveFSSObjID();
    if ( emid == -1 ) return; 

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss )
	return;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, true );
    mDynamicCastGet( MPE::FaultStickSetEditor*, fsseditor, editor.ptr() );
    if ( !fsseditor )
	return;

    const FlatDataPack* dp = editor_->viewer().pack( false );
    if ( !dp )
	dp = editor_->viewer().pack( true );

    if ( !dp ) return;

    const MouseEvent& mouseevent = meh_->event();
    const uiRect datarect( editor_->getMouseArea() );
    if ( !datarect.isInside(mouseevent.pos()) ) return;

    const uiWorld2Ui w2u( datarect.size(), editor_->getWorldRect(mUdf(int)) );
    const uiWorldPoint wp = w2u.transform( mouseevent.pos()-datarect.topLeft());

    const FlatPosData& pd = dp->posData();
    const IndexInfo ix = pd.indexInfo( true, wp.x );
    const IndexInfo iy = pd.indexInfo( false, wp.y );
    Coord3 pos = dp->getCoord( ix.nearest_, iy.nearest_ );
    pos.z = ( !cs_.isEmpty() && cs_.nrZ() == 1) ? cs_.zrg.start : wp.y;

    EM::FaultStickSetGeometry& fssg = emfss->geometry();
    EM::PosID interactpid;
    fsseditor->getInteractionInfo( interactpid, &fsspainter_->getLineSetID() ,
				   fsspainter_->getLineName(), pos,
				   SI().zScale() );

    if ( !mousepid_.isUdf() && mouseevent.ctrlStatus() 
	 && !mouseevent.shiftStatus() )
    {
	//Remove knot/stick
	const int rmnr = RowCol(mousepid_.subID()).row;
	if ( fssg.nrKnots(mousepid_.sectionID(),rmnr) == 1 )
	{
	    fssg.removeStick( mousepid_.sectionID(), rmnr, true );
	    fsseditor->setLastClicked( EM::PosID::udf() );
	}
	else
	    fssg.removeKnot( mousepid_.sectionID(), mousepid_.subID(), true );
	return;
    }

    if ( mouseevent.shiftStatus() || interactpid.isUdf() )
    {
	Coord3 editnormal( 0, 0, 1 );

	const MultiID* lineset = 0;
	const char* linenm = 0;

	if ( cs_.isEmpty() )
	{
	    editnormal = Coord3( fsspainter_->getNormalToTrace(ix.nearest_),0 );
	    lineset = &fsspainter_->getLineSetID();
	    linenm = fsspainter_->getLineName();
	}
	else if ( cs_.defaultDir()==CubeSampling::Inl )
	    editnormal = Coord3( SI().binID2Coord().rowDir(), 0 );
	else if ( cs_.defaultDir()==CubeSampling::Crl )
	    editnormal = Coord3( SI().binID2Coord().colDir(), 0 );

	const int sid = emfss->sectionID(0);
	Geometry::FaultStickSet* fss = fssg.sectionGeometry( sid );
	const int insertsticknr = !fss || fss->isEmpty() 
	    			  ? 0 : fss->rowRange().stop+1;

	fssg.insertStick( sid, insertsticknr, 0, pos, editnormal,
			  lineset, linenm, true );
	const EM::SubID subid = RowCol(insertsticknr,0).toInt64();
	fsseditor->setLastClicked( EM::PosID(emfss->id(),sid,subid) );
    }
    else
    {
	fssg.insertKnot( interactpid.sectionID(), interactpid.subID(),
			 pos, true );
	fsseditor->setLastClicked( interactpid );
    }
}


void FaultStickSetFlatViewEditor::set2D( bool yn )
{ fsspainter_->set2D( yn ); }


void FaultStickSetFlatViewEditor::setLineName( const char* ln )
{ fsspainter_->setLineName( ln ); }


void FaultStickSetFlatViewEditor::setLineID( const MultiID& lsetid )
{ fsspainter_->setLineID( lsetid ); }


TypeSet<int>& FaultStickSetFlatViewEditor::getTrcNos()
{ return fsspainter_->getTrcNos(); }


TypeSet<float>& FaultStickSetFlatViewEditor::getDistances()
{ return fsspainter_->getDistances(); }


TypeSet<Coord>& FaultStickSetFlatViewEditor::getCoords()
{ return fsspainter_->getCoords(); }

} // namespace MPE
