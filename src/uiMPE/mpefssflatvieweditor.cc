/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
#include "keystrs.h"
#include "mouseevent.h"
#include "mousecursor.h"
#include "mpeengine.h"
#include "randomlinegeom.h"
#include "sorting.h"
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
{
    mAttachCB( fsspainter_->abouttorepaint_,
	       FaultStickSetFlatViewEditor::fssRepaintATSCB );
    mAttachCB( fsspainter_->repaintdone_,
	       FaultStickSetFlatViewEditor::fssRepaintedCB );
    mAttachCB( editor_->sower().sowingEnd,
	       FaultStickSetFlatViewEditor::sowingFinishedCB );
}


FaultStickSetFlatViewEditor::~FaultStickSetFlatViewEditor()
{
    detachAllNotifiers();
//	setMouseEventHandler( 0 );
    cleanActStkContainer();
    delete fsspainter_;
    deepErase( markeridinfo_ );
}


void FaultStickSetFlatViewEditor::setMouseEventHandler( MouseEventHandler* meh )
{
    if ( meh_ == meh )
	return;

    if ( meh_ )
    {
	mDetachCB( editor_->movementStarted,
		   FaultStickSetFlatViewEditor::seedMovementStartedCB );
	mDetachCB( editor_->movementFinished,
		   FaultStickSetFlatViewEditor::seedMovementFinishedCB );
	mDetachCB( editor_->removeSelected,
		   FaultStickSetFlatViewEditor::removeSelectionCB );
	mDetachCB( meh_->movement,
		   FaultStickSetFlatViewEditor::mouseMoveCB );
	mDetachCB( meh_->buttonPressed,
		   FaultStickSetFlatViewEditor::mousePressCB );
	mDetachCB( meh_->buttonReleased,
		   FaultStickSetFlatViewEditor::mouseReleaseCB );
	mDetachCB( meh_->doubleClick,
		   FaultStickSetFlatViewEditor::doubleClickedCB );
    }

    meh_ = meh;

    if ( meh_ )
    {
	mAttachCB( editor_->movementStarted,
		   FaultStickSetFlatViewEditor::seedMovementStartedCB );
	mAttachCB( editor_->movementFinished,
		   FaultStickSetFlatViewEditor::seedMovementFinishedCB );
	mAttachCB( editor_->removeSelected,
		   FaultStickSetFlatViewEditor::removeSelectionCB );
	mAttachCB( meh_->movement,
		   FaultStickSetFlatViewEditor::mouseMoveCB );
	mAttachCB( meh_->buttonPressed,
		   FaultStickSetFlatViewEditor::mousePressCB );
	mAttachCB( meh_->buttonReleased,
		   FaultStickSetFlatViewEditor::mouseReleaseCB );
	mAttachCB( meh_->doubleClick,
		   FaultStickSetFlatViewEditor::doubleClickedCB );
    }

    for ( int idx=0; idx<markeridinfo_.size(); idx++ )
	editor_->enablePolySel( markeridinfo_[idx]->markerid_, meh_ );
}


void FaultStickSetFlatViewEditor::setTrcKeyZSampling(const TrcKeyZSampling& cs)
{
    EM::FaultStickSetFlatViewEditor::setTrcKeyZSampling( cs );
    fsspainter_->setTrcKeyZSampling( cs, true );
}


void FaultStickSetFlatViewEditor::setPath( const TrcKeySet& path )
{
    path_ = &path;
    fsspainter_->setPath( path );
}


void FaultStickSetFlatViewEditor::setRandomLineID( const RandomLineID& rdlid )
{
    rdlid_ = rdlid;
    fsspainter_->setRandomLineID( rdlid );
}


void FaultStickSetFlatViewEditor::setFlatPosData( const FlatPosData* fpd )
{
    fsspainter_->setFlatPosData( fpd );
}


void FaultStickSetFlatViewEditor::drawFault()
{
    fsspainter_->paint();
    updateActStkContainer();
}


void FaultStickSetFlatViewEditor::enableLine( bool yn )
{
    fsspainter_->enableLine( yn );
}


void FaultStickSetFlatViewEditor::enableKnots( bool yn )
{
    fsspainter_->enableKnots( yn );
}


void FaultStickSetFlatViewEditor::updateActStkContainer()
{
    cleanActStkContainer();
    fillActStkContainer();
}


