
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
 RCS:		$Id: mpefssflatvieweditor.cc,v 1.1 2010-02-12 08:44:27 cvsumesh Exp $
________________________________________________________________________

-*/

#include "mpefssflatvieweditor.h"

#include "emeditor.h"
#include "emfaultstickset.h"
#include "emfaultstickpainter.h"
#include "emmanager.h"
#include "emposid.h"
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
				FlatView::AuxDataEditor* ed )
    : EM::FaultStickSetFlatViewEditor(ed)
    , editor_(ed)
    , meh_(0)
    , activeeditorid_(-1)
    , activestickid_(-1)
    , seedhasmoved_(false)
{
    ed->movementStarted.notify(
	    mCB(this,FaultStickSetFlatViewEditor,seedMovementStartedCB) );
    ed->movementFinished.notify(
	    mCB(this,FaultStickSetFlatViewEditor,seedMovementFinishedCB) );
    MPE::engine().activefsschanged.notify(
	    mCB(this,FaultStickSetFlatViewEditor,activeFSSChgCB) );
    editor_->viewer().appearance().annot_.editable_ = true;
}


FaultStickSetFlatViewEditor::~FaultStickSetFlatViewEditor()
{
    editor_->movementStarted.remove(
	    mCB(this,FaultStickSetFlatViewEditor,seedMovementStartedCB) );
    editor_->movementFinished.notify(
	    mCB(this,FaultStickSetFlatViewEditor,seedMovementFinishedCB) );
    setMouseEventHandler( 0 );
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


void FaultStickSetFlatViewEditor::activeFSSChgCB( CallBacker* )
{
    fsspainter_->setActiveFSS( MPE::engine().getActiveFSSObjID() );
}


void FaultStickSetFlatViewEditor::seedMovementStartedCB( CallBacker* cb )
{
}


void FaultStickSetFlatViewEditor::seedMovementFinishedCB( CallBacker* cb )
{
    int edidauxdataid = editor_->getSelPtDataID();
    int seedid = -1;
    if ( editor_->getSelPtIdx().size() > 0 )
	seedid = editor_->getSelPtIdx()[0];

    if ( (edidauxdataid==-1) || (seedid==-1) )
	return;

    const Geom::Point2D<double> pos = editor_->getSelPtPos();
    const CubeSampling& cs = fsspainter_->getCubeSampling();
    const bool isinl = (cs.nrInl()==1);
    const BinID bid = isinl
	? BinID(cs.hrg.start.inl,cs.hrg.crlRange().snap(pos.x) )
	: BinID(cs.hrg.inlRange().snap(pos.x),cs.hrg.start.crl);

    Coord3 coord3( SI().transform(bid), pos.y );

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

    const RowCol rc( fsspainter_->getActiveStickId(), seedid );

    EM::PosID pid( emid,0,rc.getSerialized() );

    emfss->setPos(pid,coord3,true);
    seedhasmoved_ = true;
}


void FaultStickSetFlatViewEditor::mouseMoveCB( CallBacker* cb )
{
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

    const MouseEvent& mouseevent = meh_->event();
    const uiRect datarect( editor_->getMouseArea() );
    if ( !datarect.isInside(mouseevent.pos()) ) return;

    const uiWorld2Ui w2u( datarect.size(), editor_->getWorldRect(mUdf(int)) );
    const uiWorldPoint wp = w2u.transform( mouseevent.pos()-datarect.topLeft());

    const FlatPosData& pd = dp->posData();
    const IndexInfo ix = pd.indexInfo( true, wp.x );
    const IndexInfo iy = pd.indexInfo( false, wp.y );
    Coord3 pos = dp->getCoord( ix.nearest_, iy.nearest_ );
    pos.z = wp.y;

    EM::PosID pid;
    fsseditor->getInteractionInfo( pid, 0, 0, pos, SI().zScale() );
    const int sticknr = pid.isUdf() ? mUdf(int) : RowCol(pid.subID()).row;

    if ( activestickid_ != sticknr )
	activestickid_ = sticknr;

    if( fsspainter_->hasDiffActiveStick(&pid) )
    {
	fsspainter_->setActiveStick( pid );
	FlatView::Annotation::AuxData* auxdata = fsspainter_->getAuxData(&pid);
	int editorid = editor_->addAuxData( auxdata, true );
	if ( (editorid != activeeditorid_) && (activeeditorid_ != -1) )
	    editor_->removeAuxData( activeeditorid_ );
	editor_->enableEdit( editorid, true, true, true );
	activeeditorid_ = editorid;
    }
}


void FaultStickSetFlatViewEditor::mousePressCB( CallBacker* cb )
{
}


void FaultStickSetFlatViewEditor::mouseReleaseCB( CallBacker* cb )
{
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

    const MouseEvent& mouseevent = meh_->event();
    const uiRect datarect( editor_->getMouseArea() );
    if ( !datarect.isInside(mouseevent.pos()) ) return;

    const uiWorld2Ui w2u( datarect.size(), editor_->getWorldRect(mUdf(int)) );
    const uiWorldPoint wp = w2u.transform( mouseevent.pos()-datarect.topLeft());

    const FlatPosData& pd = dp->posData();
    const IndexInfo ix = pd.indexInfo( true, wp.x );
    const IndexInfo iy = pd.indexInfo( false, wp.y );
    Coord3 pos = dp->getCoord( ix.nearest_, iy.nearest_ );
    pos.z = wp.y;

    EM::FaultStickSetGeometry& fssg = emfss->geometry();

    EM::PosID interactpid;
    fsseditor->getInteractionInfo( interactpid, 0, 0, pos, SI().zScale() );

    if ( mouseevent.shiftStatus() || interactpid.isUdf() )
    {
	Coord3 editnormal( 0, 0, 1 );

	if ( cs_.defaultDir()==CubeSampling::Inl )
	    editnormal = Coord3( SI().binID2Coord().rowDir(), 0 );
	else if ( cs_.defaultDir()==CubeSampling::Crl )
	    editnormal = Coord3( SI().binID2Coord().colDir(), 0 );

	const int sid = emfss->sectionID(0);
	Geometry::FaultStickSet* fss = fssg.sectionGeometry( sid );
	const int insertsticknr = !fss || fss->isEmpty() 
	    			  ? 0 : fss->rowRange().stop+1;

	fssg.insertStick( sid, insertsticknr, 0, pos, editnormal,
			  0, 0, true );
	const EM::SubID subid = RowCol(insertsticknr,0).getSerialized();
	fsseditor->setLastClicked( EM::PosID(emfss->id(),sid,subid) );
    }
    else
    {
	fssg.insertKnot( interactpid.sectionID(), interactpid.subID(), pos,
			 true );
	fsseditor->setLastClicked( interactpid );
    }
}

} // namespace MPE
