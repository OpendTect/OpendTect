/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
________________________________________________________________________

-*/

#include "mpefssflatvieweditor.h"

#include "bendpointfinder.h"
#include "emeditor.h"
#include "emfaultstickset.h"
#include "emfaultstickpainter.h"
#include "emmanager.h"
#include "faultstickseteditor.h"
#include "flatauxdataeditor.h"
#include "mouseevent.h"
#include "mpeengine.h"
#include "randomlinegeom.h"
#include "sorting.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "trigonometry.h"
#include "uiflatviewer.h"

namespace MPE
{

FaultStickSetFlatViewEditor::FaultStickSetFlatViewEditor(
				FlatView::AuxDataEditor* ed,
				const DBKey& oid )
    : EM::FaultStickSetFlatViewEditor(ed)
    , editor_(ed)
    , fsspainter_( new EM::FaultStickPainter(ed->viewer(),oid) )
    , meh_(0)
    , path_(0)
    , rdlid_(-1)
    , activestickid_(-1)
    , seedhasmoved_(false)
    , makenewstick_(false)
    , doubleclicked_(false)
    , mousepid_( EM::PosID::getInvalid() )
{
    fsspainter_->abouttorepaint_.notify(
	    mCB(this,FaultStickSetFlatViewEditor,fssRepaintATSCB) );
    fsspainter_->repaintdone_.notify(
	    mCB(this,FaultStickSetFlatViewEditor,fssRepaintedCB) );
    mAttachCB( editor_->sower().sowingEnd,
	FaultStickSetFlatViewEditor::sowingFinishedCB );
}


FaultStickSetFlatViewEditor::~FaultStickSetFlatViewEditor()
{
    detachAllNotifiers();

    if ( meh_ )
    {
	editor_->movementStarted.remove(
		mCB(this,FaultStickSetFlatViewEditor,seedMovementStartedCB) );
	editor_->movementFinished.remove(
		mCB(this,FaultStickSetFlatViewEditor,seedMovementFinishedCB) );
	editor_->removeSelected.remove(
		mCB(this,FaultStickSetFlatViewEditor,removeSelectionCB) );
	meh_->movement.remove(
		mCB(this,FaultStickSetFlatViewEditor,mouseMoveCB) );
	meh_->buttonPressed.remove(
		mCB(this,FaultStickSetFlatViewEditor,mousePressCB) );
	meh_->buttonReleased.remove(
		mCB(this,FaultStickSetFlatViewEditor,mouseReleaseCB) );
	meh_->doubleClick.remove(
		mCB(this,FaultStickSetFlatViewEditor,doubleClickedCB) );
    }

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
	editor_->movementStarted.remove(
		mCB(this,FaultStickSetFlatViewEditor,seedMovementStartedCB) );
	editor_->movementFinished.remove(
		mCB(this,FaultStickSetFlatViewEditor,seedMovementFinishedCB) );
	editor_->removeSelected.remove(
		mCB(this,FaultStickSetFlatViewEditor,removeSelectionCB) );
	meh_->movement.remove(
		mCB(this,FaultStickSetFlatViewEditor,mouseMoveCB) );
	meh_->buttonPressed.remove(
		mCB(this,FaultStickSetFlatViewEditor,mousePressCB) );
	meh_->buttonReleased.remove(
		mCB(this,FaultStickSetFlatViewEditor,mouseReleaseCB) );
	meh_->doubleClick.remove(
		mCB(this,FaultStickSetFlatViewEditor,doubleClickedCB) );
    }

    meh_ = meh;