void FaultStickSetFlatViewEditor::cleanActStkContainer()
{
    for ( int idx=0; idx<markeridinfo_.size(); idx++ )
	editor_->removeAuxData( markeridinfo_[idx]->markerid_ );

    if ( markeridinfo_.size() )
	deepErase( markeridinfo_ );
}


void FaultStickSetFlatViewEditor::fillActStkContainer()
{
    ObjectSet<EM::FaultStickPainter::StkMarkerInfo> dispdstkmrkinfos;
    fsspainter_->getDisplayedSticks( dispdstkmrkinfos );

    for ( int idx=0; idx<dispdstkmrkinfos.size(); idx++ )
    {
	StkMarkerIdInfo* markeridinfo = new  StkMarkerIdInfo;
	markeridinfo->markerid_ = editor_->addAuxData(
			    dispdstkmrkinfos[idx]->marker_, true );
	markeridinfo->stickid_ = dispdstkmrkinfos[idx]->stickid_;
	editor_->enableEdit( markeridinfo->markerid_, false, true, false );
	editor_->enablePolySel( markeridinfo->markerid_, meh_ );

	markeridinfo_ += markeridinfo;
    }
}


void FaultStickSetFlatViewEditor::fssRepaintATSCB( CallBacker* )
{
    cleanActStkContainer();
}


void FaultStickSetFlatViewEditor::fssRepaintedCB( CallBacker* )
{
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
	if ( markeridinfo_[idx]->markerid_ == edidauxdataid )
	{
	    selstickid = markeridinfo_[idx]->stickid_;
	    break;
	}
    }

    if ( selstickid == -1 )
	return;

    if ( selstickid == fsspainter_->getActiveStickId() )
	return;

    EM::ObjectID emid = fsspainter_->getFaultSSID();
    if ( !emid.isValid() )
	return;

    if ( emid != fsspainter_->getFaultSSID() )
	return;

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss )
	return;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditorByID( emid );
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

    Coord3 realpos = editor_->viewer().getCoord( pos );
    realpos.z_ = (!tkzs_.isEmpty() && tkzs_.nrZ() == 1) ? tkzs_.zsamp_.start_
                                                        : pos.y_;

    EM::ObjectID emid = fsspainter_->getFaultSSID();
    if ( !emid.isValid() )
	return;

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss )
	return;

    mDynamicCastGet(const Geometry::FaultStickSet*,fss,
		    emfss->geometryElement())

    StepInterval<int> colrg = fss->colRange( fsspainter_->getActiveStickId() );
    const int knotid = colrg.start_ + displayedknotid*colrg.step_;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditorByID( emid );
    mDynamicCastGet( MPE::FaultStickSetEditor*, fsseditor, editor.ptr() );
    if ( !fsseditor )
	return;

    const RowCol knotrc( fsspainter_->getActiveStickId(), knotid );
    const EM::PosID pid( emid, knotrc );
    emfss->setPos( pid, realpos, true );
}


bool FaultStickSetFlatViewEditor::getMousePosInfo(
	    const Geom::Point2D<int>& mousepos, IndexInfo& ix, IndexInfo& iy,
	    Coord3& worldpos, int* trcnr ) const
{
    ConstRefMan<FlatDataPack> dp =
			      editor_->viewer().getPack( false, true ).get();
    if ( !dp )
	return false;

    const uiRect datarect( editor_->getMouseArea() );
    if ( !mousepos.isDefined() || datarect.isOutside(mousepos) )
	return false;

    const uiWorld2Ui w2u( datarect.size(), editor_->getWorldRect(mUdf(int)) );
    const uiWorldPoint wp = w2u.transform( mousepos-datarect.topLeft() );

    const FlatPosData& pd = dp->posData();
    ix = pd.indexInfo( true, wp.x_ );
    iy = pd.indexInfo( false, wp.y_ );

    worldpos = editor_->viewer().getCoord( wp );
    worldpos.z_ = ( !tkzs_.isEmpty() && tkzs_.nrZ() == 1) ? tkzs_.zsamp_.start_
                                                          : wp.y_;

    if ( trcnr )
    {
	const TrcKey tk = dp->getTrcKey( ix.nearest_, iy.nearest_ );
	if ( !tk.is3D() )
	    *trcnr = tk.trcNr();
    }

    return true;
}


