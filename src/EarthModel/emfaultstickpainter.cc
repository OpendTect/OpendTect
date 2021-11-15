/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Jan 2010
________________________________________________________________________

-*/

#include "emfaultstickpainter.h"

#include "faultstickset.h"
#include "emfaultstickset.h"
#include "emmanager.h"
#include "emobject.h"
#include "flatposdata.h"
#include "randomlinegeom.h"
#include "zaxistransform.h"
#include "survinfo.h"
#include "trigonometry.h"

namespace EM
{
FaultStickPainter::FaultStickPainter( FlatView::Viewer& fv,
				      const EM::ObjectID& oid )
    : viewer_(fv)
    , emid_(oid)
    , markerstyle_(MarkerStyle2D::Square,4,OD::Color(255,255,0))
    , activestickid_( -1 )
    , is2d_( false )
    , path_(0)
    , flatposdata_(0)
    , geomid_( Survey::GeometryManager::cUndefGeomID() )
    , abouttorepaint_( this )
    , repaintdone_( this )
    , linenabled_(true)
    , knotenabled_(false)
    , paintenable_(true)
    , rdlid_(-1)
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
    {
	emobj->ref();
	emobj->change.notify( mCB(this,FaultStickPainter,fssChangedCB) );
    }

    tkzs_.setEmpty();
}


FaultStickPainter::~FaultStickPainter()
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
    {
	emobj->change.remove( mCB(this,FaultStickPainter,fssChangedCB) );
	emobj->unRef();
    }

    removePolyLine();
    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


const char* FaultStickPainter::getLineName() const
{ return Survey::GM().getName( geomid_ ); }

void FaultStickPainter::setTrcKeyZSampling( const TrcKeyZSampling& cs,bool upd )
{ tkzs_ = cs; }

void FaultStickPainter::setPath( const TrcKeyPath& path )
{ path_ = &path; }

void FaultStickPainter::setRandomLineID( int rdlid )
{ rdlid_ = rdlid; }


void FaultStickPainter::setFlatPosData( const FlatPosData* fps )
{
    if ( path_ )
	flatposdata_ = fps;
}


void FaultStickPainter::paint()
{
    repaintFSS();
}


