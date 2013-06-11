/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Feb 2010
 RCS:		$Id$
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
#include "positionlist.h"
#include "survinfo.h"
#include "trigonometry.h"

namespace EM
{

Fault3DPainter::Fault3DPainter( FlatView::Viewer& fv, const EM::ObjectID& oid )
    : viewer_(fv)
    , emid_(oid)
    , markerlinestyle_(LineStyle::Solid,2,Color(0,255,0))
    , markerstyle_(MarkerStyle2D::Square, 4, Color(255,255,0) )
    , activestickid_( mUdf(int) )
    , path_(0)
    , flatposdata_(0)
    , abouttorepaint_(this)
    , repaintdone_(this)
    , linenabled_(true)
    , knotenabled_(true)
{
    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    if ( emobj )
    {
	emobj->ref();
	emobj->change.notify( mCB(this,Fault3DPainter,fault3DChangedCB) );
    }
    cs_.setEmpty();
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


void Fault3DPainter::setCubeSampling( const CubeSampling& cs, bool update )
{ cs_ = cs; }


void Fault3DPainter::setPath( const TypeSet<BinID>* path )
{
    path_ = path;
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
	pts += SI().transform((*path_)[idx]);
    }

    BendPointFinder2D bpfinder( pts, 0.5 );
    if ( !bpfinder.execute() || bpfinder.bendPoints().size()<1 )
	return;

    bendpts_ = bpfinder.bendPoints();
}


bool Fault3DPainter::addPolyLine()
{
    RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid_ );

    mDynamicCastGet(EM::Fault3D*,emf3d,emobject.ptr());
    if ( !emf3d ) return false;

    for ( int sidx=0; sidx<emf3d->nrSections(); sidx++ )
    {
	const EM::SectionID sid = emf3d->sectionID( sidx );
	Fault3DMarker* f3dsectionmarker = new Fault3DMarker;
	f3dmarkers_ += f3dsectionmarker;

	bool stickpained = paintSticks(*emf3d,sid,f3dsectionmarker);
	bool intersecpainted = paintIntersection(*emf3d,sid,f3dsectionmarker);
	if ( !stickpained && !intersecpainted )
	     return false;
    }

    return true;
}