Coord3 FaultStickSetFlatViewEditor::getScaleVector() const
{
    Coord3 scalevec( 0, 1, SI().zScale() );

    const uiRect datarect( editor_->getMouseArea() );
    IndexInfo ix(0), iy(0);
    Coord3 p0, p1, p2;

    if ( !getMousePosInfo(datarect.bottomLeft(),  ix, iy, p0) ||
	 !getMousePosInfo(datarect.bottomRight(), ix, iy, p1) ||
	 !getMousePosInfo(datarect.topLeft(),     ix, iy, p2) )
    {
	return scalevec;
    }

    const int du = datarect.topLeft().x_ - datarect.bottomRight().x_;
    const int dv = datarect.topLeft().y_ - datarect.bottomRight().y_;
    if ( !du || !dv )
	return scalevec;

    const float dz = (float) ( p2.z_ - p1.z_ );

    if ( mIsZero(dz,mDefEps) )	// z-slice
    {
	const Coord eu = (p1-p0) / du;
	const Coord ev = (p2-p0) / dv;

        const float det = (float) fabs( eu.x_*ev.y_ - eu.y_*ev.x_ );

        const Coord ex(  ev.y_/det, -eu.y_/det );
        const Coord ey( -ev.x_/det,  eu.x_/det );

        scalevec = Coord3( ex.dot(ey)*det, ey.sqAbs()*det, scalevec.z_ );
    }
    else
    {
	float ds = (float) Coord(p1).distTo(p2);
	// Assumption: straight in case of 2D line

        scalevec.z_ = fabs( (ds*dv) / (dz*du) );
    }

    return scalevec;
}


Coord3 FaultStickSetFlatViewEditor::getNormal( const Coord3* mousepos ) const
{
    Coord3 normal = Coord3( Coord3::udf() );
    if ( path_ && mousepos )
    {
	const BinID mousebid = SI().transform( *mousepos );
	TrcKey mousetk( mousebid );
	RefMan<Geometry::RandomLine> rlgeom = Geometry::RLM().get( rdlid_ );
	if ( !rlgeom || !path_ )
	    return Coord3::udf();

	TrcKeySet nodes;
	rlgeom->allNodePositions( nodes );
	return Coord3( Geometry::RandomLine::getNormal(nodes,mousetk), 0.0f );
    }
    else if ( !tkzs_.isEmpty() )
	tkzs_.getDefaultNormal( normal );

    return normal;
}


void FaultStickSetFlatViewEditor::mouseMoveCB( CallBacker* cb )
{
    if ( seedhasmoved_ )
	return;

    const MouseEvent& mouseevent = meh_->event();
    if ( editor_ && editor_->sower().accept(mouseevent, false) )
	return;

    EM::ObjectID emid = fsspainter_->getFaultSSID();
    if ( !emid.isValid() )
	return;

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr())
    if ( !emfss )
	return;

    if ( emfss->isEmpty() )
	return;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditorByID( emid );
    mDynamicCastGet(MPE::FaultStickSetEditor*,fsseditor,editor.ptr())
    if ( !fsseditor )
	return;

    IndexInfo ix(0), iy(0); Coord3 pos;
    if ( !getMousePosInfo(mouseevent.pos(), ix, iy, pos) )
	return;

    EM::PosID pid;
    Coord3 normal = getNormal( &pos );
    fsseditor->setScaleVector( getScaleVector() );
    fsseditor->getInteractionInfo( pid, 0, 0, fsspainter_->getGeomID(), pos,
				   &normal );

    if ( pid.isUdf() )
	return;

    const int sticknr = pid.isUdf() ? mUdf(int) : pid.getRowCol().row();

    if ( activestickid_ != sticknr )
	activestickid_ = sticknr;

    if( fsspainter_->hasDiffActiveStick(&pid) )
	fsspainter_->setActiveStick( pid );
}


