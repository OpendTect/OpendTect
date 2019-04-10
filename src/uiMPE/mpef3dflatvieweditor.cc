
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2010
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
#include "mpeengine.h"
#include "randomlinegeom.h"
#include "sorting.h"
#include "survinfo.h"
#include "uiflatviewer.h"

namespace MPE
{

Fault3DFlatViewEditor::Fault3DFlatViewEditor(
			    FlatView::AuxDataEditor* ed,
			    const DBKey& oid )
    : EM::FaultStickSetFlatViewEditor(ed)
    , editor_(ed)
    , f3dpainter_( new EM::Fault3DPainter(ed->viewer(),oid) )
    , meh_(0)
    , activestickid_(mUdf(int))
    , path_(0)
    , seedhasmoved_(false)
    , makenewstick_(false)
    , doubleclicked_(false)
    , mousepid_( EM::PosID::getInvalid() )
{
    f3dpainter_->abouttorepaint_.notify(
	    mCB(this,Fault3DFlatViewEditor,f3dRepaintATSCB) );
    f3dpainter_->repaintdone_.notify(
	    mCB(this,Fault3DFlatViewEditor,f3dRepaintedCB) );
    mAttachCB( editor_->sower().sowingEnd,
	Fault3DFlatViewEditor::sowingFinishedCB );
}


Fault3DFlatViewEditor::~Fault3DFlatViewEditor()
{
    detachAllNotifiers();

    if ( meh_ )
    {
	editor_->movementStarted.remove(
		mCB(this,Fault3DFlatViewEditor,seedMovementStartedCB) );
	editor_->movementFinished.remove(
		mCB(this,Fault3DFlatViewEditor,seedMovementFinishedCB) );
	editor_->removeSelected.remove(
		mCB(this,Fault3DFlatViewEditor,removeSelectionCB) );
	meh_->movement.remove(
		mCB(this,Fault3DFlatViewEditor,mouseMoveCB) );
	meh_->buttonPressed.remove(
		mCB(this,Fault3DFlatViewEditor,mousePressCB) );
	meh_->buttonReleased.remove(
		mCB(this,Fault3DFlatViewEditor,mouseReleaseCB) );
	meh_->doubleClick.remove(
		mCB(this,Fault3DFlatViewEditor,doubleClickedCB) );
    }

    cleanActStkContainer();
    delete f3dpainter_;
    deepErase( markeridinfo_ );
}


void Fault3DFlatViewEditor::setMouseEventHandler( MouseEventHandler* meh )
{
    if ( meh_ == meh )
	return;

    if ( meh_ )
    {
	editor_->movementStarted.remove(
		mCB(this,Fault3DFlatViewEditor,seedMovementStartedCB) );
	editor_->movementFinished.remove(
		mCB(this,Fault3DFlatViewEditor,seedMovementFinishedCB) );
	editor_->removeSelected.remove(
		mCB(this,Fault3DFlatViewEditor,removeSelectionCB) );
	meh_->movement.remove(
		mCB(this,Fault3DFlatViewEditor,mouseMoveCB) );
	meh_->buttonPressed.remove(
		mCB(this,Fault3DFlatViewEditor,mousePressCB) );
	meh_->buttonReleased.remove(
		mCB(this,Fault3DFlatViewEditor,mouseReleaseCB) );
	meh_->doubleClick.remove(
		mCB(this,Fault3DFlatViewEditor,doubleClickedCB) );
    }

    meh_ = meh;

    if ( meh_ )
    {
	editor_->movementStarted.notify(
		mCB(this,Fault3DFlatViewEditor,seedMovementStartedCB) );
	editor_->movementFinished.notify(
		mCB(this,Fault3DFlatViewEditor,seedMovementFinishedCB) );
	editor_->removeSelected.notify(
		mCB(this,Fault3DFlatViewEditor,removeSelectionCB) );
	meh_->movement.notify(
		mCB(this,Fault3DFlatViewEditor,mouseMoveCB) );
	meh_->buttonPressed.notify(
		mCB(this,Fault3DFlatViewEditor,mousePressCB) );
	meh_->buttonReleased.notify(
		mCB(this,Fault3DFlatViewEditor,mouseReleaseCB) );
	meh_->doubleClick.notify(
		mCB(this,Fault3DFlatViewEditor,doubleClickedCB) );
    }

    for ( int idx=0; idx<markeridinfo_.size(); idx++ )
	editor_->enablePolySel( markeridinfo_[idx]->markerid_, meh_ );
}


void Fault3DFlatViewEditor::setTrcKeyZSampling( const TrcKeyZSampling& cs )
{
    EM::FaultStickSetFlatViewEditor::setTrcKeyZSampling( cs );
    f3dpainter_->setTrcKeyZSampling( cs, true );
}


void Fault3DFlatViewEditor::setPath( const TrcKeyPath& path )
{
    path_ = &path;
    f3dpainter_->setPath( path );
}


void Fault3DFlatViewEditor::setRandomLineID( int rdlid )
{
    rdlid_ = rdlid;
    f3dpainter_->setRandomLineID( rdlid );
}


void Fault3DFlatViewEditor::setFlatPosData( const FlatPosData* fpd )
{
    f3dpainter_->setFlatPosData( fpd );
}


void Fault3DFlatViewEditor::drawFault()
{
    f3dpainter_->paint();
    updateActStkContainer();
}


void Fault3DFlatViewEditor::enableLine( bool yn )
{
    f3dpainter_->enableLine( yn );
}


void Fault3DFlatViewEditor::enableKnots( bool yn )
{
    f3dpainter_->enableKnots( yn );
}


void Fault3DFlatViewEditor::f3dRepaintATSCB( CallBacker* )
{
    cleanActStkContainer();
}


void Fault3DFlatViewEditor::f3dRepaintedCB( CallBacker* )
{
    fillActStkContainer();
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
	if ( markeridinfo_[idx]->markerid_ == edidauxdataid )
	{
	    selstickid = markeridinfo_[idx]->stickid_;
	    break;
	}
    }

