/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : J.C. Glas
 * DATE     : November 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "isocontourtracer.h"

#include "arrayndinfo.h"
#include "threadwork.h"
#include "hiddenparam.h"

static HiddenParam< IsoContourTracer, unsigned int > edgepar_( 0 );


#define mOnEdge(xy,idx)		(idx<edge_ || idx>xy##range_.width()+edge_)
#define mFieldIdx(xy,idx)	(xy##range_.start + idx - edge_)

#define mFieldX(idx)		xsampling_.atIndex( mFieldIdx(x,idx) )
#define mFieldY(idy)		ysampling_.atIndex( mFieldIdx(y,idy) )

#define mFieldZ(idx,idy) ( mOnEdge(x,idx) || mOnEdge(y,idy) ? edgevalue_ : \
			   field_.get(mFieldIdx(x,idx), mFieldIdx(y,idy)) )

#define mMakeVertex( vertex, idx, idy, hor, frac ) \
\
    const Geom::Point2D<float> vertex( \
			(1.0f-frac)*mFieldX(idx) + frac*mFieldX(idx+hor), \
			(1.0f-frac)*mFieldY(idy) + frac*mFieldY(idy+1-hor) );


static bool nextCrossing( Array3DImpl<float>& crossings, int& idx, int& idy,
			  int& hor, int& up, float& frac ) 
{
    const int xsize = crossings.info().getSize(0);
    const int ysize = crossings.info().getSize(1);

    const int lidx =   up    ? idx : ( hor ? idx+1 : idx-1 );	/* [l]eft */
    const int lidy = up==hor ? idy : ( hor ? idy-1 : idy+1 );
    const int ridx = up!=hor ? idx : ( hor ? idx+1 : idx-1 );	/* [r]ight */
    const int ridy =   up    ? idy : ( hor ? idy-1 : idy+1 );

    float lfrac = mUdf(float); 
    if ( lidx>=0 && lidx<xsize && lidy>=0 && lidy<ysize ) 
	lfrac = crossings.get( lidx, lidy, 1-hor );

    float rfrac = mUdf(float); 
    if ( ridx>=0 && ridx<xsize && ridy>=0 && ridy<ysize ) 
	rfrac = crossings.get( ridx, ridy, 1-hor );

    if ( !mIsUdf(lfrac) && !mIsUdf(rfrac) )			/* Tie-break */
    {
	const float ldist =   up    ? lfrac : 1.0f-lfrac;
	const float rdist =   up    ? rfrac : 1.0f-rfrac;
	const float  dist = up==hor ?  frac : 1.0f-frac;

	if ( ldist*ldist+dist*dist > rdist*rdist+(1.0-dist)*(1.0-dist) )
	    lfrac = mUdf(float);
	else
	    rfrac = mUdf(float);
    }

    if ( !mIsUdf(lfrac) )
    {
	idx = lidx; idy = lidy; frac = lfrac; hor = 1-hor;
	up = up==hor ? 1 : 0;
	return true;
    }

    if ( !mIsUdf(rfrac) )
    {
	idx = ridx; idy = ridy; frac = rfrac; hor = 1-hor;
	up = up==hor ? 0 : 1;
	return true;
    }
		
    const int sidx =  hor ? idx : ( up ? idx+1 : idx-1 );	/* [s]traight */
    const int sidy = !hor ? idy : ( up ? idy+1 : idy-1 );

    float sfrac = mUdf(float); 
    if ( sidx>=0 && sidx<xsize && sidy>=0 && sidy<ysize ) 
	sfrac = crossings.get( sidx, sidy, hor );
    
    if ( mIsUdf(sfrac) )
	return false;
    
    idx = sidx; idy = sidy; frac = sfrac;
    return true;
}