void FaultStickSetFlatViewEditor::mousePressCB( CallBacker* cb )
{
    if ( (editor_ && editor_->sower().accept(meh_->event(), false)) ||
	  meh_->event().middleButton() )
	return;

    if ( !editor_->viewer().appearance().annot_.editable_
	 || editor_->isSelActive() )
	return;

    mousepid_ = EM::PosID::udf();
    int edidauxdataid = editor_->getSelPtDataID();
    int displayedknotid = -1;
    if ( editor_->getSelPtIdx().size() > 0 )
	displayedknotid = editor_->getSelPtIdx()[0];

    EM::ObjectID emid = fsspainter_->getFaultSSID();
    if ( !emid.isValid() )
	return;

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );

    if ( (edidauxdataid==-1) || (displayedknotid==-1) )
    {
	editor_->sower().reInitSettings();
	editor_->sower().alternateSowingOrder();
	editor_->sower().setIfDragInvertMask();
	editor_->sower().activate( emobject->preferredColor(), meh_->event() );
	return;
    }

    int stickid = getStickId( edidauxdataid );
    if ( stickid == -1 ) return;

    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss ) return;

    mDynamicCastGet(const Geometry::FaultStickSet*,fss,
		    emfss->geometryElement())

    RowCol rc;
    rc.row() = stickid;
    const StepInterval<int> colrg = fss->colRange( rc.row() );
    rc.col() = colrg.start_ + displayedknotid*colrg.step_;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditorByID( emid );
    mDynamicCastGet( MPE::FaultStickSetEditor*, fsseditor, editor.ptr() );
    if ( !fsseditor )
	return;

    EM::PosID mousepid( emid, rc );
    fsseditor->setLastClicked( mousepid );
    activestickid_ = stickid;
    fsspainter_->setActiveStick( mousepid );
    mousepid_ = mousepid;
}


#define mSetUserInteractionEnd() \
    if ( !editor_->sower().moreToSow() ) \
	EM::EMM().undo(emfss->id()).setUserInteractionEnd( \
	    EM::EMM().undo(emfss->id()).currentEventID() );


void FaultStickSetFlatViewEditor::sowingFinishedCB( CallBacker* )
{
    fsspainter_->enablePaint( true );
    fsspainter_->paint();
    makenewstick_ = true;
}


void FaultStickSetFlatViewEditor::mouseReleaseCB( CallBacker* cb )
{
    if ( !editor_->viewer().appearance().annot_.editable_
	 || editor_->isSelActive() )
	return;

    mDynamicCastGet(MouseEventHandler*,meh,cb);
    if ( !meh )
	return;

    const MouseEvent& mev = meh->event();
    if ( !mev.leftButton() )
	return;

    if ( doubleclicked_ )
    {
	doubleclicked_ = false;
	return;
    }

    if ( seedhasmoved_ )
    {
	seedhasmoved_ = false;
	return;
    }

    EM::ObjectID emid = fsspainter_->getFaultSSID();
    if ( !emid.isValid() )
	return;

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss )
	return;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditorByID( emid );
    mDynamicCastGet( MPE::FaultStickSetEditor*, fsseditor, editor.ptr() );
    if ( !fsseditor )
	return;

    const MouseEvent& mouseevent = meh_->event();
    IndexInfo ix(0), iy(0); Coord3 pos;

    Coord3 worldpivot = Coord3::udf();
    getMousePosInfo( editor_->sower().pivotPos(), ix, iy, worldpivot );
    fsseditor->setSowingPivot( worldpivot );
    if ( editor_->sower().accept(mouseevent,true) )
	return;

    int trcnr = -1;
    if ( !getMousePosInfo(mouseevent.pos(), ix, iy, pos, &trcnr) )
	return;

    EM::FaultStickSetGeometry& fssg = emfss->geometry();
    EM::PosID interactpid;
    Coord3 normal = getNormal( &pos );
    fsseditor->setScaleVector( getScaleVector() );
    fsseditor->getInteractionInfo( interactpid, 0, 0, fsspainter_->getGeomID(),
				   pos, &normal );

    if ( !mousepid_.isUdf() && mouseevent.ctrlStatus()
	 && !mouseevent.shiftStatus() )
    {
	//Remove knot/stick
	const int rmnr = mousepid_.getRowCol().row();
	if ( fssg.nrKnots(rmnr) == 1 )
	{
	    fssg.removeStick( rmnr, true );
	    fsseditor->setLastClicked( EM::PosID::udf() );
	}
	else
	    fssg.removeKnot( mousepid_.subID(), true );


	mSetUserInteractionEnd();
	mousepid_ = EM::PosID::udf();
	return;
    }

    if ( !mousepid_.isUdf() || mouseevent.ctrlStatus() )
	return;

    if ( mouseevent.shiftStatus() || interactpid.isUdf() || makenewstick_ )
    {
	if ( editor_->sower().moreToSow() )
	    fsspainter_->enablePaint( false );
	else
	    fsspainter_->enablePaint( true );

	makenewstick_ = false;
	Coord3 editnormal = getNormal( &pos );

	Pos::GeomID geomid;
	if ( tkzs_.isEmpty() && editnormal.isUdf() )
	{
	    geomid = fsspainter_->getGeomID();
	    editnormal = Coord3( fsspainter_->getNormalToTrace(trcnr), 0 );
	}

	Geometry::FaultStickSet* fss = fssg.geometryElement();
	const int insertsticknr = !fss || fss->isEmpty()
				  ? 0 : fss->rowRange().stop_+1;

	if ( geomid.is2D() )
	    fssg.insertStick( insertsticknr, 0, pos, editnormal, geomid,
			      true );
	else
	    fssg.insertStick( insertsticknr, 0, pos, editnormal, true );

	const EM::SubID subid = RowCol(insertsticknr,0).toInt64();
	fsseditor->setLastClicked( EM::PosID(emfss->id(),subid) );
	mSetUserInteractionEnd();
    }
    else
    {
	fssg.insertKnot( interactpid.subID(), pos, true );
	fsseditor->setLastClicked( interactpid );
	mSetUserInteractionEnd();
    }
}


