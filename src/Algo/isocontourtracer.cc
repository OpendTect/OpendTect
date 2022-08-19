/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "isocontourtracer.h"

#include "arrayndinfo.h"
#include "threadwork.h"


#define mOnEdge(xy,idx)		(idx<edge_ || idx>xy##range_.width()+edge_)
#define mFieldIdx(xy,idx)	(xy##range_.start + idx - edge_)

#define mFieldX(idx)		xsampling_.atIndex( mFieldIdx(x,idx) )
#define mFieldY(idy)		ysampling_.atIndex( mFieldIdx(y,idy) )

#define mMakeVertex( vertex, idx, idy, hor, frac ) \
\
    const Geom::Point2D<float> vertex( \
			(1.0f-frac)*mFieldX(idx) + frac*mFieldX(idx+hor), \
			(1.0f-frac)*mFieldY(idy) + frac*mFieldY(idy+1-hor) );


class SameZFinder : public ParallelTask
{
public:
				SameZFinder( const Array2D<float>* field,
					     float* zarray, int* execs,
					     int xsize, int ysize,
					     const ODPolygon<float>*polyroi,
					     const float zval,
					     const float edgevalue);
    void			setRanges(const Interval<int>&,
					  const Interval<int>&);
    void			setSamplings(const StepInterval<int>&,
					     const StepInterval<int>&);
				~SameZFinder(){};
protected:
    bool			doWork(od_int64,od_int64,int) override;
    od_int64			nrIterations() const override
				{ return totalnr_; }

private:
    void			findDataWithTheZ(int idx,float z);
    Threads::Atomic<od_int64>	totalnr_;
    float*			zarray_;
    int*			execs_;
    int				xsize_;
    int				ysize_;
    const Array2D<float>*	field_;
    const float			zval_;
    const float			edgevalue_;
    const ODPolygon<float>*	polyroi_;
    unsigned int		edge_;
    Interval<int>		xrange_;
    Interval<int>		yrange_;
    StepInterval<int>		xsampling_;
    StepInterval<int>		ysampling_;
};


SameZFinder::SameZFinder( const Array2D<float>* field, float* zarray,
	int* execs, int xsize, int ysize, const ODPolygon<float>*polyroi,
	const float zval, const float edgevalue )
    : field_( field )
    , edgevalue_( edgevalue )
    , polyroi_( polyroi )
    , zarray_( zarray )
    , execs_( execs )
    , xsize_( xsize )
    , ysize_( ysize )
    , zval_( zval )
    , edge_(0)
{
    edge_ = mIsUdf(edgevalue) ? 0 : 1;
    totalnr_ = xsize;
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
    for ( od_int64 idx=start; idx<=stop; idx++ )
	findDataWithTheZ( (int)idx, zval_ );
    return true;
}

#define mNoExecutive		-1
#define mIsExecutive(val)	(val!=mNoExecutive)
#define mNoFraction		-1.0f
#define mIsFraction(val)	(val>-0.5f)

void SameZFinder::findDataWithTheZ( int idx, float z )
{
    for ( int idy=0; idy<ysize_; idy++ )
    {
	float z0 = mUdf(float);
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
	    const int zarrayidx = (idx*ysize_+idy)*2+hor;
	    zarray_[zarrayidx] = mNoFraction;
	    if ( !mFastIsFloatDefined(z0) )
		continue;
	    if ( (hor && idx==xsize_-1) || (!hor && idy==ysize_-1) )
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
		if ( !mFastIsFloatDefined(z1) )
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
		zarray_[zarrayidx] = frac;
		execs_[zarrayidx] = mNoExecutive;
	    }
	}
    }
}


class ContourTracer : public ParallelTask
{
public:
			    ContourTracer(
					ObjectSet<ODPolygon<float> >& contours,
					float* crossings, int* execs, int xsize,
					int ysize, unsigned int edge,
					float bendpointeps, int nrlargestonly,
					int minnrvertices, bool closedonly);
    void		    setRanges(const Interval<int>&,
				      const Interval<int>&);
    void		    setSamplings(const StepInterval<int>&,
					 const StepInterval<int>&);
			    ~ContourTracer(){};

protected:
    bool		    doWork(od_int64 start,od_int64 stop,int) override;
    od_int64		    nrIterations() const override { return totalnr_; }

private:

    void		    addVertex(ODPolygon<float>& contour,
				      bool headinsert,int idx,int idy,
				      int hor,float frac) const;
    bool		    nextCrossing(int& idx,int& idy,int& hor,int& up,
					 float& frac);

    enum ExecutiveMode	    { Approved, Confirmed, Refused };
    ExecutiveMode	    setExecutive(int idx,int executive,bool takeover);
    int			    nextExecutive();

    float*			crossings_;
    int*			execs_;
    ObjectSet<ODPolygon<float> >& contours_;
    Threads::Atomic<od_int64>	totalnr_;
    Interval<int>		xrange_;
    Interval<int>		yrange_;
    StepInterval<int>		xsampling_;
    StepInterval<int>		ysampling_;
    float			bendpointeps_;
    int				minnrvertices_;
    unsigned int		edge_;
    int				xsize_;
    int				ysize_;
    bool			closedonly_;
    int				nextexecutive_;
    Threads::Lock		contourlock_;
    Threads::Lock		execslock_;
    Threads::Lock		nextexecutivelock_;
};


ContourTracer::ContourTracer( ObjectSet<ODPolygon<float> >& contours,
    float* crossings, int* execs, int xsize, int ysize, unsigned int edge,
    float bendpointeps, int nrlargestonly, int minnrvertices, bool closedonly )
    : crossings_( crossings )
    , contours_( contours )
    , execs_( execs )
    , xsize_( xsize )
    , ysize_( ysize )
    , edge_( edge )
    , bendpointeps_( bendpointeps )
    , minnrvertices_( minnrvertices )
    , closedonly_( closedonly )
    , totalnr_( 0 )
    , nextexecutive_( 0 )
    , contourlock_( Threads::Lock::SmallWork )
    , execslock_( Threads::Lock::SmallWork )
    , nextexecutivelock_( Threads::Lock::SmallWork )
{
    totalnr_ = xsize * ysize;
}


ContourTracer::ExecutiveMode ContourTracer::setExecutive(
					int idx, int executive, bool takeover )
{
    Threads::Locker locker( execslock_, Threads::Locker::WriteLock );
    if ( execs_[idx] == executive )
	return Confirmed;

    if ( !mIsExecutive(execs_[idx]) || (takeover && execs_[idx]>executive) )
    {
	execs_[idx] = executive;
	return Approved;
    }

    return Refused;
}