class SameZFinder : public ParallelTask
{ 
public:
				SameZFinder( const Array2D<float>* field,
					     Array3DImpl<float>* zarray, 
					     const ODPolygon<float>*polyroi, 
					     const float zval,
					     const float edgevalue);
    void			setRanges(const Interval<int>&,
					  const Interval<int>&);
    void			setSamplings(const StepInterval<int>&,
					     const StepInterval<int>&);
				~SameZFinder(){};
protected:
    bool			doWork(od_int64 start, od_int64 stop, int);
    od_int64			nrIterations() const { return totalnr_; }

private:
    void			findDataWithTheZ(int idx,float z);
    Threads::Atomic<od_int64>	totalnr_;
    Array3DImpl<float>*		zarray_;
    const Array2D<float>*	field_;
    const float			zval_;
    const float			edgevalue_;
    const ODPolygon<float>*	polyroi_;
    unsigned int		edge_;
    Interval<int>		xrange_;
    Interval<int>		yrange_;
    StepInterval<int>		xsampling_;
    StepInterval<int>		ysampling_;
    Threads::Mutex		lock_;
};


class ContourTracer : public ParallelTask
{ 
public:
			    ContourTracer(
					ObjectSet<ODPolygon<float> >& contours,
					Array3DImpl<float>* crossings,
					unsigned int edge,float bendpointeps,
					int nrlargestonly,int minnrvertices,
					bool closedonly);
    void		    setRanges(const Interval<int>&,
				      const Interval<int>&);
    void		    setSamplings(const StepInterval<int>&,
					 const StepInterval<int>&);
			    ~ContourTracer(){};

protected:
    bool		    doWork(od_int64 start, od_int64 stop, int);
    od_int64		    nrIterations() const { return totalnr_; }

private:
    void		    addVertex(ODPolygon<float>& contour,
				      bool headinsert,int idx,int idy,
				      int hor,float frac) const;

    IsoContourTracer*		isotracer_;
    Array3DImpl<float>*		crossings_;
    ObjectSet<ODPolygon<float> >& contours_;
    Threads::Atomic<od_int64>	totalnr_;
    Interval<int>		xrange_;
    Interval<int>		yrange_;
    StepInterval<int>		xsampling_;
    StepInterval<int>		ysampling_;
    float			bendpointeps_;
    int				minnrvertices_;
    int				nrlargestonly_;
    unsigned int		edge_;
    int				xsize_;
    int				ysize_;
    bool			closedonly_;
    Threads::Mutex		lock_;
};


SameZFinder::SameZFinder( const Array2D<float>* field, 
    Array3DImpl<float>* zarray, const ODPolygon<float>*polyroi,
    const float zval, const float edgevalue )
    : field_( field )
    , edgevalue_( edgevalue )
    , polyroi_( polyroi )
    , zarray_( zarray )
    , zval_( zval )
    , edge_(0)
{
    edge_ = mIsUdf(edgevalue) ? 0 : 1;
    totalnr_ = zarray_ ? zarray_->info().getSize(0) : 0 ;
}

void SameZFinder::setRanges( const Interval<int>&xrange, 
    const Interval<int>& yrange )
{
    xrange_ = xrange;
    yrange_ = yrange;
}


void SameZFinder::setSamplings( const StepInterval<int>& xsampling,
    const StepInterval<int>& ysampling )
{
    xsampling_ = xsampling;
    ysampling_ = ysampling;
}


bool SameZFinder::doWork( od_int64 start, od_int64 stop, int )
{
    for ( od_int64 idx=start; idx<=stop && shouldContinue(); idx++ )
	findDataWithTheZ( (int)idx, zval_ );
    return true;
}