    if ( meh_ )
    {
	editor_->movementStarted.notify(
		mCB(this,FaultStickSetFlatViewEditor,seedMovementStartedCB) );
	editor_->movementFinished.notify(
		mCB(this,FaultStickSetFlatViewEditor,seedMovementFinishedCB) );
	editor_->removeSelected.notify(
		mCB(this,FaultStickSetFlatViewEditor,removeSelectionCB) );
	meh_->movement.notify(
		mCB(this,FaultStickSetFlatViewEditor,mouseMoveCB) );
	meh_->buttonPressed.notify(
		mCB(this,FaultStickSetFlatViewEditor,mousePressCB) );
	meh_->buttonReleased.notify(
		mCB(this,FaultStickSetFlatViewEditor,mouseReleaseCB) );
	meh_->doubleClick.notify(
		mCB(this,FaultStickSetFlatViewEditor,doubleClickedCB) );
    }

    for ( int idx=0; idx<markeridinfo_.size(); idx++ )
	editor_->enablePolySel( markeridinfo_[idx]->markerid_, meh_ );
}


void FaultStickSetFlatViewEditor::setTrcKeyZSampling(const TrcKeyZSampling& cs)
{
    EM::FaultStickSetFlatViewEditor::setTrcKeyZSampling( cs );
    fsspainter_->setTrcKeyZSampling( cs, true );
}


void FaultStickSetFlatViewEditor::setPath( const TrcKeyPath& path )
{
    path_ = &path;
    fsspainter_->setPath( path );
}


void FaultStickSetFlatViewEditor::setRandomLineID( int rdlid )
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

    DBKey emid = fsspainter_->getFaultSSID();
    if ( emid.isInvalid() ) return;

    if ( emid != fsspainter_->getFaultSSID() )
	return;

    RefMan<EM::Object> emobject = EM::FSSMan().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss )
	return;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, false );
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

    DBKey emid = fsspainter_->getFaultSSID();
    if ( emid.isInvalid() ) return;

    RefMan<EM::Object> emobject = EM::FSSMan().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss )
	return;

    mDynamicCastGet( const Geometry::FaultStickSet*, fss,
		     emfss->geometryElement() );

    StepInterval<int> colrg = fss->colRange( fsspainter_->getActiveStickId() );
    const int knotid = colrg.start + displayedknotid*colrg.step;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, false );
    mDynamicCastGet( MPE::FaultStickSetEditor*, fsseditor, editor.ptr() );
    if ( !fsseditor )
	return;

    const RowCol knotrc( fsspainter_->getActiveStickId(), knotid );
    const EM::PosID pid = EM::PosID::getFromRowCol( knotrc );
    const Coord3 coord3 = editor_->viewer().getCoord( editor_->getSelPtPos() );
    emfss->setPos( pid, coord3, true );
}


bool FaultStickSetFlatViewEditor::getMousePosInfo(
	const Geom::Point2D<int>& mousepos, Coord3& worldpos, int* trcnr ) const
{
    if ( !mousepos.isDefined() || editor_->getMouseArea().isOutside(mousepos) )
	return false;

    mDynamicCastGet(const uiFlatViewer*,vwr,&editor_->viewer());
    if ( !vwr ) return false;

    Geom::Point2D<double> w2uipos = vwr->getWorld2Ui().transform( mousepos );
    w2uipos.x_ = vwr->posRange( true ).limitValue( w2uipos.x_ );
    w2uipos.y_ = vwr->posRange( false ).limitValue( w2uipos.y_ );
    worldpos = vwr->getCoord( w2uipos );

    if ( trcnr )
    {
	const auto& geom2d = SurvGeom::get2D(fsspainter_->getGeomID());
	*trcnr = geom2d.nearestTracePosition( worldpos.getXY() );
    }

    return true;
}


Coord3 FaultStickSetFlatViewEditor::getScaleVector() const
{
    Coord3 scalevec( 0, 1, SI().zScale() );

    const uiRect datarect( editor_->getMouseArea() );
    Coord3 p0, p1, p2;

    if ( !getMousePosInfo(datarect.bottomLeft(),p0) ||
	 !getMousePosInfo(datarect.bottomRight(),p1) ||
	 !getMousePosInfo(datarect.topLeft(),p2) )
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
	const Coord eu = (p1.getXY()-p0.getXY()) / du;
	const Coord ev = (p2.getXY()-p0.getXY()) / dv;

	const float det = (float) fabs( eu.x_*ev.y_ - eu.y_*ev.x_ );

	const Coord ex(  ev.y_/det, -eu.y_/det );
	const Coord ey( -ev.x_/det,  eu.x_/det );

	scalevec = Coord3( ex.dot(ey)*det, ey.sqAbs()*det, scalevec.z_ );
    }
    else
    {
	float ds = p1.xyDistTo<float>(p2);
	// Assumption: straight in case of 2D line

	scalevec.z_ = fabs( (ds*dv) / (dz*du) );
    }

    return scalevec;
}