    if ( selstickid == mUdf(int) )
	return;

    if ( selstickid == f3dpainter_->getActiveStickId() )
	return;

    DBKey emid = f3dpainter_->getFaultID();
    if ( emid.isInvalid() ) return;

    RefMan<EM::Object> emobject = EM::Flt3DMan().getObject( emid );
    mDynamicCastGet(EM::Fault3D*,emf3d,emobject.ptr());
    if ( !emf3d )
	return;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, false );
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

    DBKey emid = f3dpainter_->getFaultID();
    if ( emid.isInvalid()  ) return;

    RefMan<EM::Object> emobject = EM::Flt3DMan().getObject( emid );
    mDynamicCastGet(EM::Fault3D*,emf3d,emobject.ptr());
    if ( !emf3d )
	return;

    mDynamicCastGet( const Geometry::FaultStickSet*, emfss,
		     emf3d->geometryElement() );
    if ( !emfss ) return;

    StepInterval<int> colrg = emfss->colRange( f3dpainter_->getActiveStickId());
    const int knotid = colrg.start + displayedknotid*colrg.step;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, false );
    mDynamicCastGet( MPE::FaultEditor*, f3deditor, editor.ptr() );
    if ( !f3deditor )
	return;

    const EM::PosID pid =
	EM::PosID::getFromRowCol( f3dpainter_->getActiveStickId(), knotid );
    const Coord3 coord3 = editor_->viewer().getCoord( editor_->getSelPtPos() );
    emf3d->setPos( pid, coord3, true );
}


bool Fault3DFlatViewEditor::getMousePosInfo( const Geom::Point2D<int>& mousepos,
					     Coord3& worldpos ) const
{
    if ( !mousepos.isDefined() || editor_->getMouseArea().isOutside(mousepos) )
	return false;

    mDynamicCastGet(const uiFlatViewer*,vwr,&editor_->viewer());
    if ( !vwr ) return false;

    Geom::Point2D<double> w2uipos = vwr->getWorld2Ui().transform( mousepos );
    w2uipos.x_ = vwr->posRange( true ).limitValue( w2uipos.x_ );
    w2uipos.y_ = vwr->posRange( false ).limitValue( w2uipos.y_ );
    worldpos = vwr->getCoord( w2uipos );
    return true;
}


