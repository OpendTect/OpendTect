/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emfault3dpainter.h"

#include "bendpointfinder.h"
#include "emfault3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "explfaultsticksurface.h"
#include "explplaneintersection.h"
#include "flatposdata.h"
#include "randomlinegeom.h"
#include "zaxistransform.h"
#include "positionlist.h"
#include "survinfo.h"
#include "trigonometry.h"

namespace EM
{

Fault3DPainter::Fault3DPainter( FlatView::Viewer& fv, const EM::ObjectID& oid )
    : viewer_(fv)
    , emid_(oid)
    , markerstyle_(MarkerStyle2D::Square,4,OD::Color(255,255,0))
    , activestickid_( mUdf(int) )
    , path_(0)
    , flatposdata_(0)
    , abouttorepaint_(this)
    , repaintdone_(this)
    , linenabled_(true)
    , knotenabled_(false)
    , paintenable_(true)
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
    {
	emobj->ref();
	emobj->change.notify( mCB(this,Fault3DPainter,fault3DChangedCB) );
    }
    tkzs_.setEmpty();
}


Fault3DPainter::~Fault3DPainter()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
    {
	emobj->change.remove( mCB(this,Fault3DPainter,fault3DChangedCB) );
	emobj->unRef();
    }

    removePolyLine();
    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


void Fault3DPainter::setRandomLineID( RandomLineID rdlid )
{ rdlid_ = rdlid; }


void Fault3DPainter::setTrcKeyZSampling( const TrcKeyZSampling& cs,bool update )
{ tkzs_ = cs; }


void Fault3DPainter::setPath( const TrcKeyPath& path )
{
    path_ = &path;
}


void Fault3DPainter::setFlatPosData( const FlatPosData* fps )
{
    if ( !path_ )
	return;

    flatposdata_ = fps;

    if ( flatposdata_->nrPts(true) < 2 )
	return;

    TypeSet<Coord> pts;
    for ( int idx=0; idx<path_->size(); idx++ )
    {
	pts += Survey::GM().toCoord( (*path_)[idx] );
    }

    BendPointFinder2D bpfinder( pts, 0.5 );
    if ( !bpfinder.execute() || bpfinder.bendPoints().size()<1 )
	return;

    bendpts_ = bpfinder.bendPoints();
}


bool Fault3DPainter::addPolyLine()
{
    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid_ );
    mDynamicCastGet(EM::Fault3D*,emf3d,emobject.ptr())
    if ( !emf3d ) return false;

    Fault3DMarker* f3dsectionmarker = new Fault3DMarker;
    f3dmarkers_ += f3dsectionmarker;

    bool stickpained = paintSticks( *emf3d, f3dsectionmarker );
    bool intersecpainted = paintIntersection( *emf3d, f3dsectionmarker );
    if ( !stickpained && !intersecpainted )
	 return false;

    return true;
}


void Fault3DPainter::paint()
{
    repaintFault3D();
}


bool Fault3DPainter::paintSticks( EM::Fault3D& f3d, Fault3DMarker* f3dmaker )
{
    mDynamicCastGet(Geometry::FaultStickSurface*,fss,f3d.geometryElement())
    if ( !fss || fss->isEmpty() )
	return false;

    if ( !path_ && tkzs_.isEmpty() ) return false;

    RowCol rc;
    const StepInterval<int> rowrg = fss->rowRange();
    for ( rc.row()=rowrg.start; rc.row()<=rowrg.stop; rc.row()+=rowrg.step )
    {
	StepInterval<int> colrg = fss->colRange( rc.row() );
	FlatView::AuxData* stickauxdata = viewer_.createAuxData( 0 );
	stickauxdata->cursor_ = knotenabled_ ? MouseCursor::Cross
					     : MouseCursor::Arrow;
	stickauxdata->poly_.erase();
	stickauxdata->linestyle_ = f3d.preferredLineStyle();
	if ( rc.row() == activestickid_ )
	    stickauxdata->linestyle_.width_ *= 2;

	stickauxdata->linestyle_.color_ = f3d.preferredColor();
	stickauxdata->markerstyles_ += markerstyle_;
	if ( !knotenabled_ )
	    stickauxdata->markerstyles_.erase();
	stickauxdata->enabled_ = linenabled_;

	const Coord3 stkednor = f3d.geometry().getEditPlaneNormal( rc.row() );

	if ( !path_ )
	{
	    if ( !paintStickOnPlane(*fss,rc,colrg,stkednor,*stickauxdata) )
		continue;
	}
	else
	{
	    if ( !paintStickOnRLine(*fss,rc,colrg,stkednor,*stickauxdata) )
		continue;
	}

	if ( stickauxdata->poly_.size() == 0 )
		delete stickauxdata;
	    else
	    {
		StkMarkerInfo* stkmkrinfo = new StkMarkerInfo;
		stkmkrinfo->marker_ = stickauxdata;
		stkmkrinfo->stickid_ = rc.row();
		f3dmaker->stickmarker_ += stkmkrinfo;
		viewer_.addAuxData( stickauxdata );
	    }
    }

    return true;
}


