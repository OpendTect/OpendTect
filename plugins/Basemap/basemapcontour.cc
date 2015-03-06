/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		February 2015
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "basemapcontour.h"

#include "binid.h"
#include "isocontourtracer.h"
#include "task.h"
#include "survinfo.h"

#include "emhorizon3d.h"
#include "emioobjinfo.h"
#include "emmanager.h"


namespace Basemap
{

class PolygonHolder
{
public:
~PolygonHolder()    { polygons_.erase(); }

    ObjectSet<ODPolygon<float> > polygons_;
};


class ContourExtractor : public ParallelTask
{
public:
ContourExtractor( const Array2D<float>& zvalues,
		  const StepInterval<float>& ix,
		  ObjectSet<ODPolygon<float> >& polys,
		  TypeSet<float>& contourvals )
    : zvalues_(zvalues)
    , zintv_(ix)
    , polygons_(polys)
    , contourvals_(contourvals)

{}

uiString uiNrDoneText() const	{ return "Contours Done"; }

int maxNrThreads() const	{ return 1; }

od_int64 nrIterations() const	{ return zintv_.nrSteps(); }

bool doPrepare( int nrthreads )
{
    for ( int idx=0; idx<nrthreads; idx++ )
	thrdpolygons_ += new PolygonHolder;

    return true;
}

bool doWork( od_int64 start, od_int64 stop, int threadidx )
{
    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	const float zval = zintv_.atIndex( idx );
	extractPolygons( zvalues_, zval, thrdpolygons_[threadidx]->polygons_ );
	addToNrDone( 1 );
    }

    return true;
}

bool doFinish( bool success )
{
    for ( int idx=0; idx<thrdpolygons_.size(); idx++ )
	polygons_.append( thrdpolygons_[idx]->polygons_ );

    deepErase( thrdpolygons_ );
    return true;
}

protected:

    const Array2D<float>&   zvalues_;
    StepInterval<float>     zintv_;
    ObjectSet<ODPolygon<float> >&    polygons_;
    ObjectSet<PolygonHolder>   thrdpolygons_;
    TypeSet<float>&	    contourvals_;

private:
void extractPolygons( const Array2D<float>& area, float z,
		      ObjectSet<ODPolygon<float> >& allpolygons )
{
    IsoContourTracer contours( area );
    contours.setBendPointsOnly( 0.1 );
    ObjectSet<ODPolygon<float> > polygons;
    const bool res = contours.getContours( polygons, z );
    if ( !res ) return;

    allpolygons.append( polygons );
    for ( int idx=0; idx<polygons.size(); idx++ )
	contourvals_ += z*SI().zDomain().userFactor();
}

};



ContourObject::ContourObject()
    : BaseMapObject(0)
    , horrange_(*new TrcKeySampling)
    , hor3d_(0)
{}


ContourObject::~ContourObject()
{
    delete &horrange_;
    deepErase( polygons_ );
    if ( hor3d_ ) hor3d_->unRef();
}


void ContourObject::updateGeometry()
{
    changed.trigger();
}


void ContourObject::setLineStyle( int idx, const LineStyle& ls )
{
    ls_ = ls;
    stylechanged.trigger();
}


void ContourObject::setMultiID( const MultiID& mid, TaskRunner* tsk )
{
    if ( hor3d_ ) hor3d_->unRef();
    hormid_ = mid;
    RefMan<EM::EMObject> emobj = EM::EMM().loadIfNotFullyLoaded( hormid_, tsk );
    mDynamicCast(EM::Horizon3D*,hor3d_,emobj.ptr())
    if ( !hor3d_ ) return;

    hor3d_->ref();
    setName( hor3d_->name() );
    horrange_ = hor3d_->range();
}


void ContourObject::setContours( const StepInterval<float>& ix,
				 TaskRunner* tsk )
{
    if ( !hor3d_ ) return;

    PtrMan<Array2D<float> > zvalues =
		hor3d_->createArray2D( hor3d_->sectionID(0) );
    if ( !zvalues ) return;

    deepErase( polygons_ );
    ContourExtractor ce( *zvalues, ix, polygons_, contourvals_ );
    if ( !TaskRunner::execute(tsk,ce) ) return;
}


int ContourObject::nrShapes() const
{ return polygons_.size(); }


const char* ContourObject::getShapeName( int idx ) const
{
    mDeclStaticString( str );
    str.set( contourvals_[idx], 0 );
    return str.buf();
}


bool ContourObject::extractPolygons( const Array2DImpl<float>& area, float z )
{
    IsoContourTracer contours( area );
    contours.setBendPointsOnly( 0.1 );
    ObjectSet<ODPolygon<float> > polygons;
    const bool res = contours.getContours( polygons, z );
    if ( !res ) return false;

    polygons_.append( polygons );
    polygons.erase();
    return true;
}


void ContourObject::getPoints( int shapeidx, TypeSet<Coord>& pts ) const
{
    if ( !polygons_.validIdx(shapeidx) ) return;

    const StepInterval<int> inlrg = horrange_.inlRange();
    const StepInterval<int> crlrg = horrange_.crlRange();

    const ODPolygon<float>& poly = *polygons_[shapeidx];
    Pos::IdxPair2Coord& tf = SI().getBinID2Coord();

    for ( int idy=0; idy<poly.size(); idy++ )
    {
	const Geom::Point2D<float>& pt = poly.getVertex( idy );
	const Coord bidf( inlrg.start + pt.x*inlrg.step,
			  crlrg.start + pt.y*crlrg.step );
	pts.add( tf.transform(bidf) );
    }
}

} // namespace Basemap