void SameZFinder::findDataWithTheZ( int idx, float z )
{
    const int xsize = zarray_->info().getSize(0);
    const int ysize = zarray_->info().getSize(1);
    for ( int idy=0; idy<ysize; idy++ )
    {
	float z0 = mUdf( float );
	if ( mOnEdge(x,idx) || mOnEdge(y,idy) )
	{
	    if ( edge_==0 )
		continue;
	    z0 = edgevalue_;
	}
	else
	{
	    z0 = field_->get(mFieldIdx(x,idx), mFieldIdx(y,idy));
	}
	for ( int hor=0; hor<=1; hor++ )
	{
	    zarray_->set( idx, idy, hor, mUdf(float) );
	    if ( mIsUdf(z0) )
		continue;
	    if ( (hor && idx==xsize-1) || (!hor && idy==ysize-1) )
		continue;
	    const int tmpidx = idx+hor;
	    const int tmpidy = idy+1-hor;
	    float z1 = mUdf(float);
	    if ( mOnEdge(x,tmpidx) || mOnEdge(y,tmpidy) )
	    {
		if ( edge_==0 )
		    continue;
		z1 = edgevalue_;
	    }
	    else
	    {
		z1 = field_->get(mFieldIdx(x,tmpidx), mFieldIdx(y,tmpidy));
		if ( mIsUdf(z1) )
		    continue;
	    }
	    if ( (z0<z && z<=z1) || (z1<z && z<=z0) )
	    {
		const float frac = (z-z0) / (z1-z0);
		if ( polyroi_ )
		{
		    mMakeVertex( vertex, idx, idy, hor, frac );
		    if ( !polyroi_->isInside(vertex, true, mDefEps) )
			continue;
		}
		zarray_->set( idx, idy, hor, frac );
	    }
	}
    }
}


ContourTracer::ContourTracer( ObjectSet<ODPolygon<float> >& contours,
    Array3DImpl<float>* crossings, unsigned int edge, float bendpointeps,
    int nrlargestonly, int minnrvertices, bool closedonly )
    : crossings_( crossings )
    , contours_( contours )
    , edge_( edge )
    , bendpointeps_( bendpointeps )
    , nrlargestonly_( nrlargestonly )
    , minnrvertices_( minnrvertices )
    , closedonly_( closedonly )
    , xsize_( 0 )
    , ysize_( 0 )
    , totalnr_( 0 )
{
    xsize_ = crossings_->info().getSize(0);
    ysize_ = crossings_->info().getSize(1);
    totalnr_ = xsize_*ysize_;
}


void ContourTracer::setRanges(const Interval<int>&xrange,
    const Interval<int>& yrange)
{
    xrange_ = xrange;
    yrange_ = yrange;
}


void ContourTracer::setSamplings(const StepInterval<int>& xsampling,
    const StepInterval<int>& ysampling)
{
    xsampling_ = xsampling;
    ysampling_ = ysampling;
}


void ContourTracer::addVertex( ODPolygon<float>& contour, bool headinsert,
			       int idx, int idy, int hor, float frac ) const
{
    mMakeVertex( vertex, idx, idy, hor, frac );

    const int prev = headinsert ? 0 : contour.size()-1;
    if ( contour.size() && vertex==contour.getVertex(prev) )
	return;

    contour.insert( (headinsert ? 0 : prev+1), vertex );
}


bool ContourTracer::doWork( od_int64 start, od_int64 stop, int )
{
    for ( od_int64 idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int pidy = (int)Math::Floor((double)idx/xsize_);
	const int pidx = (int)idx - pidy*xsize_;
	for ( int phor=0; phor<=1; phor++ )
	{
	    const float pfrac = crossings_->get( pidx, pidy, phor );
	    if ( mIsUdf(pfrac) )
		continue;
	    ODPolygon<float>* contour = new ODPolygon<float>();
	    contour->setClosed( false );
	    for ( int pup=0; pup<=1; pup++ )
	    {
		int idxx = pidx; int idyy = pidy; int hor = phor;
		int up = pup; float frac = pfrac;
		if ( !pup && !nextCrossing(*crossings_,idxx,idyy,hor,up,frac) )
		    continue;
		do
		{
		    addVertex( *contour, pup, idxx, idyy, hor, frac );
		    crossings_->set( idxx, idyy, hor, mUdf(float) );
		}
		while( nextCrossing(*crossings_,idxx,idyy,hor,up,frac) );
		if ( !pup && idxx==pidx && idyy==pidy && hor==phor )
		{
		    contour->setClosed( true );
		    break;
		}
	    }
	    if ( !mIsUdf(bendpointeps_) )
		contour->keepBendPoints( bendpointeps_ );
	    const int sz = contour->size();
	    if ( sz<minnrvertices_ || (closedonly_ && !contour->isClosed()) )
		delete contour;
	    else
	    {
		Threads::MutexLocker lock( lock_ );
		contours_ += contour;
		lock.unLock();
	    }
	}
    }
    return true;

}