bool Fault3DPainter::paintStickOnPlane( const Geometry::FaultStickSurface& fss,
					RowCol& rc,const StepInterval<int>& crg,
					const Coord3& stkednor,
				FlatView::AuxData& stickauxdata )
{
    Coord3 editnormal( 0, 0, 1 );

    if ( tkzs_.defaultDir() == TrcKeyZSampling::Inl )
	editnormal = Coord3( SI().binID2Coord().inlDir(), 0 );
    else if ( tkzs_.defaultDir() == TrcKeyZSampling::Crl )
	editnormal = Coord3( SI().binID2Coord().crlDir(), 0 );

    const Coord3 nzednor = editnormal.normalize();

    const bool equinormal =
	mIsEqual(nzednor.x,stkednor.x,.001) &&
	mIsEqual(nzednor.y,stkednor.y,.001) &&
	mIsEqual(nzednor.z,stkednor.z,.00001);

    if ( !equinormal ) return false;

    RefMan<Survey::Geometry3D> geom3d = SI().get3DGeometry( false );
    const Pos::IdxPair2Coord& bid2crd = geom3d->binID2Coord();
    if ( tkzs_.defaultDir() != TrcKeyZSampling::Z )
    {
	BinID extrbid1, extrbid2;
	if ( tkzs_.defaultDir() == TrcKeyZSampling::Inl )
	{
	    extrbid1.inl() = extrbid2.inl() = tkzs_.hsamp_.inlRange().start;
	    extrbid1.crl() = tkzs_.hsamp_.crlRange().start;
	    extrbid2.crl() = tkzs_.hsamp_.crlRange().stop;
	}
	else if ( tkzs_.defaultDir() == TrcKeyZSampling::Crl )
	{
	    extrbid1.inl() = tkzs_.hsamp_.inlRange().start;
	    extrbid2.inl() = tkzs_.hsamp_.inlRange().stop;
	    extrbid1.crl() = extrbid2.crl() = tkzs_.hsamp_.crlRange().start;
	}

	Coord extrcoord1, extrcoord2;
	extrcoord1 = SI().transform( extrbid1 );
	extrcoord2 = SI().transform( extrbid2 );

	ConstRefMan<ZAxisTransform> zat = viewer_.getZAxisTransform();
	for ( rc.col()=crg.start; rc.col()<=crg.stop; rc.col()+=crg.step )
	{
	    const Coord3& pos = fss.getKnot( rc );
	    BinID knotbinid = SI().transform( pos );

	    if ( pointOnEdge2D(pos.coord(),extrcoord1,extrcoord2,.5)
		 || (tkzs_.defaultDir()==TrcKeyZSampling::Inl
		     && knotbinid.inl()==extrbid1.inl())
		 || (tkzs_.defaultDir()==TrcKeyZSampling::Crl
		     && knotbinid.crl()==extrbid1.crl()) )
	    {
		const Coord bidf = bid2crd.transformBackNoSnap( pos.coord() );
		const double z = zat ? zat->transform(pos) : pos.z;
		if ( tkzs_.defaultDir() == TrcKeyZSampling::Inl )
		    stickauxdata.poly_ += FlatView::Point( bidf.y, z );
		else if ( tkzs_.defaultDir() == TrcKeyZSampling::Crl )
		    stickauxdata.poly_ += FlatView::Point( bidf.x, z );
	    }
	}
    }
    else
    {
	for ( rc.col()=crg.start; rc.col()<=crg.stop; rc.col()+=crg.step )
	{
	    const Coord3 pos = fss.getKnot( rc );
	    if ( !mIsEqual(pos.z,tkzs_.zsamp_.start,.0001) )
		break;

	    const Coord bidf = bid2crd.transformBackNoSnap( pos.coord() );
	    stickauxdata.poly_ += FlatView::Point( bidf.x, bidf.y );
	}
    }

    return true;
}