Coord3 FaultStickSetFlatViewEditor::getNormal( const Coord3* mousepos ) const
{
    Coord3 normal = Coord3::udf();
    if ( path_ && mousepos )
    {
	const BinID mousebid = SI().transform( mousepos->getXY() );
	TrcKey mousetk( mousebid );
	RefMan<Geometry::RandomLine> rlgeom = Geometry::RLM().get( rdlid_ );
	if ( !rlgeom || !path_ )
	    return Coord3::udf();

	TrcKeyPath nodes;
	rlgeom->getNodePositions( nodes );
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

    DBKey emid = fsspainter_->getFaultSSID();
    if ( emid.isInvalid() ) return;

    RefMan<EM::Object> emobject = EM::FSSMan().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss )
	return;

    if ( emfss->isEmpty() )
	return;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, false );
    mDynamicCastGet( MPE::FaultStickSetEditor*, fsseditor, editor.ptr() );
    if ( !fsseditor )
	return;

    Coord3 pos = Coord3::udf();
    if ( !getMousePosInfo(mouseevent.pos(),pos) )
	return;

    EM::PosID pid;
    Coord3 normal = getNormal( &pos );
    fsseditor->setScaleVector( getScaleVector() );
    fsseditor->getInteractionInfo( pid, 0, 0, fsspainter_->getGeomID(), pos,
				   &normal );

    if ( pid.isInvalid() )
	return;

    const int sticknr = pid.isInvalid() ? mUdf(int) : pid.getRowCol().row();

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

    mousepid_ = EM::PosID::getInvalid();
    int edidauxdataid = editor_->getSelPtDataID();
    int displayedknotid = -1;
    if ( editor_->getSelPtIdx().size() > 0 )
	displayedknotid = editor_->getSelPtIdx()[0];

    DBKey emid = fsspainter_->getFaultSSID();
    if ( emid.isInvalid() ) return;

    RefMan<EM::Object> emobject = EM::FSSMan().getObject( emid );

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
		    emfss->geometryElement());

    RowCol rc;
    rc.row() = stickid;
    const StepInterval<int> colrg = fss->colRange( rc.row() );
    rc.col() = colrg.start + displayedknotid*colrg.step;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, false );
    mDynamicCastGet( MPE::FaultStickSetEditor*, fsseditor, editor.ptr() );
    if ( !fsseditor )
	return;

    EM::PosID mousepid = EM::PosID::getFromRowCol( rc );
    fsseditor->setLastClicked( mousepid );
    activestickid_ = stickid;
    fsspainter_->setActiveStick( mousepid );
    mousepid_ = mousepid;
}


#define mSetUserInteractionEnd() \
    if ( !editor_->sower().moreToSow() ) \
	EM::FSSMan().undo(emid).setUserInteractionEnd( \
				EM::FSSMan().undo(emid).currentEventID() );

void FaultStickSetFlatViewEditor::sowingFinishedCB( CallBacker* cb )
{
    fsspainter_->enablePaint( true );
    fsspainter_->paint();
    makenewstick_ = true;
}