int ContourTracer::nextExecutive()
{
    Threads::Locker locker( nextexecutivelock_, Threads::Locker::WriteLock );
    return nextexecutive_++;
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


bool ContourTracer::nextCrossing( int& idx, int& idy, int& hor, int& up,
				  float& frac )
{
    const int lidx =   up    ? idx : ( hor ? idx+1 : idx-1 );	/* [l]eft */
    const int lidy = up==hor ? idy : ( hor ? idy-1 : idy+1 );

    float lfrac = mNoFraction;
    if ( lidx>=0 && lidx<xsize_ && lidy>=0 && lidy<ysize_ )
	lfrac = crossings_[(lidx*ysize_+lidy)*2+1-hor];

    const int ridx = up!=hor ? idx : ( hor ? idx+1 : idx-1 );	/* [r]ight */
    const int ridy =   up    ? idy : ( hor ? idy-1 : idy+1 );

    float rfrac = mNoFraction;
    if ( !mIsFraction(lfrac) || hor )
    {
	if ( ridx>=0 && ridx<xsize_ && ridy>=0 && ridy<ysize_ )
	    rfrac = crossings_[(ridx*ysize_+ridy)*2+1-hor];
    }

    if ( mIsFraction(lfrac) && mIsFraction(rfrac) )
    {
	// Tie-break (direction invariance required for multi-threading)
	if ( hor )
	    lfrac = mNoFraction;
	else
	    rfrac = mNoFraction;
    }

    if ( mIsFraction(lfrac) )
    {
	idx = lidx; idy = lidy; frac = lfrac; hor = 1-hor;
	up = up==hor ? 1 : 0;
	return true;
    }

    if ( mIsFraction(rfrac) )
    {
	idx = ridx; idy = ridy; frac = rfrac; hor = 1-hor;
	up = up==hor ? 0 : 1;
	return true;
    }

    const int sidx =  hor ? idx : ( up ? idx+1 : idx-1 );	/* [s]traight */
    const int sidy = !hor ? idy : ( up ? idy+1 : idy-1 );

    float sfrac = mNoFraction;
    if ( sidx>=0 && sidx<xsize_ && sidy>=0 && sidy<ysize_ )
	sfrac = crossings_[(sidx*ysize_+sidy)*2+hor];

    if ( !mIsFraction(sfrac) )
	return false;

    idx = sidx; idy = sidy; frac = sfrac;
    return true;
}


bool ContourTracer::doWork( od_int64 start, od_int64 stop, int )
{
    // Minimize wasted work: (Horizon) contours often align with shorter dim.
    const bool xisfastdim = xsize_ <= ysize_;

    int pidx = mCast( int, xisfastdim ? start%xsize_ : start/ysize_ );
    int pidy = mCast( int, xisfastdim ? start/xsize_ : start%ysize_ );

    for ( od_int64 count=start; count<=stop; count++ )
    {
	for ( int phor=0; phor<=1; phor++ )
	{
	    const int pivotidx = (pidx*ysize_+pidy)*2 + phor;
	    const float pfrac = crossings_[pivotidx];
	    if ( !mIsFraction(pfrac) || mIsExecutive(execs_[pivotidx]) )
		continue;

	    const int executive = nextExecutive();
	    if ( setExecutive(pivotidx,executive,false) == Refused )
		continue;

	    ODPolygon<float>* contour = new ODPolygon<float>();
	    contour->setClosed( false );
	    addVertex( *contour, false, pidx, pidy, phor, pfrac );

	    for ( int pup=0; pup<=1; pup++ )
	    {
		int idx = pidx; int idy = pidy; int hor = phor;
		int up = pup; float frac = pfrac;

		while ( nextCrossing(idx,idy,hor,up,frac) )
		{
		    const ExecutiveMode res =
			setExecutive( (idx*ysize_+idy)*2+hor, executive, true );

		    if ( res != Approved )
		    {
			if ( res == Confirmed )
			    contour->setClosed( true );
			else	/* Refused */
			    contour->setEmpty();

			pup = 1;	// escape from up/down loop too
			break;
		    }

		    addVertex( *contour, pup, idx, idy, hor, frac );
		}
	    }

	    if ( mFastIsFloatDefined(bendpointeps_) )
		contour->keepBendPoints( bendpointeps_ );

	    const int sz = contour->size();
	    if ( sz<minnrvertices_ || (closedonly_ && !contour->isClosed()) )
		delete contour;
	    else
	    {
		Threads::Locker locker(contourlock_,Threads::Locker::WriteLock);
		contours_ += contour;
	    }
	}

	if ( xisfastdim )
	{
	    if ( ++pidx >= xsize_ )
	    {
		++pidy;
		pidx = 0;
	    }
	}
	else if ( ++pidy >= ysize_ )
	{
	    ++pidx;
	    pidy = 0;
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
    , edgepar_( 0 )
{
}


IsoContourTracer::~IsoContourTracer()
{
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
    edgepar_ = mIsUdf(edgeval) ? 0 : 1;
    edgevalue_ = edgeval;
}


bool IsoContourTracer::getContours( ObjectSet<ODPolygon<float> >& contours,
				    float z, bool closedonly ) const
{
    deepErase( contours );
    const int xsize = xrange_.width() + 2*edgepar_ + 1;
    const int ysize = yrange_.width() + 2*edgepar_ + 1;
    float* crossings = new float[ xsize*ysize*2 ];
    int* execs = new int[ xsize*ysize*2 ];

    SameZFinder finder( &field_, crossings, execs, xsize, ysize, polyroi_, z,
			edgevalue_);
    finder.setRanges( xrange_, yrange_ );
    finder.setSamplings( xsampling_, ysampling_ );
    if ( finder.execute() )
    {
	ContourTracer tracer( contours, crossings, execs, xsize, ysize,
			      edgepar_, bendpointeps_, nrlargestonly_,
			      minnrvertices_, closedonly );
	tracer.setRanges( xrange_, yrange_ );
	tracer.setSamplings( xsampling_, ysampling_ );
	tracer.execute();
    }
    delete [] crossings;
    delete [] execs;

    if ( nrlargestonly_>0 && contours.size()>nrlargestonly_ )
    {
	sort( contours );
	contours.reverse();

	for ( int idx=contours.size()-1; idx>=nrlargestonly_; idx-- )
	    contours.removeSingle( idx );
    }

    return !contours.isEmpty();
}
