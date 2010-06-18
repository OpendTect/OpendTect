
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2010
 RCS:		$Id: mpef3dflatvieweditor.cc,v 1.2 2010-06-18 12:23:27 cvskris Exp $
________________________________________________________________________

-*/

#include "mpef3dflatvieweditor.h"

#include "faulteditor.h"
#include "emfault3d.h"
#include "emfault3dpainter.h"
#include "emmanager.h"
#include "flatauxdataeditor.h"
#include "flatposdata.h"
#include "mouseevent.h"
#include "mousecursor.h"
#include "mpeengine.h"
#include "survinfo.h"
#include "undo.h"

#include "uiworld2ui.h"

namespace MPE
{

Fault3DFlatViewEditor::Fault3DFlatViewEditor(
			    FlatView::AuxDataEditor* ed)
    : EM::FaultStickSetFlatViewEditor(ed)
    , editor_(ed)
    , f3dpainter_( new EM::Fault3DPainter(ed->viewer()) )
    , meh_(0)
    , activestickid_(mUdf(int))
    , seedhasmoved_(false)
    , mousepid_(-1)
{
    ed->movementStarted.notify(
	    mCB(this,Fault3DFlatViewEditor,seedMovementStartedCB) );
    ed->movementFinished.notify(
	    mCB(this,Fault3DFlatViewEditor,seedMovementFinishedCB) );
    MPE::engine().activefaultchanged_.notify(
	    mCB(this,Fault3DFlatViewEditor,activeF3DChgCB) );
    f3dpainter_->abouttorepaint_.notify(
	    mCB(this,Fault3DFlatViewEditor,f3dRepaintATSCB) );
    f3dpainter_->repaintdone_.notify( 
	    mCB(this,Fault3DFlatViewEditor,f3dRepaintedCB) );
}


Fault3DFlatViewEditor::~Fault3DFlatViewEditor()
{
    editor_->movementStarted.remove(
	    mCB(this,Fault3DFlatViewEditor,seedMovementStartedCB) );
    editor_->movementFinished.remove(
	    mCB(this,Fault3DFlatViewEditor,seedMovementFinishedCB) );
    MPE::engine().activefaultchanged_.remove(
	    mCB(this,Fault3DFlatViewEditor,activeF3DChgCB) );
    setMouseEventHandler( 0 );
    delete f3dpainter_;
    deepErase( markeridinfo_ );
}


void Fault3DFlatViewEditor::setMouseEventHandler( MouseEventHandler* meh )
{
    if ( meh_ )
    {
	meh_->movement.remove(
		mCB(this,Fault3DFlatViewEditor,mouseMoveCB) );
	meh_->buttonPressed.remove(
		mCB(this,Fault3DFlatViewEditor,mousePressCB) );
	meh_->buttonReleased.remove(
		mCB(this,Fault3DFlatViewEditor,mouseReleaseCB) );
    }

    meh_ = meh;

    if ( meh_ )
    {
	meh_->movement.notify(
		mCB(this,Fault3DFlatViewEditor,mouseMoveCB) );
	meh_->buttonPressed.notify(
		mCB(this,Fault3DFlatViewEditor,mousePressCB) );
	meh_->buttonReleased.notify(
		mCB(this,Fault3DFlatViewEditor,mouseReleaseCB) );
    }
}


void Fault3DFlatViewEditor::setCubeSampling( const CubeSampling& cs )
{
    EM::FaultStickSetFlatViewEditor::setCubeSampling( cs );
    f3dpainter_->setCubeSampling( cs, true );
}


void Fault3DFlatViewEditor::drawFault()
{
    for ( int idx=0; idx<EM::EMM().nrLoadedObjects(); idx++ )
    {
	EM::ObjectID emid = EM::EMM().objectID( idx );
	RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
	mDynamicCastGet(EM::Fault3D*,emf3d,emobject.ptr());
	if ( !emf3d )
	    continue;
	f3dpainter_->addFault3D( emid );
    }
}


void Fault3DFlatViewEditor::activeF3DChgCB( CallBacker* cb )
{
    f3dpainter_->setActiveF3D( MPE::engine().getActiveFaultObjID() );
    cleanActStkContainer();
    if ( MPE::engine().getActiveFaultObjID() != -1 )
	fillActStkContainer( MPE::engine().getActiveFaultObjID() );
}


void Fault3DFlatViewEditor::f3dRepaintATSCB( CallBacker* )
{
    cleanActStkContainer();
}


void Fault3DFlatViewEditor::f3dRepaintedCB( CallBacker* )
{
    if ( MPE::engine().getActiveFaultObjID() != -1 )
	fillActStkContainer( MPE::engine().getActiveFaultObjID() );
    activestickid_ = mUdf(int);
}


void Fault3DFlatViewEditor::seedMovementStartedCB( CallBacker* )
{
    int edidauxdataid = editor_->getSelPtDataID();
    int knotid = -1;
    if ( editor_->getSelPtIdx().size() > 0 )
	knotid = editor_->getSelPtIdx()[0];

    if ( (edidauxdataid==-1) || (knotid==-1) )
	return;

    int selstickid = mUdf(int);

    for ( int idx=0; idx<markeridinfo_.size(); idx++ )
    {
	if ( markeridinfo_[idx]->merkerid_ == edidauxdataid )
	{ 
	    selstickid = markeridinfo_[idx]->stickid_;
	    break;
	}
    }

    if ( selstickid == mUdf(int) )
	return;

    if ( selstickid == f3dpainter_->getActiveStickId() )
	return;

    const Geom::Point2D<double> pos = editor_->getSelPtPos();

    EM::ObjectID emid = MPE::engine().getActiveFaultObjID();
    if ( emid == -1 ) return; 

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::Fault3D*,emf3d,emobject.ptr());
    if ( !emf3d )
	return;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, true );
    mDynamicCastGet( MPE::FaultEditor*, f3deditor, editor.ptr() );
    if ( !f3deditor )
	return;

    seedhasmoved_ = true;
}