bool FaultStickPainter::addPolyLine()
{
    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid_ );

    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject.ptr());
    if ( !emfss ) return false;

    RefMan<Survey::Geometry3D> geom3d = SI().get3DGeometry( false );
    const Pos::IdxPair2Coord& bid2crd = geom3d->binID2Coord();
    for ( int sidx=0; sidx<emfss->nrSections(); sidx++ )
    {
	const EM::SectionID sid = emfss->sectionID( sidx );
	mDynamicCastGet(const Geometry::FaultStickSet*,fss,
			emfss->sectionGeometry(sid));
	if ( fss->isEmpty() )
	    continue;

	RowCol rc;
	const StepInterval<int> rowrg = fss->rowRange();

	ObjectSet<StkMarkerInfo>* secmarkerlines = new ObjectSet<StkMarkerInfo>;
	sectionmarkerlines_ += secmarkerlines;

	ConstRefMan<ZAxisTransform> zat = viewer_.getZAxisTransform();
	for ( rc.row()=rowrg.start; rc.row()<=rowrg.stop; rc.row()+=rowrg.step )
	{
	    StepInterval<int> colrg = fss->colRange( rc.row() );

	    FlatView::AuxData* stickauxdata = viewer_.createAuxData( 0 );
	    stickauxdata->cursor_ = knotenabled_ ? MouseCursor::Cross
						 : MouseCursor::Arrow;
	    stickauxdata->poly_.erase();
	    stickauxdata->linestyle_ = emfss->preferredLineStyle();
	    if ( rc.row() == activestickid_ )
		stickauxdata->linestyle_.width_ *= 2;

	    stickauxdata->linestyle_.color_ = emfss->preferredColor();
	    stickauxdata->markerstyles_ += markerstyle_;
	    if ( !knotenabled_ )
		stickauxdata->markerstyles_.erase();
	    stickauxdata->enabled_ = linenabled_;

	    if ( emfss->geometry().pickedOn2DLine(sid,rc.row()) )
	    {
		const Pos::GeomID geomid =
			    emfss->geometry().pickedGeomID( sid, rc.row() );

		if ( !is2d_ || geomid != geomid_ )
		    continue;
	    }
	    else if ( emfss->geometry().pickedOnPlane(sid,rc.row()) )
	    {
		if ( tkzs_.isEmpty() && !path_ ) continue;
	    }
	    else continue;

	    if ( tkzs_.isEmpty() ) // this means this is a 2D or random Line
	    {
		RefMan<Geometry::RandomLine> rlgeom =
		    Geometry::RLM().get( rdlid_ );
		if ( path_ && rlgeom )
		{
		    TrcKeyPath knots;
		    rlgeom->allNodePositions( knots );
		    for ( rc.col()=colrg.start;rc.col()<=colrg.stop;
			  rc.col()+=colrg.step )
		    {
			const Coord3 pos = fss->getKnot( rc );
			const BinID bid = SI().transform( pos.coord() );
			const TrcKey trckey( bid );
			Coord3 editnormal(
			    Geometry::RandomLine::getNormal(knots,trckey), 0.f);
			const Coord3 nzednor = editnormal.normalize();
			const Coord3 stkednor =
			    emfss->geometry().getEditPlaneNormal(sid,rc.row());
			const bool equinormal =
			    mIsEqual(nzednor.x,stkednor.x,.001) &&
			    mIsEqual(nzednor.y,stkednor.y,.001) &&
			    mIsEqual(nzednor.z,stkednor.z,.00001);

			if ( !equinormal ) continue;

			const int posidx =
			    Geometry::RandomLine::getNearestPathPosIdx(
				    knots, *path_, trckey );
			const double z = zat ? zat->transform(pos) : pos.z;
			stickauxdata->poly_ += FlatView::Point(
					flatposdata_->position(true,posidx),z);
		    }
		}
		else
		{
		    for ( rc.col()=colrg.start;rc.col()<=colrg.stop;
			  rc.col()+=colrg.step )
		    {
			const Coord3 pos = fss->getKnot( rc );
			float dist;
			const double z = zat ? zat->transform(pos) : pos.z;
			if ( getNearestDistance(pos,dist) )
			    stickauxdata->poly_ += FlatView::Point(dist,z);
		    }
		}
	    }
	    else
	    {
		Coord3 editnormal( 0, 0, 1 );
		// Let's assume cs default dir. is 'Z'

		if ( tkzs_.defaultDir() == TrcKeyZSampling::Inl )
		    editnormal = Coord3( SI().binID2Coord().inlDir(), 0 );
		else if ( tkzs_.defaultDir() == TrcKeyZSampling::Crl )
		    editnormal = Coord3( SI().binID2Coord().crlDir(), 0 );

		const Coord3 nzednor = editnormal.normalize();
		const Coord3 stkednor =
			emfss->geometry().getEditPlaneNormal(sid,rc.row());

		const bool equinormal =
		    mIsEqual(nzednor.x,stkednor.x,.001) &&
		    mIsEqual(nzednor.y,stkednor.y,.001) &&
		    mIsEqual(nzednor.z,stkednor.z,.00001);

		if ( !equinormal ) continue;

		// we need to deal in different way if cs direction is Z
		if ( tkzs_.defaultDir() != TrcKeyZSampling::Z )
		{
		    BinID extrbid1, extrbid2;
		    if ( tkzs_.defaultDir() == TrcKeyZSampling::Inl )
		    {
			extrbid1.inl() = extrbid2.inl() =
					tkzs_.hsamp_.inlRange().start;
			extrbid1.crl() = tkzs_.hsamp_.crlRange().start;
			extrbid2.crl() = tkzs_.hsamp_.crlRange().stop;
		    }
		    else if ( tkzs_.defaultDir() == TrcKeyZSampling::Crl )
		    {
			extrbid1.inl() = tkzs_.hsamp_.inlRange().start;
			extrbid2.inl() = tkzs_.hsamp_.inlRange().stop;
			extrbid1.crl() = extrbid2.crl() =
					 tkzs_.hsamp_.crlRange().start;
		    }

		    Coord extrcoord1, extrcoord2;
		    extrcoord1 = SI().transform( extrbid1 );
		    extrcoord2 = SI().transform( extrbid2 );

		    for ( rc.col()=colrg.start;rc.col()<=colrg.stop;
						rc.col()+=colrg.step )
		    {
			const Coord3& pos = fss->getKnot( rc );
			BinID knotbinid = SI().transform( pos );
			if (pointOnEdge2D(pos.coord(),extrcoord1,extrcoord2,.5)
			    || (tkzs_.defaultDir()==TrcKeyZSampling::Inl
				&& knotbinid.inl()==extrbid1.inl())
			    || (tkzs_.defaultDir()==TrcKeyZSampling::Crl
				&& knotbinid.crl()==extrbid1.crl()) )
			{
			    const Coord bidf =
				bid2crd.transformBackNoSnap( pos.coord() );
			    const double z = zat ? zat->transform(pos) : pos.z;
			    if ( tkzs_.defaultDir() == TrcKeyZSampling::Inl )
				stickauxdata->poly_ += FlatView::Point(
								bidf.y, z );
			    else if ( tkzs_.defaultDir()==TrcKeyZSampling::Crl )
				stickauxdata->poly_ += FlatView::Point(
								bidf.x, z );
			}
		    }
		}
		else
		{
		    for ( rc.col()=colrg.start; rc.col()<=colrg.stop;
							rc.col()+=colrg.step )
		    {
			const Coord3 pos = fss->getKnot( rc );
			if ( !mIsEqual(pos.z,tkzs_.zsamp_.start,.0001) )
			    break;

			const Coord bidf =
			    bid2crd.transformBackNoSnap( pos.coord() );
			stickauxdata->poly_ += FlatView::Point( bidf.x, bidf.y);
		    }
		}
	    }

	    if ( stickauxdata->poly_.size() == 0 )
		delete stickauxdata;
	    else
	    {
		StkMarkerInfo* stkmkrinfo = new StkMarkerInfo;
		stkmkrinfo->marker_ = stickauxdata;
		stkmkrinfo->stickid_ = rc.row();
		(*secmarkerlines) += stkmkrinfo;
		viewer_.addAuxData( stickauxdata );
	    }
	}
    }

    return true;
}