void Fault3DPainter::paint()
{
    removePolyLine();
    addPolyLine();
    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


bool Fault3DPainter::paintSticks(EM::Fault3D& f3d, const EM::SectionID& sid,
				 Fault3DMarker* f3dmaker )
{
    mDynamicCastGet( Geometry::FaultStickSurface*, fss,
		     f3d.sectionGeometry(sid) );

    if ( !fss || fss->isEmpty() )
	return false;

    if ( !path_ && cs_.isEmpty() ) return false;

    RowCol rc;
    const StepInterval<int> rowrg = fss->rowRange();
    for ( rc.row=rowrg.start; rc.row<=rowrg.stop; rc.row+=rowrg.step )
    {
	StepInterval<int> colrg = fss->colRange( rc.row );
	FlatView::AuxData* stickauxdata = viewer_.createAuxData( 0 );
	stickauxdata->poly_.erase();
	stickauxdata->linestyle_ = markerlinestyle_;
	if ( rc.row == activestickid_ )
	    stickauxdata->linestyle_.width_ *= 2;

	stickauxdata->linestyle_.color_ = f3d.preferredColor();
	stickauxdata->markerstyles_ += markerstyle_;
	if ( !knotenabled_ )
	    stickauxdata->markerstyles_.erase();
	stickauxdata->enabled_ = linenabled_;

	const Coord3 stkednor = f3d.geometry().getEditPlaneNormal(sid,rc.row);

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
		stkmkrinfo->stickid_ = rc.row;
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

    if ( cs_.defaultDir() == CubeSampling::Inl )
	editnormal = Coord3( SI().binID2Coord().rowDir(), 0 );
    else if ( cs_.defaultDir() == CubeSampling::Crl )
	editnormal = Coord3( SI().binID2Coord().colDir(), 0 );

    const Coord3 nzednor = editnormal.normalize();

    const bool equinormal =
	mIsEqual(nzednor.x,stkednor.x,.001) &&
	mIsEqual(nzednor.y,stkednor.y,.001) &&
	mIsEqual(nzednor.z,stkednor.z,.00001);

    if ( !equinormal ) return false;

    if ( cs_.defaultDir() != CubeSampling::Z )
    {
	BinID extrbid1, extrbid2;
	if ( cs_.defaultDir() == CubeSampling::Inl )
	{
	    extrbid1.inl = extrbid2.inl = cs_.hrg.inlRange().start;
	    extrbid1.crl = cs_.hrg.crlRange().start;
	    extrbid2.crl = cs_.hrg.crlRange().stop;
	}
	else if ( cs_.defaultDir() == CubeSampling::Crl )
	{
	    extrbid1.inl = cs_.hrg.inlRange().start;
	    extrbid2.inl = cs_.hrg.inlRange().stop;
	    extrbid1.crl = extrbid2.crl = cs_.hrg.crlRange().start;
	}

	Coord extrcoord1, extrcoord2;
	extrcoord1 = SI().transform( extrbid1 );
	extrcoord2 = SI().transform( extrbid2 );

	for ( rc.col=crg.start; rc.col<=crg.stop; rc.col+=crg.step )
	{
	    const Coord3& pos = fss.getKnot( rc );
	    BinID knotbinid = SI().transform( pos );

	    if ( pointOnEdge2D(pos.coord(),extrcoord1,extrcoord2,.5)
		 || (cs_.defaultDir()==CubeSampling::Inl
		     && knotbinid.inl==extrbid1.inl)
		 || (cs_.defaultDir()==CubeSampling::Crl
		     && knotbinid.crl==extrbid1.crl) )
	    {
		if ( cs_.defaultDir() == CubeSampling::Inl )
		    stickauxdata.poly_ += FlatView::Point(
			    		SI().transform(pos.coord()).crl, pos.z);
		else if ( cs_.defaultDir() == CubeSampling::Crl )
		    stickauxdata.poly_ += FlatView::Point(
			    		SI().transform(pos.coord()).inl, pos.z);
	    }
	}
    }
    else
    {
	for ( rc.col=crg.start; rc.col<=crg.stop; rc.col+=crg.step )
	{
	    const Coord3 pos = fss.getKnot( rc );
	    if ( !mIsEqual(pos.z,cs_.zrg.start,.0001) )
		break;

	    BinID binid = SI().transform(pos.coord());
	    stickauxdata.poly_ += FlatView::Point( binid.inl, binid.crl );
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
    for ( rc.col=crg.start;rc.col<=crg.stop;rc.col+=crg.step )
    {
	const Coord3 pos = fss.getKnot( rc );
	bid = SI().transform( pos.coord() );
	int idx = path_->indexOf( bid );

	if ( idx < 0 ) continue;

	Coord3 editnormal( getNormalInRandLine(idx), 0 );
	const Coord3 nzednor = editnormal.normalize();

	const bool equinormal =
	    mIsEqual(nzednor.x,stkednor.x,.001) &&
	    mIsEqual(nzednor.y,stkednor.y,.001) &&
	    mIsEqual(nzednor.z,stkednor.z,.00001);

	if ( !equinormal )
	    return false;

	stickauxdata.poly_ += FlatView::Point( flatposdata_->position(true,idx),
					       pos.z );
    }
    return true;
}


bool Fault3DPainter::paintIntersection( EM::Fault3D& f3d,
					const EM::SectionID& sid,
					Fault3DMarker* f3dmaker )
{
    PtrMan<Geometry::IndexedShape> faultsurf =
		    new Geometry::ExplFaultStickSurface(
			f3d.geometry().sectionGeometry(sid), SI().zScale() );
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

	for ( int bdptidx=1; bdptidx<bendpts_.size(); bdptidx++ )
	{
	    pts.erase();

	    Coord3 p0( SI().transform((*path_)[bendpts_[bdptidx-1]]), zstart );
	    Coord3 p1( SI().transform((*path_)[bendpts_[bdptidx-1]]), zstop );
	    Coord3 p2( SI().transform((*path_)[bendpts_[bdptidx]]), zstart );
	    Coord3 p3( SI().transform((*path_)[bendpts_[bdptidx]]), zstop );

	    pts += p0; pts += p1; pts += p2; pts += p3;

	    if ( paintPlaneIntxn(f3d,f3dmaker,intxn,pts) )
		status = true;
	}

	return status;
    }
    else
    {
	BinID start( cs_.hrg.start.inl, cs_.hrg.start.crl );
	BinID stop(cs_.hrg.stop.inl, cs_.hrg.stop.crl );

	Coord3 p0( SI().transform(start), cs_.zrg.start );
	Coord3 p1( SI().transform(start), cs_.zrg.stop );
	Coord3 p2( SI().transform(stop), cs_.zrg.start );
	Coord3 p3( SI().transform(stop), cs_.zrg.stop );

	TypeSet<Coord3> pts;
	
	pts += p0; pts += p1; pts += p2; pts += p3;

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

    const Geometry::IndexedGeometry* idxgeom = idxshape->getGeometry()[0];
    TypeSet<int> coordindices = idxgeom->coordindices_;

    if ( !coordindices.size() )
	return false;

    Coord3List* clist = idxshape->coordList();
    mDynamicCastGet(Coord3ListImpl*,intxnposlist,clist);
    TypeSet<Coord3> intxnposs;
    if ( intxnposlist->getSize() )
    {
	int nextposid = intxnposlist->nextID( -1 );
	while ( nextposid!=-1 )
	{
	    if ( intxnposlist->isDefined(nextposid) )
		intxnposs += intxnposlist->get( nextposid );
	    nextposid = intxnposlist->nextID( nextposid );
	}
    }


    genIntersectionAuxData( f3d, f3dmaker, coordindices, intxnposs );

    return true;
}


void Fault3DPainter::genIntersectionAuxData( EM::Fault3D& f3d,
					    Fault3DMarker* f3dmaker,
					    TypeSet<int>& coordindices,
					    TypeSet<Coord3>& intxnposs)
{
    FlatView::AuxData* intsecauxdat = viewer_.createAuxData( 0 );

    intsecauxdat->poly_.erase();
    intsecauxdat->linestyle_ = markerlinestyle_;
    intsecauxdat->linestyle_.width_ = markerlinestyle_.width_/2;
    intsecauxdat->linestyle_.color_ = f3d.preferredColor();
    intsecauxdat->enabled_ = linenabled_;

    for ( int idx=0; idx<coordindices.size(); idx++ )
    {
	if ( coordindices[idx] == -1 )
	{
	    viewer_.addAuxData( intsecauxdat );
	    f3dmaker->intsecmarker_ += intsecauxdat;
	    intsecauxdat = viewer_.createAuxData( 0 );
	    intsecauxdat->poly_.erase();
	    intsecauxdat->linestyle_ = markerlinestyle_;
	    intsecauxdat->linestyle_.width_ = markerlinestyle_.width_/2;
	    intsecauxdat->linestyle_.color_ = f3d.preferredColor();
	    intsecauxdat->enabled_ = linenabled_;
	    continue;
	}

	const Coord3 pos = intxnposs[coordindices[idx]];
	BinID posbid =  SI().transform( pos.coord() );

	if ( path_ )
	{
	    int bididx = path_->indexOf( posbid );
	    if ( bididx != -1 )
		intsecauxdat->poly_ += FlatView::Point(
			flatposdata_->position(true,bididx), pos.z );

	    continue;
	}

	if ( cs_.nrZ() == 1 )
	    intsecauxdat->poly_ += FlatView::Point( posbid.inl, posbid.crl );
	else if ( cs_.nrCrl() == 1 )
	    intsecauxdat->poly_ += FlatView::Point( posbid.inl, pos.z );
	else if ( cs_.nrInl() == 1 )
	    intsecauxdat->poly_ += FlatView::Point( posbid.crl, pos.z );
    }

    viewer_.addAuxData( intsecauxdat );
    f3dmaker->intsecmarker_ += intsecauxdat;
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


void Fault3DPainter::setActiveStick( EM::PosID& pid )
{
    if ( pid.objectID() != emid_ ) return;

    if ( pid.getRowCol().row == activestickid_ ) return;

    for ( int auxdid=0; auxdid<f3dmarkers_[0]->stickmarker_.size(); auxdid++ )
    {
	LineStyle& linestyle = 
	    f3dmarkers_[0]->stickmarker_[auxdid]->marker_->linestyle_;
	if ( f3dmarkers_[0]->stickmarker_[auxdid]->stickid_== activestickid_ )
	    linestyle.width_ = markerlinestyle_.width_;
	else if ( f3dmarkers_[0]->stickmarker_[auxdid]->stickid_ ==
		  pid.getRowCol().row )
	    linestyle.width_ = markerlinestyle_.width_ * 2;
    }

    activestickid_ = pid.getRowCol().row;
    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


bool Fault3DPainter::hasDiffActiveStick( const EM::PosID* pid ) const
{
    if ( pid->objectID() != emid_ ||
	 pid->getRowCol().row != activestickid_ )
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


void Fault3DPainter::repaintFault3D()
{
    removePolyLine();
    addPolyLine();
    viewer_.handleChange( FlatView::Viewer::Auxdata );
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

			mrks.stickmarker_[stid]->marker_->linestyle_.color_ = 
			    				emf3d->preferredColor();
			viewer_.updateProperties(
					*mrks.stickmarker_[stid]->marker_ );
		    }
		    
		    for ( int itid=0; itid<mrks.intsecmarker_.size(); itid++ )
		    {
			if ( !mrks.intsecmarker_[itid] ) continue;

			mrks.intsecmarker_[itid]->linestyle_.color_ =
			    				emf3d->preferredColor();
			viewer_.updateProperties( *mrks.intsecmarker_[itid] );
		    }
		}
		break;
	    }
	case EM::EMObjectCallbackData::PositionChange:
	{
	    if ( emf3d->hasBurstAlert() )
		return;
	    abouttorepaint_.trigger();
	    repaintFault3D();
	    repaintdone_.trigger();
	    break;
	}
	case EM::EMObjectCallbackData::BurstAlert:
	{
	    if (  emobject->hasBurstAlert() )
		return;
	    abouttorepaint_.trigger();
	    repaintFault3D();
	    repaintdone_.trigger();
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

    BinID pivotbid = (*path_)[idx];
    BinID nextbid;

    if ( idx+1 < path_->size() )
	nextbid = (*path_)[idx+1];
    else if ( idx-1 > 0 )
	nextbid = (*path_)[idx-1];

    if ( pivotbid.inl == nextbid.inl )
	return  SI().binID2Coord().rowDir();
    else if ( pivotbid.crl == nextbid.crl )
	return SI().binID2Coord().colDir();

    return Coord(mUdf(float), mUdf(float));
}


} //namespace EM