void Fault3DFlatViewEditor::seedMovementFinishedCB( CallBacker* )
{
    int edidauxdataid = editor_->getSelPtDataID();
    int displayedknotid = -1;
    if ( editor_->getSelPtIdx().size() > 0 )
	displayedknotid = editor_->getSelPtIdx()[0];

    if ( (edidauxdataid==-1) || (displayedknotid==-1) )
	return;

    const Geom::Point2D<double> pos = editor_->getSelPtPos();
    
    const FlatDataPack* dp = editor_->viewer().pack( false );
    if ( !dp )
	dp = editor_->viewer().pack( true );

    if ( !dp ) return;

    const FlatPosData& pd = dp->posData();
    const IndexInfo ix = pd.indexInfo( true, pos.x );
    const IndexInfo iy = pd.indexInfo( false, pos.y );
    Coord3 coord3 = dp->getCoord( ix.nearest_, iy.nearest_ );
    coord3.z = ( !cs_.isEmpty() && cs_.nrZ() == 1) ? cs_.zrg.start : pos.y;

    EM::ObjectID emid = MPE::engine().getActiveFaultObjID();
    if ( emid == -1 ) return; 

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::Fault3D*,emf3d,emobject.ptr());
    if ( !emf3d )
	return;

    int sid = emf3d->sectionID( 0 );
    mDynamicCastGet( const Geometry::FaultStickSet*, emfss, 
		     emf3d->sectionGeometry( sid ) );
    if ( !emfss ) return;

    StepInterval<int> colrg = emfss->colRange( f3dpainter_->getActiveStickId());
    const int knotid = colrg.start + displayedknotid*colrg.step;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, true );
    mDynamicCastGet( MPE::FaultEditor*, f3deditor, editor.ptr() );
    if ( !f3deditor )
	return;

    const RowCol knotrc( f3dpainter_->getActiveStickId(), knotid );

    EM::PosID pid( emid,0,knotrc.toInt64() );

    emf3d->setPos(pid,coord3,true);
    seedhasmoved_ = true;
}


void Fault3DFlatViewEditor::mouseMoveCB( CallBacker* )
{
    if ( seedhasmoved_ )
	return;

    EM::ObjectID emid = MPE::engine().getActiveFaultObjID();
    if ( emid == -1 ) return; 

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::Fault3D*,emf3d,emobject.ptr());
    if ( !emf3d )
	return;

    if ( emf3d->isEmpty() )
	return;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, true );
    mDynamicCastGet( MPE::FaultEditor*, f3deditor, editor.ptr() );
    if ( !f3deditor )
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

    bool shdmakenewstick = false;
    EM::PosID pid;
    f3deditor->getInteractionInfo( shdmakenewstick, pid, pos, SI().zScale() );

    if ( pid.isUdf() || shdmakenewstick )
	return; 

    const int sticknr = pid.isUdf() ? mUdf(int) : RowCol(pid.subID()).row;

    if ( activestickid_ != sticknr )
	activestickid_ = sticknr;

    if( f3dpainter_->hasDiffActiveStick(&pid) )
	f3dpainter_->setActiveStick( pid );
}