void FaultStickSetFlatViewEditor::doubleClickedCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,meh,cb);
    if ( !meh )
	return;

    const MouseEvent& mev = meh->event();
    if ( !mev.leftButton() )
	return;

    makenewstick_ = true;
    doubleclicked_ = true;
}


int FaultStickSetFlatViewEditor::getStickId( int markerid ) const
{
    if ( !markeridinfo_.size() )
	return mUdf( int );

    for ( int stkmkridx=0; stkmkridx<markeridinfo_.size(); stkmkridx++ )
    {
	if ( markeridinfo_[stkmkridx]->markerid_ == markerid )
	{
	    return markeridinfo_[stkmkridx]->stickid_;
	}
    }

    return mUdf( int );
}


void FaultStickSetFlatViewEditor::removeSelectionCB( CallBacker* cb )
{
    TypeSet<int> selectedids;
    TypeSet<int> selectedidxs;
    editor_->getPointSelections( selectedids, selectedidxs );
    if ( !selectedids.size() ) return;

    sort_coupled( selectedidxs.arr(), selectedids.arr(), selectedids.size() );
    RefMan<EM::EMObject> emobject =
			EM::EMM().getObject( fsspainter_->getFaultSSID() );
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss ) return;

    mDynamicCastGet(const Geometry::FaultStickSet*,
		    fss,emfss->geometryElement())
    if ( !fss ) return;

    emfss->setBurstAlert( true );

    RowCol rc;
    for ( int ids=selectedids.size()-1; ids>=0; ids-- )
    {
	rc.row() = getStickId( selectedids[ids] );
	const StepInterval<int> colrg = fss->colRange( rc.row() );
	rc.col() = colrg.start_ + selectedidxs[ids]*colrg.step_;
	emfss->geometry().removeKnot( rc.toInt64(), false );
	if ( !emfss->geometry().nrKnots(rc.row()) )
	    emfss->geometry().removeStick( rc.row(), false );
    }

    emfss->setBurstAlert( false );
}


void FaultStickSetFlatViewEditor::set2D( bool yn )
{ fsspainter_->set2D( yn ); }


void FaultStickSetFlatViewEditor::setGeomID( const Pos::GeomID& geomid )
{ fsspainter_->setGeomID( geomid ); }


TypeSet<int>& FaultStickSetFlatViewEditor::getTrcNos()
{ return fsspainter_->getTrcNos(); }


TypeSet<float>& FaultStickSetFlatViewEditor::getDistances()
{ return fsspainter_->getDistances(); }


TypeSet<Coord>& FaultStickSetFlatViewEditor::getCoords()
{ return fsspainter_->getCoords(); }

} // namespace MPE