void FaultStickPainter::enableLine( bool yn )
{
    if ( linenabled_ == yn )
	return;

    for ( int markidx=sectionmarkerlines_.size()-1; markidx>=0; markidx-- )
    {
	ObjectSet<StkMarkerInfo>* markerlines = sectionmarkerlines_[markidx];
	for ( int idy=markerlines->size()-1; idy>=0; idy-- )
	{
	    (*markerlines)[idy]->marker_->enabled_ = yn;
	}
    }

    linenabled_ = yn;
    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


void FaultStickPainter::enableKnots( bool yn )
{
    if ( knotenabled_ == yn )
	return;

    for ( int markidx=sectionmarkerlines_.size()-1; markidx>=0; markidx-- )
    {
	ObjectSet<StkMarkerInfo>* markerlines = sectionmarkerlines_[markidx];
	for ( int idy=markerlines->size()-1; idy>=0; idy-- )
	{
	    if ( !yn )
		(*markerlines)[idy]->marker_->markerstyles_.erase();
	    else
		(*markerlines)[idy]->marker_->markerstyles_ += markerstyle_;
	}
    }

    knotenabled_ = yn;
    viewer_.handleChange( FlatView::Viewer::Auxdata );
}



void FaultStickPainter::enablePaint(bool paintenable)
{
    paintenable_ = paintenable;
}


void FaultStickPainter::repaintFSS()
{
    if ( !paintenable_ )
	return;

    abouttorepaint_.trigger();
    removePolyLine();
    addPolyLine();
    viewer_.handleChange( FlatView::Viewer::Auxdata );
    repaintdone_.trigger();
}


void FaultStickPainter::removePolyLine()
{
    for ( int markidx=sectionmarkerlines_.size()-1; markidx>=0; markidx-- )
    {
	ObjectSet<StkMarkerInfo>* markerlines = sectionmarkerlines_[markidx];
	for ( int idy=markerlines->size()-1; idy>=0; idy-- )
	    viewer_.removeAuxData( (*markerlines)[idy]->marker_ );
	deepErase( *markerlines );
    }

    deepErase( sectionmarkerlines_ );
}


void FaultStickPainter::fssChangedCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( const EM::EMObjectCallbackData&,
				cbdata, caller, cb );
    mDynamicCastGet(EM::EMObject*,emobject,caller);
    if ( !emobject ) return;

    mDynamicCastGet(EM::FaultStickSet*,emfss,emobject);
    if ( !emfss ) return;

    if ( emobject->id() != emid_ ) return;

    switch ( cbdata.event )
    {
	case EM::EMObjectCallbackData::Undef:
	    break;
	case EM::EMObjectCallbackData::PrefColorChange:
	    {
		for ( int oidx=0; oidx<sectionmarkerlines_.size(); oidx++ )
		{
		    if ( !sectionmarkerlines_[oidx] ) continue;
		    ObjectSet<StkMarkerInfo>& stmkrinfos =
						*sectionmarkerlines_[oidx];

		    for( int iidx=0; iidx<stmkrinfos.size(); iidx++ )
		    {
			if ( !stmkrinfos[iidx] ) continue;

			stmkrinfos[iidx]->marker_->linestyle_ =
						emfss->preferredLineStyle();
			stmkrinfos[iidx]->marker_->linestyle_.color_ =
							emfss->preferredColor();
		    }
		}

		viewer_.handleChange( FlatView::Viewer::Auxdata );
		break;
	    }
	case EM::EMObjectCallbackData::PositionChange:
	{
	    if ( emfss->hasBurstAlert() )
		return;
	    repaintFSS();
	    break;
	}
	case EM::EMObjectCallbackData::BurstAlert:
	{
	    if (  emobject->hasBurstAlert() )
		return;
	    repaintFSS();
	    break;
	}
	default:
	    break;
    }
}