IsoContourTracer::IsoContourTracer( const Array2D<float>& field )
    : field_( field )
    , xsampling_( 0, field.info().getSize(0)-1, 1 )
    , ysampling_( 0, field.info().getSize(1)-1, 1 )
    , xrange_( 0, field.info().getSize(0)-1 )
    , yrange_( 0, field.info().getSize(1)-1 )
    , polyroi_( 0 )
    , minnrvertices_( 2 )
    , nrlargestonly_( -1 )
    , edgevalue_( mUdf(float) )
    , bendpointeps_( mUdf(float) )
{
    edgepar_.setParam( this, 0 );
}


IsoContourTracer::~IsoContourTracer()
{
    edgepar_.removeParam( this );
}


void IsoContourTracer::setSampling( const StepInterval<int>& xsamp,
				    const StepInterval<int>& ysamp )
{
    xsampling_ = xsamp;
    ysampling_ = ysamp;
    xsampling_.stop = xsampling_.atIndex( field_.info().getSize(0)-1 );
    ysampling_.stop = ysampling_.atIndex( field_.info().getSize(1)-1 );
}


void IsoContourTracer::selectRectROI( const Interval<int>& xintv,
				      const Interval<int>& yintv )
{
    xrange_.start = xsampling_.nearestIndex( xintv.start );
    xrange_.stop = xsampling_.nearestIndex( xintv.stop );
    yrange_.start = ysampling_.nearestIndex( yintv.start );
    yrange_.stop = ysampling_.nearestIndex( yintv.stop );
    xrange_.limitTo( Interval<int>(0, field_.info().getSize(0)-1) );
    yrange_.limitTo( Interval<int>(0, field_.info().getSize(1)-1) );
}

void IsoContourTracer::selectPolyROI( const ODPolygon<float>* poly )
{ polyroi_ = poly; }


void IsoContourTracer::setBendPointsOnly( float eps )
{ bendpointeps_ = eps; }


void IsoContourTracer::setMinNrVertices( int nr )
{ minnrvertices_ = nr>2 ? nr : 2; }


void IsoContourTracer::setNrLargestOnly( int nr )
{ nrlargestonly_ = nr>0 ? nr : -1; }


void IsoContourTracer::setEdgeValue( float edgeval )
{
    edgepar_.setParam( this, mIsUdf(edgeval) ? 0 : 1 );
    edgevalue_ = edgeval;
}


bool IsoContourTracer::getContours( ObjectSet<ODPolygon<float> >& contours,
				    float z, bool closedonly ) const
{
    deepErase( contours );
    const unsigned int edge = edgepar_.getParam(this);
    Array3DImpl<float>* crossings = new Array3DImpl<float>( 
	xrange_.width()+2*edge+1, yrange_.width()+2*edge+1, 2 );

    const bool multithread1 = !Threads::WorkManager::twm().isWorkThread();
    SameZFinder finder( &field_,crossings,polyroi_,z,edgevalue_ );
    finder.setRanges( xrange_, yrange_ );
    finder.setSamplings( xsampling_, ysampling_ );
    if ( finder.executeParallel( multithread1 ) )
    {
	const bool multithread2 = !Threads::WorkManager::twm().isWorkThread();
	ContourTracer tracer( contours, crossings, edge, bendpointeps_,
	    nrlargestonly_, minnrvertices_, closedonly );
	tracer.setRanges( xrange_, yrange_ );
	tracer.setSamplings( xsampling_, ysampling_ );
	tracer.executeParallel( multithread2 );
    }
    delete crossings;
    
    if ( nrlargestonly_>0 && contours.size()>nrlargestonly_ )
    {
	sort( contours );
	contours.reverse();

	for ( int idx=contours.size()-1; idx>=nrlargestonly_; idx-- )
	    contours.removeSingle( idx );
    }

    return !contours.isEmpty();
}