bool Fault3DPainter::paintStickOnRLine( const Geometry::FaultStickSurface& fss,
					RowCol& rc,const StepInterval<int>& crg,
					const Coord3& stkednor,
				   FlatView::AuxData& stickauxdata )
{
    BinID bid;
    ConstRefMan<ZAxisTransform> zat = viewer_.getZAxisTransform();
    RefMan<Geometry::RandomLine> rlgeom =
	Geometry::RLM().get( rdlid_ );
    if ( !path_ || !rlgeom )
	return false;

    for ( rc.col()=crg.start;rc.col()<=crg.stop;rc.col()+=crg.step )
    {
	const Coord3 pos = fss.getKnot( rc );
	bid = SI().transform( pos.coord() );
	const TrcKey trckey( bid );
	TrcKeyPath knots;
	rlgeom->allNodePositions( knots );
	Coord3 editnormal( Geometry::RandomLine::getNormal(knots,trckey), 0 );
	const Coord3 nzednor = editnormal.normalize();

	const bool equinormal =
	    mIsEqual(nzednor.x,stkednor.x,.001) &&
	    mIsEqual(nzednor.y,stkednor.y,.001) &&
	    mIsEqual(nzednor.z,stkednor.z,.00001);

	if ( !equinormal )
	    return false;

	const int posidx = Geometry::RandomLine::getNearestPathPosIdx(
		knots, *path_, trckey );
	if ( posidx<0 )
	    return false;

	stickauxdata.poly_ +=
	    FlatView::Point( flatposdata_->position(true,posidx),
			     zat ? zat->transform(pos) : pos.z );
    }
    return true;
}


bool Fault3DPainter::paintIntersection( EM::Fault3D& f3d,
					Fault3DMarker* f3dmaker )
{
    PtrMan<Geometry::IndexedShape> faultsurf =
		    new Geometry::ExplFaultStickSurface(
			f3d.geometry().geometryElement(), SI().zScale() );
    faultsurf->setCoordList( new Coord3ListImpl, new Coord3ListImpl );
    if ( !faultsurf->update(true,0) )
	return false;

    PtrMan<Geometry::ExplPlaneIntersection> intxn =
					new Geometry::ExplPlaneIntersection;
    intxn->setShape( *faultsurf );

    if ( path_ )
    {
	//TODO make it multithreaded
	double zstart = flatposdata_->position( false, 0 );
	double zstop = flatposdata_->position( false,
					       flatposdata_->nrPts(false)-1 );

	TypeSet<Coord3> pts;

	bool status = false;

	for ( int idx=1; idx<bendpts_.size(); idx++ )
	{
	    pts.erase();

	    Coord3 p0( Survey::GM().toCoord((*path_)[bendpts_[idx-1]]),zstart );
	    Coord3 p1( Survey::GM().toCoord((*path_)[bendpts_[idx-1]]),zstop );
	    Coord3 p2( Survey::GM().toCoord((*path_)[bendpts_[idx]]), zstart );
	    Coord3 p3( Survey::GM().toCoord((*path_)[bendpts_[idx]]), zstop );

	    pts += p0; pts += p1; pts += p2; pts += p3;

	    if ( paintPlaneIntxn(f3d,f3dmaker,intxn,pts) )
		status = true;
	}

	return status;
    }
    else
    {
	TypeSet<Coord3> pts;
	if ( tkzs_.defaultDir() == TrcKeyZSampling::Z )
	{
	    BinID lt( tkzs_.hsamp_.start_.inl(), tkzs_.hsamp_.start_.crl() );
	    BinID lb(tkzs_.hsamp_.start_.inl(), tkzs_.hsamp_.stop_.crl() );
	    BinID rt( tkzs_.hsamp_.stop_.inl(), tkzs_.hsamp_.start_.crl() );
	    BinID rb(tkzs_.hsamp_.stop_.inl(), tkzs_.hsamp_.stop_.crl() );

	    pts += Coord3( SI().transform(lt), tkzs_.zsamp_.start );
	    pts += Coord3( SI().transform(lb), tkzs_.zsamp_.start );
	    pts += Coord3( SI().transform(rt), tkzs_.zsamp_.start );
	    pts += Coord3( SI().transform(rb), tkzs_.zsamp_.start );
	}
	else
	{
	    BinID start( tkzs_.hsamp_.start_.inl(), tkzs_.hsamp_.start_.crl() );
	    BinID stop(tkzs_.hsamp_.stop_.inl(), tkzs_.hsamp_.stop_.crl() );

	    pts += Coord3( SI().transform(start), tkzs_.zsamp_.start );
	    pts += Coord3( SI().transform(start), tkzs_.zsamp_.stop );
	    pts += Coord3( SI().transform(stop), tkzs_.zsamp_.start );
	    pts += Coord3( SI().transform(stop), tkzs_.zsamp_.stop );
	}
	if ( !paintPlaneIntxn(f3d,f3dmaker,intxn,pts) )
	    return false;
    }

    return true;
}