FlatView::AuxData* FaultStickPainter::getAuxData( const EM::PosID* pid )
{
    if ( pid->objectID() != emid_ || sectionmarkerlines_.isEmpty() ||
	 !sectionmarkerlines_[0]->validIdx(activestickid_) )
	return nullptr;

    return (*sectionmarkerlines_[0])[activestickid_]->marker_;
}


void FaultStickPainter::getDisplayedSticks(
					ObjectSet<StkMarkerInfo>& dispstkinfo )
{
    if ( sectionmarkerlines_.isEmpty() )
	return;

    dispstkinfo = *sectionmarkerlines_[0];
}


bool FaultStickPainter::hasDiffActiveStick( const EM::PosID* pid )
{
    if ( pid->objectID() != emid_ ||
	 pid->getRowCol().row() != activestickid_ )
	return true;
    else
	return false;
}


void FaultStickPainter::setActiveStick( EM::PosID& pid )
{
    if ( pid.objectID() != emid_ ||
	 pid.getRowCol().row() == activestickid_ ||
         sectionmarkerlines_.isEmpty() )
	return;

    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid_ );
    if ( !emobject )
	return;

    const OD::LineStyle& emls = emobject->preferredLineStyle();

    auto* line = sectionmarkerlines_[0];
    for ( int stkidx=0; stkidx<line->size(); stkidx++ )
    {
	OD::LineStyle& linestyle = (*line)[stkidx]->marker_->linestyle_;
	if ( (*line)[stkidx]->stickid_==activestickid_ )
	    linestyle.width_ = emls.width_;
	else if ( (*line)[stkidx]->stickid_== pid.getRowCol().row() )
	    linestyle.width_ = emls.width_ * 2;
    }

    activestickid_ = pid.getRowCol().row();
    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


bool FaultStickPainter::getNearestDistance( const Coord3& pos, float& dist )
{
    int posidx = -1;
    mSetUdf(dist);

    for ( int idx=coords_.size()-1; idx>=0; idx-- )
    {
	const float caldist = (float) pos.Coord::sqDistTo( coords_[idx] );
	if ( caldist < dist )
	{
	    dist = caldist;
	    posidx = idx;
	}
    }

    if ( posidx != -1 )
	dist = distances_[posidx];

    return posidx!=-1;
}


Coord FaultStickPainter::getNormalToTrace( int trcnr ) const
{
    int posid = -1;
    int sz = trcnos_.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( trcnos_[idx] == trcnr )
	{
	    posid = idx;
	    break;
	}
    }

    if ( posid == -1 || sz == 0 )
	return Coord(mUdf(float), mUdf(float));

    Coord pos = coords_[posid];
    Coord v1;
    if ( posid+1<sz )
	v1 = coords_[posid+1]- pos;
    else if ( posid-1>=0 )
	v1 = pos - coords_[posid-1];

    if ( v1.x == 0 )
	return Coord( 1, 0 );
    else if ( v1.y == 0 )
	return Coord( 0, 1 );
    else
    {
	double length = Math::Sqrt( v1.x*v1.x + v1.y*v1.y );
	return Coord( -v1.y/length, v1.x/length );
    }
}


Coord FaultStickPainter::getNormalInRandLine( int idx ) const
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

} //namespace EM