Coord3 Fault3DFlatViewEditor::getScaleVector() const
{
    Coord3 scalevec( 0, 1, SI().zScale() );
    Coord3 p0, p1, p2;
    const uiRect datarect( editor_->getMouseArea() );
    if ( !getMousePosInfo(datarect.bottomLeft(), p0) ||
	 !getMousePosInfo(datarect.bottomRight(),p1) ||
	 !getMousePosInfo(datarect.topLeft(),p2) )
	return scalevec;

    const int du = datarect.topLeft().x_ - datarect.bottomRight().x_;
    const int dv = datarect.topLeft().y_ - datarect.bottomRight().y_;
    if ( !du || !dv )
	return scalevec;

    const float dz = (float) ( p2.z_ - p1.z_ );

    if ( mIsZero(dz,mDefEps) )	// z-slice
    {
	const Coord eu = (p1.getXY()-p0.getXY()) / du;
	const Coord ev = (p2.getXY()-p0.getXY()) / dv;

	const float det = (float) ( fabs( eu.x_*ev.y_ - eu.y_*ev.x_ ) );

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


#define mGetNormal(var) \
    Coord3 var( Coord3::udf() ); \
    if ( !tkzs_.isEmpty() ) tkzs_.getDefaultNormal( var )



Coord3 Fault3DFlatViewEditor::getNormal( const Coord3* mousepos ) const
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


void Fault3DFlatViewEditor::mouseMoveCB( CallBacker* )
{
    if ( seedhasmoved_ )
	return;

    const MouseEvent& mouseevent = meh_->event();
    if ( editor_ && editor_->sower().accept(mouseevent, false) )
	return;

    DBKey emid = f3dpainter_->getFaultID();
    if ( emid.isInvalid() ) return;

    RefMan<EM::Object> emobject = EM::Flt3DMan().getObject( emid );
    mDynamicCastGet(EM::Fault3D*,emf3d,emobject.ptr());
    if ( !emf3d )
	return;

    if ( emf3d->isEmpty() )
	return;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, false );
    mDynamicCastGet( MPE::FaultEditor*, f3deditor, editor.ptr() );
    if ( !f3deditor )
	return;

    Coord3 pos = Coord3::udf();
    if ( !getMousePosInfo(mouseevent.pos(),pos) )
	return;

    bool shdmakenewstick = false;
    EM::PosID pid;
    Coord3 normal = getNormal( &pos );
    f3deditor->setScaleVector( getScaleVector() );
    f3deditor->getInteractionInfo( shdmakenewstick, pid, pos, &normal );

    if ( pid.isInvalid() || shdmakenewstick )
	return;

    const int sticknr = pid.isInvalid() ? mUdf(int) : pid.getRowCol().row();

    if ( activestickid_ != sticknr )
	activestickid_ = sticknr;

    if( f3dpainter_->hasDiffActiveStick(&pid) )
	f3dpainter_->setActiveStick( pid );
}


void Fault3DFlatViewEditor::mousePressCB( CallBacker* )
{
    if ( (editor_ && editor_->sower().accept(meh_->event(),false)) ||
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

    DBKey emid = f3dpainter_->getFaultID();
    if ( emid.isInvalid() ) return;

    RefMan<EM::Object> emobject = EM::Flt3DMan().getObject( emid );

    if ( (edidauxdataid==-1) || (displayedknotid==-1) )
    {
	editor_->sower().reInitSettings();
	editor_->sower().alternateSowingOrder();
	editor_->sower().setIfDragInvertMask();
	editor_->sower().activate( emobject->preferredColor(), meh_->event() );
	return;
    }


    int stickid = mUdf(int);

    for ( int idx=0; idx<markeridinfo_.size(); idx++ )
    {
	if ( markeridinfo_[idx]->markerid_ == edidauxdataid )
	{
	    stickid = markeridinfo_[idx]->stickid_;
	    break;
	}
    }

    if ( stickid == mUdf(int) ) return;

    mDynamicCastGet(EM::Fault3D*,emf3d,emobject.ptr());
    if ( !emf3d ) return;

    mDynamicCastGet( const Geometry::FaultStickSet*, fss,
		     emf3d->geometryElement() );

    RowCol rc;
    rc.row() = stickid;
    int knotid = mUdf(int);

    StepInterval<int> colrg = fss->colRange( rc.row() );
    knotid = colrg.start + displayedknotid*colrg.step;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, false );
    mDynamicCastGet( MPE::FaultEditor*, f3deditor, editor.ptr() );
    if ( !f3deditor )
	return;

    EM::PosID mousepid = EM::PosID::getFromRowCol( stickid, knotid );
    f3deditor->setLastClicked( mousepid );
    activestickid_ = stickid;
    f3dpainter_->setActiveStick( mousepid );
    mousepid_ = mousepid;
}

#define mSetUserInteractionEnd() \
    if ( !editor_->sower().moreToSow() ) \
	EM::Flt3DMan().undo(emid).setUserInteractionEnd( \
				EM::Flt3DMan().undo(emid).currentEventID() );

void Fault3DFlatViewEditor::doubleClickedCB( CallBacker* cb )
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


void Fault3DFlatViewEditor::sowingFinishedCB( CallBacker* cb )
{
    f3dpainter_->enablePaint( true );
    f3dpainter_->paint();
    makenewstick_ = true;
}


void Fault3DFlatViewEditor::mouseReleaseCB( CallBacker* cb )
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

    DBKey emid = f3dpainter_->getFaultID();
    if ( emid.isInvalid() ) return;

    RefMan<EM::Object> emobject = EM::Flt3DMan().getObject( emid );
    mDynamicCastGet(EM::Fault3D*,emf3d,emobject.ptr());
    if ( !emf3d )
	return;

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid, false );
    mDynamicCastGet( MPE::FaultEditor*, f3deditor, editor.ptr() );
    if ( !f3deditor )
	return;

    Coord3 pos = Coord3::udf();
    Coord3 worldpivot = Coord3::udf();
    getMousePosInfo( editor_->sower().pivotPos(), worldpivot );
    f3deditor->setSowingPivot( worldpivot );
    const MouseEvent& mouseevent = meh_->event();
    if ( editor_->sower().accept(mouseevent,true) )
	return;

    if ( !getMousePosInfo(mouseevent.pos(),pos) )
	return;

    bool makenewstick =
	(!mouseevent.ctrlStatus() && mouseevent.shiftStatus()) || makenewstick_;
    EM::PosID interactpid;
    Coord3 normal = getNormal( &pos );
    f3deditor->setScaleVector( getScaleVector() );
    f3deditor->getInteractionInfo( makenewstick, interactpid, pos, &normal );

    if ( !mousepid_.isInvalid() && mouseevent.ctrlStatus()
	 && !mouseevent.shiftStatus() )
    {
	//Remove knot/stick
	bool res;
	const int rmnr = mousepid_.getRowCol().row();
	if ( emf3d->geometry().nrKnots(rmnr) == 1 )
	{
	    res = emf3d->geometry().removeStick( rmnr, true );
	    f3deditor->setLastClicked( EM::PosID::getInvalid() );
	}
	else
	    res = emf3d->geometry().removeKnot( mousepid_, true );

	if ( res )
	    mSetUserInteractionEnd();

	mousepid_ = EM::PosID::getInvalid();
	return;
    }

    if ( !mousepid_.isInvalid() || interactpid.isInvalid() ||
		mouseevent.ctrlStatus() )
	return;

    if ( makenewstick )
    {
	if ( editor_->sower().moreToSow() )
	    f3dpainter_->enablePaint( false );
	else
	    f3dpainter_->enablePaint( true );

	makenewstick_ = false;
	Coord3 editnormal = getNormal( &pos );
	if ( editnormal.isUdf() ) return;

	const int insertsticknr = interactpid.isInvalid()
	    ? mUdf( int )
	    : interactpid.getRowCol().row();

	if ( emf3d->geometry().insertStick(insertsticknr,0,pos,editnormal,true))
	{
	    mSetUserInteractionEnd();
	    f3deditor->setLastClicked( interactpid );
	    f3deditor->editpositionchange.trigger();
	}
    }
    else
    {
	if ( emf3d->geometry().insertKnot(interactpid,pos,true) )
	{
	    mSetUserInteractionEnd();
	    f3deditor->setLastClicked( interactpid );
	    f3deditor->editpositionchange.trigger();
	}
    }
}