bool Fault3DPainter::paintPlaneIntxn(EM::Fault3D& f3d, Fault3DMarker* f3dmaker,
				     Geometry::ExplPlaneIntersection* intxn,
				     TypeSet<Coord3>& pts )
{
    if ( pts.size() < 4 ) return false;

    if ( !intxn ) return false;

    int nrplanes = intxn->nrPlanes();

    for ( int idx=0; idx<nrplanes; idx++ )
	intxn->removePlane( intxn->planeID(idx) );

    const Coord3 normal = (pts[1]-pts[0]).cross(pts[3]-pts[0]).normalize();
    intxn->addPlane( normal, pts );

    Geometry::IndexedShape* idxshape = intxn;
    idxshape->setCoordList( new Coord3ListImpl, new Coord3ListImpl );

    if ( !idxshape->update(true,0) )
	return false;

    if ( !idxshape->getGeometry().size() )
	return false;

    Geometry::IndexedGeometry* idxgeom = idxshape->getGeometry()[0];
    Geometry::PrimitiveSet* geomps = idxgeom->getCoordsPrimitiveSet();
    if ( !geomps->size() )
	return false;

    Coord3List* clist = idxshape->coordList();
    mDynamicCastGet(Coord3ListImpl*,intxnposlist,clist);
    TypeSet<Coord3> intxnposs;
    if ( intxnposlist->size() > 0 )
    {
	int nextposid = intxnposlist->nextID( -1 );
	while ( nextposid!=-1 )
	{
	    if ( intxnposlist->isDefined(nextposid) )
		intxnposs += intxnposlist->get( nextposid );
	    nextposid = intxnposlist->nextID( nextposid );
	}
    }


    genIntersectionAuxData( f3d, f3dmaker, geomps, intxnposs );

    return true;
}


void Fault3DPainter::genIntersectionAuxData( EM::Fault3D& f3d,
					Fault3DMarker* f3dmaker,
					const Geometry::PrimitiveSet* coordps,
					TypeSet<Coord3>& intxnposs)
{
    for ( int idx=1; idx<coordps->size(); idx+=2 )
    {
	const Coord3 pos1 = intxnposs[coordps->get(idx)];
	const Coord3 pos2 = intxnposs[coordps->get(idx-1)];
	FlatView::Point auxpos1 = getFVAuxPoint( pos1 );
	FlatView::Point auxpos2 = getFVAuxPoint( pos2 );
	if ( !auxpos1.isDefined() || !auxpos2.isDefined() )
	    continue;

	FlatView::AuxData* intsecauxdat = viewer_.createAuxData( 0 );
	intsecauxdat->poly_.erase();
	intsecauxdat->cursor_ = knotenabled_ ? MouseCursor::Cross
					     : MouseCursor::Arrow;
	intsecauxdat->linestyle_ = f3d.preferredLineStyle();
	intsecauxdat->linestyle_.width_ = f3d.preferredLineStyle().width_ / 2;
	intsecauxdat->linestyle_.color_ = f3d.preferredColor();
	intsecauxdat->enabled_ = linenabled_;
	intsecauxdat->poly_ += auxpos1;
	intsecauxdat->poly_ += auxpos2;
	f3dmaker->intsecmarker_ += intsecauxdat;
	viewer_.addAuxData( intsecauxdat );
    }
}