void Fault3DFlatViewEditor::mousePressCB( CallBacker* )
{
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

    EM::ObjectID emid = MPE::engine().getActiveFaultObjID();
    if ( emid == -1 ) return;

    int stickid = mUdf(int);

    for ( int idx=0; idx<markeridinfo_.size(); idx++ )
    {
	if ( markeridinfo_[idx]->merkerid_ == edidauxdataid )
	{ 
	    stickid = markeridinfo_[idx]->stickid_;
	    break;
	}
    }

    if ( stickid == mUdf(int) ) return;

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );

    mDynamicCastGet(EM::Fault3D*,emf3d,emobject.ptr());
    if ( !emf3d ) return;

    int sid = emf3d->sectionID( 0 );
    mDynamicCastGet( const Geometry::FaultStickSet*, fss, 
		     emf3d->sectionGeometry( sid ) );

    RowCol rc;
    rc.row = stickid;
    int knotid = mUdf(int);

    StepInterval<int> colrg = fss->colRange( rc.row );
    knotid = colrg.start + displayedknotid*colrg.step;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, true );
    mDynamicCastGet( MPE::FaultEditor*, f3deditor, editor.ptr() );
    if ( !f3deditor )
	return;

    EM::PosID mousepid( emid, 0, RowCol(stickid,knotid).toInt64() );
    const MouseEvent& mouseevent = meh_->event();
    f3deditor->setLastClicked( mousepid );
    activestickid_ = stickid;
    f3dpainter_->setActiveStick( mousepid );
    mousepid_ = mousepid;
}


void Fault3DFlatViewEditor::mouseReleaseCB( CallBacker* )
{
    if ( !editor_->viewer().appearance().annot_.editable_ 
	 || editor_->isSelActive() )
	return;

    if ( seedhasmoved_ )
    {
	seedhasmoved_ = false;
	return;
    }
    EM::ObjectID emid = MPE::engine().getActiveFaultObjID();
    if ( emid == -1 ) return; 

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::Fault3D*,emf3d,emobject.ptr());
    if ( !emf3d )
	return;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, true );
    mDynamicCastGet( MPE::FaultEditor*, f3deditor, editor.ptr() );
    if ( !f3deditor )
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

    bool makenewstick = !mouseevent.ctrlStatus() && mouseevent.shiftStatus();
    EM::PosID interactpid;
    f3deditor->getInteractionInfo( makenewstick, interactpid, pos,
	    			   SI().zScale() );

    if ( !mousepid_.isUdf() && mouseevent.ctrlStatus() 
	 && !mouseevent.shiftStatus() )
    {
	//Remove knot/stick
	bool res;
	const int rmnr = RowCol(mousepid_.subID()).row;
	if ( emf3d->geometry().nrKnots(mousepid_.sectionID(),rmnr) == 1 )
	{
	    res = emf3d->geometry().removeStick( mousepid_.sectionID(), rmnr,
		    				 true );
	    f3deditor->setLastClicked( EM::PosID::udf() );
	}
	else
	    res = emf3d->geometry().removeKnot( mousepid_.sectionID(),
		    				mousepid_.subID(), true );
	if ( res )
	    EM::EMM().undo().setUserInteractionEnd(
			EM::EMM().undo().currentEventID() );
	return;
    }

    if ( !mousepid_.isUdf() || interactpid.isUdf() )
	return;

    if ( makenewstick )
    {
	Coord3 editnormal( 0, 0, 1 );
	if ( cs_.isEmpty() ) return;

	const int insertsticknr = interactpid.isUdf()
	    ? mUdf( int )
	    : RowCol(interactpid.subID()).row;

	if ( cs_.defaultDir()==CubeSampling::Inl )
	    editnormal = Coord3( SI().binID2Coord().rowDir(), 0 );
	else if ( cs_.defaultDir()==CubeSampling::Crl )
	    editnormal = Coord3( SI().binID2Coord().colDir(), 0 );

	if ( emf3d->geometry().insertStick(interactpid.sectionID(),
		insertsticknr,0,pos,editnormal,true) )
	{
	    EM::EMM().undo().setUserInteractionEnd(
			EM::EMM().undo().currentEventID() );
	    f3deditor->setLastClicked( interactpid );
	    f3deditor->editpositionchange.trigger();
	}
    }
    else
    {
	if ( emf3d->geometry().insertKnot(interactpid.sectionID(),
		interactpid.subID(),pos,true) )
	{
	    EM::EMM().undo().setUserInteractionEnd(
			EM::EMM().undo().currentEventID() );
	    f3deditor->setLastClicked( interactpid );
	    f3deditor->editpositionchange.trigger();
	}
    }
}


void Fault3DFlatViewEditor::cleanActStkContainer()
{
    for ( int idx=0; idx<markeridinfo_.size(); idx++ )
	editor_->removeAuxData( markeridinfo_[idx]->merkerid_ );

    if ( markeridinfo_.size() )
	deepErase( markeridinfo_ );
}


void Fault3DFlatViewEditor::updateActStkContainer()
{
    cleanActStkContainer();
    if ( MPE::engine().getActiveFaultObjID() != -1 )
	fillActStkContainer( MPE::engine().getActiveFaultObjID() );
}


void Fault3DFlatViewEditor::fillActStkContainer( const EM::ObjectID oid )
{
    ObjectSet<EM::Fault3DPainter::StkMarkerInfo> dispdstkmrkinfos;
    f3dpainter_->getDisplayedSticks( oid, dispdstkmrkinfos );

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

} //namespace MPE