void Fault3DFlatViewEditor::cleanActStkContainer()
{
    for ( int idx=0; idx<markeridinfo_.size(); idx++ )
	editor_->removeAuxData( markeridinfo_[idx]->markerid_ );

    if ( markeridinfo_.size() )
	deepErase( markeridinfo_ );
}


void Fault3DFlatViewEditor::updateActStkContainer()
{
    cleanActStkContainer();
    fillActStkContainer();
}


void Fault3DFlatViewEditor::fillActStkContainer()
{
    ObjectSet<EM::Fault3DPainter::StkMarkerInfo> dispdstkmrkinfos;
    f3dpainter_->getDisplayedSticks( dispdstkmrkinfos );

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


int Fault3DFlatViewEditor::getStickId( int markerid ) const
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


void Fault3DFlatViewEditor::removeSelectionCB( CallBacker* cb )
{
    TypeSet<int> selectedids;
    TypeSet<int> selectedidxs;
    editor_->getPointSelections( selectedids, selectedidxs );
    if ( !selectedids.size() ) return;

    sort_coupled( selectedidxs.arr(), selectedids.arr(), selectedids.size() );
    RefMan<EM::Object> emobject =
			EM::Flt3DMan().getObject( f3dpainter_->getFaultID() );

    mDynamicCastGet(EM::Fault3D*,emf3d,emobject.ptr());
    if ( !emf3d )
	return;

    mDynamicCastGet(const Geometry::FaultStickSet*,fss,
		    emf3d->geometryElement());
    if ( !fss ) return;

    emf3d->setBurstAlert( true );

    RowCol rc;
    for ( int ids=selectedids.size()-1; ids>=0; ids-- )
    {
	rc.row() = getStickId( selectedids[ids] );
	const StepInterval<int> colrg = fss->colRange( rc.row() );
	rc.col() = colrg.start + selectedidxs[ids]*colrg.step;
	emf3d->geometry().removeKnot( EM::PosID::getFromRowCol(rc), false );
	if ( !emf3d->geometry().nrKnots(rc.row()) )
	    emf3d->geometry().removeStick( rc.row(), false );
    }

    emf3d->setBurstAlert( false );
}

} //namespace MPE