void FaultStickSetFlatViewEditor::mouseReleaseCB( CallBacker* cb )
{
    if ( !editor_->viewer().appearance().annot_.editable_ ||
	 editor_->isSelActive() )
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

    DBKey emid = fsspainter_->getFaultSSID();
    if ( emid.isInvalid() ) return;

    RefMan<EM::Object> emobject = EM::FSSMan().getObject( emid );
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss )
	return;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, false );
    mDynamicCastGet( MPE::FaultStickSetEditor*, fsseditor, editor.ptr() );
    if ( !fsseditor )
	return;

    const MouseEvent& mouseevent = meh_->event();
    Coord3 pos = Coord3::udf();
    Coord3 worldpivot = Coord3::udf();
    getMousePosInfo( editor_->sower().pivotPos(), worldpivot );
    fsseditor->setSowingPivot( worldpivot );
    if ( editor_->sower().accept(mouseevent,true) )
	return;

    int trcnr = -1;
    if ( !getMousePosInfo(mouseevent.pos(),pos,&trcnr) )
	return;

    EM::FaultStickSetGeometry& fssg = emfss->geometry();
    EM::PosID interactpid;
    Coord3 normal = getNormal( &pos );
    fsseditor->setScaleVector( getScaleVector() );
    fsseditor->getInteractionInfo( interactpid, 0, 0, fsspainter_->getGeomID(),
				   pos, &normal );

    if ( !mousepid_.isInvalid() && mouseevent.ctrlStatus()
	 && !mouseevent.shiftStatus() )
    {
	//Remove knot/stick
	const int rmnr = mousepid_.getRowCol().row();
	if ( fssg.nrKnots(rmnr) == 1 )
	{
	    fssg.removeStick( rmnr, true );
	    fsseditor->setLastClicked( EM::PosID::getInvalid() );
	}
	else
	    fssg.removeKnot( mousepid_, true );


	mSetUserInteractionEnd();
	mousepid_ = EM::PosID::getInvalid();
	return;
    }

    if ( !mousepid_.isInvalid() || mouseevent.ctrlStatus() )
	return;

    if ( mouseevent.shiftStatus() || interactpid.isInvalid() || makenewstick_ )
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
				  ? 0 : fss->rowRange().stop+1;

	if ( !geomid.isValid() )
	    fssg.insertStick( insertsticknr, 0, pos, editnormal, true );
	else
	    fssg.insertStick( insertsticknr, 0, pos, editnormal, geomid,
			      true );

	const EM::PosID posid = EM::PosID::getFromRowCol( insertsticknr, 0 );
	fsseditor->setLastClicked( posid );
	mSetUserInteractionEnd();
    }
    else
    {
	fssg.insertKnot( interactpid, pos, true );
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
    RefMan<EM::Object> emobject =
			EM::FSSMan().getObject( fsspainter_->getFaultSSID() );
    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss ) return;

    mDynamicCastGet(const Geometry::FaultStickSet*,
		    fss, emfss->geometryElement() );
    if ( !fss ) return;

    emfss->setBurstAlert( true );

    RowCol rc;
    for ( int ids=selectedids.size()-1; ids>=0; ids-- )
    {
	rc.row() = getStickId( selectedids[ids] );
	const StepInterval<int> colrg = fss->colRange( rc.row() );
	rc.col() = colrg.start + selectedidxs[ids]*colrg.step;
	emfss->geometry().removeKnot( EM::PosID::getFromRowCol(rc), false );
	if ( !emfss->geometry().nrKnots(rc.row()) )
	    emfss->geometry().removeStick( rc.row(), false );
    }

    emfss->setBurstAlert( false );
}


void FaultStickSetFlatViewEditor::set2D( bool yn )
{ fsspainter_->set2D( yn ); }


void FaultStickSetFlatViewEditor::setGeomID( Pos::GeomID geomid )
{ fsspainter_->setGeomID( geomid ); }


TypeSet<int>& FaultStickSetFlatViewEditor::getTrcNos()
{ return fsspainter_->getTrcNos(); }


TypeSet<float>& FaultStickSetFlatViewEditor::getDistances()
{ return fsspainter_->getDistances(); }


TypeSet<Coord>& FaultStickSetFlatViewEditor::getCoords()
{ return fsspainter_->getCoords(); }

} // namespace MPE