FlatView::Point Fault3DPainter::getFVAuxPoint( const Coord3& pos ) const
{
    BinID posbid =  SI().transform( pos.coord() );
    ConstRefMan<ZAxisTransform> zat = viewer_.getZAxisTransform();
    if ( path_ )
    {
	const TrcKey trckey( posbid );
	const int trcidx = path_->indexOf( trckey );
	if ( trcidx == -1 )
	    return FlatView::Point::udf();

	const double z = zat ? zat->transform(pos) : pos.z;
	return FlatView::Point( flatposdata_->position(true,trcidx), z );
    }

    if ( tkzs_.nrZ() == 1 )
	return FlatView::Point( posbid.inl(), posbid.crl());
    else if ( tkzs_.nrCrl() == 1 )
	return FlatView::Point( posbid.inl(), zat? zat->transform(pos):pos.z );
    else if ( tkzs_.nrInl() == 1 )
	return FlatView::Point( posbid.crl(), zat? zat->transform(pos):pos.z );
    return FlatView::Point::udf();
}


void Fault3DPainter::enableLine( bool yn )
{
    if ( linenabled_ == yn )
	return;

    for ( int markidx=f3dmarkers_.size()-1; markidx>=0; markidx-- )
    {
	Fault3DMarker* marklns = f3dmarkers_[markidx];

	for ( int idxstk=marklns->stickmarker_.size()-1; idxstk>=0; idxstk-- )
	{
	    marklns->stickmarker_[idxstk]->marker_->enabled_ = yn;
	}

	for ( int idxitr=marklns->intsecmarker_.size()-1; idxitr>=0; idxitr-- )
	{
	    marklns->intsecmarker_[idxitr]->enabled_ = yn;
	}
    }

    linenabled_ = yn;
    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


void Fault3DPainter::enableKnots( bool yn )
{
    if ( knotenabled_ == yn )
	return;

    for ( int markidx=f3dmarkers_.size()-1; markidx>=0; markidx-- )
    {
	Fault3DMarker* marklns = f3dmarkers_[markidx];

	for ( int idxstk=marklns->stickmarker_.size()-1; idxstk>=0; idxstk-- )
	{
	    if ( !yn )
		marklns->stickmarker_[idxstk]->marker_->markerstyles_.erase();
	    else
		marklns->stickmarker_[idxstk]->marker_->markerstyles_ +=
								markerstyle_;
	}
    }

    knotenabled_ = yn;
    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


void Fault3DPainter::setActiveStick( const EM::PosID& pid )
{
    if ( pid.objectID() != emid_ ) return;

    if ( pid.getRowCol().row() == activestickid_ ) return;

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid_ );
    if ( !emobject ) return;

    const OD::LineStyle& fltlinestyle = emobject->preferredLineStyle();
    for ( int auxdid=0; auxdid<f3dmarkers_[0]->stickmarker_.size(); auxdid++ )
    {
	OD::LineStyle& linestyle =
	    f3dmarkers_[0]->stickmarker_[auxdid]->marker_->linestyle_;
	if ( f3dmarkers_[0]->stickmarker_[auxdid]->stickid_== activestickid_ )
	    linestyle.width_ = fltlinestyle.width_;
	else if ( f3dmarkers_[0]->stickmarker_[auxdid]->stickid_ ==
		  pid.getRowCol().row() )
	    linestyle.width_ = fltlinestyle.width_ * 2;
    }

    activestickid_ = pid.getRowCol().row();
    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


bool Fault3DPainter::hasDiffActiveStick( const EM::PosID* pid ) const
{
    if ( pid->objectID() != emid_ ||
	 pid->getRowCol().row() != activestickid_ )
	return true;
    else
	return false;
}


FlatView::AuxData* Fault3DPainter::getAuxData(
						const EM::PosID* pid) const
{
    if ( pid->objectID() != emid_ )
	return 0;

    return f3dmarkers_[0]->stickmarker_[activestickid_]->marker_;
}


void Fault3DPainter::getDisplayedSticks( ObjectSet<StkMarkerInfo>& dispstkinfo )
{
    if ( !f3dmarkers_.size() ) return;

    dispstkinfo = f3dmarkers_[0]->stickmarker_;
}


void Fault3DPainter::removePolyLine()
{
    for ( int markidx=f3dmarkers_.size()-1; markidx>=0; markidx-- )
    {
	Fault3DMarker* f3dmarker = f3dmarkers_[markidx];
	for ( int idi=f3dmarker->intsecmarker_.size()-1; idi>=0; idi-- )
	{
	    viewer_.removeAuxData( f3dmarker->intsecmarker_[idi] );
	}
	for ( int ids=f3dmarker->stickmarker_.size()-1; ids>=0; ids-- )
	{
	    viewer_.removeAuxData( f3dmarker->stickmarker_[ids]->marker_ );
	}
    }

    deepErase( f3dmarkers_ );
}


void Fault3DPainter::enablePaint( bool paintenable )
{
    paintenable_ = paintenable;
}


void Fault3DPainter::repaintFault3D()
{
    if ( !paintenable_ )
	return;

    abouttorepaint_.trigger();
    removePolyLine();
    addPolyLine();
    viewer_.handleChange( FlatView::Viewer::Auxdata );
    repaintdone_.trigger();
}

void Fault3DPainter::fault3DChangedCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( const EM::EMObjectCallbackData&,
				cbdata, caller, cb );
    mDynamicCastGet(EM::EMObject*,emobject,caller);
    if ( !emobject ) return;

    mDynamicCastGet(EM::Fault3D*,emf3d,emobject);
    if ( !emf3d ) return;

    if ( emobject->id() != emid_ ) return;

    switch ( cbdata.event )
    {
	case EM::EMObjectCallbackData::Undef:
	    break;
	case EM::EMObjectCallbackData::PrefColorChange:
	    {
		for ( int oidx=0; oidx<f3dmarkers_.size(); oidx++ )
		{
		    if ( !f3dmarkers_[oidx] ) continue;
		    Fault3DMarker& mrks = *f3dmarkers_[oidx];

		    for ( int stid=0; stid<mrks.stickmarker_.size(); stid++ )
		    {
			if ( !mrks.stickmarker_[stid] ) continue;

			mrks.stickmarker_[stid]->marker_->linestyle_ =
						emf3d->preferredLineStyle();
			mrks.stickmarker_[stid]->marker_->linestyle_.color_ =
							emf3d->preferredColor();
		    }

		    for ( int itid=0; itid<mrks.intsecmarker_.size(); itid++ )
		    {
			if ( !mrks.intsecmarker_[itid] ) continue;

			mrks.intsecmarker_[itid]->linestyle_ =
						emf3d->preferredLineStyle();
			mrks.intsecmarker_[itid]->linestyle_.color_ =
							emf3d->preferredColor();
		    }
		}

		viewer_.handleChange( FlatView::Viewer::Auxdata );
		break;
	    }
	case EM::EMObjectCallbackData::PositionChange:
	{
	    if ( emf3d->hasBurstAlert() )
		return;
	    repaintFault3D();
	    break;
	}
	case EM::EMObjectCallbackData::BurstAlert:
	{
	    if (  emobject->hasBurstAlert() )
		return;
	    repaintFault3D();
	    break;
	}
	default: break;
    }
}


Coord Fault3DPainter::getNormalInRandLine( int idx ) const
{
    if ( !path_ )
	return Coord(mUdf(float), mUdf(float));

    if ( idx < 0 || path_->size() == 0 )
	return Coord(mUdf(float), mUdf(float));

    const Coord pivotcrd = Survey::GM().toCoord( (*path_)[idx] );
    Coord nextcrd;

    if ( idx+1 < path_->size() )
	nextcrd = Survey::GM().toCoord( (*path_)[idx+1] );
    else if ( idx-1 > 0 )
	nextcrd = Survey::GM().toCoord( (*path_)[idx-1] );

    Coord direction = nextcrd - pivotcrd;
    return Coord( -direction.y, direction.x );
}


} // namespace EM
