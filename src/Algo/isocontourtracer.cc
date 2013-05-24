/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : J.C. Glas
 * DATE     : November 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "isocontourtracer.h"

#include "arrayndinfo.h"

static Threads::ReadWriteLock ictlock;
static ObjectSet<IsoContourTracer> isocontourtracers;
static ObjectSet<float> edgevalues;
static ObjectSet<float> bendpointepsilons;

static float& getEdgeValue( const IsoContourTracer* ict )
{
    ictlock.readLock();
    float& res = *edgevalues[isocontourtracers.indexOf(ict)];
    ictlock.readUnLock();
    return res;
}


static float& getBendPointEps( const IsoContourTracer* ict )
{
    ictlock.readLock();
    float& res = *bendpointepsilons[isocontourtracers.indexOf(ict)];
    ictlock.readUnLock();
    return res;
}

#define edgevalue_	getEdgeValue(this)
#define bendpointeps_	getBendPointEps(this)


IsoContourTracer::IsoContourTracer( const Array2D<float>& field )
    : field_( field )
    , xsampling_( 0, field.info().getSize(0)-1, 1 )
    , ysampling_( 0, field.info().getSize(1)-1, 1 )
    , xrange_( 0, field.info().getSize(0)-1 )
    , yrange_( 0, field.info().getSize(1)-1 )
    , polyroi_( 0 )
    , minnrvertices_( 2 )
    , nrlargestonly_( -1 )
{
    ictlock.writeLock();
    isocontourtracers += this;
    edgevalues += new float( mUdf(float) );
    bendpointepsilons += new float( mUdf(float) );
    ictlock.writeUnLock();
}


IsoContourTracer::~IsoContourTracer()
{
    ictlock.writeLock();
    delete edgevalues.removeSingle( isocontourtracers.indexOf(this) );
    delete bendpointepsilons.removeSingle( isocontourtracers.indexOf(this) );
    isocontourtracers -= this;
    ictlock.writeUnLock();
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
{ edgevalue_ = edgeval; }


#define mEdge			(mIsUdf(edgevalue_) ? 0 : 1)
#define mOnEdge(xy,idx)		(idx<mEdge || idx>xy##range_.width()+mEdge)
#define mFieldIdx(xy,idx)	(xy##range_.start + idx - mEdge)


#define mFieldX(idx)		xsampling_.atIndex( mFieldIdx(x,idx) )
#define mFieldY(idy)		ysampling_.atIndex( mFieldIdx(y,idy) )

#define mFieldZ(idx,idy) ( mOnEdge(x,idx) || mOnEdge(y,idy) ? edgevalue_ : \
			   field_.get(mFieldIdx(x,idx), mFieldIdx(y,idy)) )

#define mMakeVertex( vertex, idx, idy, hor, frac ) \
\
    const Geom::Point2D<float> vertex( \
			(1.0f-frac)*mFieldX(idx) + frac*mFieldX(idx+hor), \
			(1.0f-frac)*mFieldY(idy) + frac*mFieldY(idy+1-hor) );


bool IsoContourTracer::getContours( ObjectSet<ODPolygon<float> >& contours,
				    float z, bool closedonly ) const
{
    deepErase( contours );
    Array3DImpl<float>* crossings = new Array3DImpl<float>(
		    xrange_.width()+2*mEdge+1, yrange_.width()+2*mEdge+1, 2 );

    findCrossings( *crossings, z );
    traceContours( *crossings, contours, closedonly );
    delete crossings;

    return !contours.isEmpty();
}


void IsoContourTracer::findCrossings( Array3DImpl<float>& crossings, 
				      float z ) const
{
    const int xsize = crossings.info().getSize(0);
    const int ysize = crossings.info().getSize(1);
    
    for ( int idx=0; idx<xsize; idx++)
    {
	for ( int idy=0; idy<ysize; idy++ )
	{
	    const float z0 = mFieldZ( idx, idy );
	    for ( int hor=0; hor<=1; hor++ )
	    {
		crossings.set( idx, idy, hor, mUdf(float) );

		if  ( (hor && idx==xsize-1) || (!hor && idy==ysize-1) )
		    continue;

		const float z1 = mFieldZ( idx+hor, idy+1-hor );

		if ( mIsUdf(z0) || mIsUdf(z1) )
		    continue;
		    
		if ( (z0<z && z<=z1) || (z1<z && z<=z0) )
		{
		    const float frac = (z-z0) / (z1-z0);

		    if ( polyroi_ )
		    {
			mMakeVertex( vertex, idx, idy, hor, frac );
			if ( !polyroi_->isInside(vertex, true, mDefEps) )
			    continue;
		    }

		    crossings.set( idx, idy, hor, frac );
		}
	    }
	}
    }
}


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


void IsoContourTracer::traceContours( Array3DImpl<float>& crossings,
				      ObjectSet<ODPolygon<float> >& contours,
				      bool closedonly ) const
{
    for ( int pidx=0; pidx<crossings.info().getSize(0); pidx++ )  /* [p]ivot */
    {
	for ( int pidy=0; pidy<crossings.info().getSize(1); pidy++ )
	{
	    for ( int phor=0; phor<=1; phor++ )
	    {
		const float pfrac = crossings.get( pidx, pidy, phor );
		if ( mIsUdf(pfrac) )
		    continue;

		ODPolygon<float>* contour = new ODPolygon<float>();
		contour->setClosed( false );
		
		for ( int pup=0; pup<=1; pup++ )
		{
		    int idx = pidx; int idy = pidy; int hor = phor; 
		    int up = pup; float frac = pfrac;
		    
		    if ( !pup && !nextCrossing(crossings,idx,idy,hor,up,frac) )
			continue;

		    do
		    {
			addVertex( *contour, pup, idx, idy, hor, frac );
			crossings.set( idx, idy, hor, mUdf(float) );
		    }
		    while( nextCrossing(crossings,idx,idy,hor,up,frac) );
			
		    if ( !pup && idx==pidx && idy==pidy && hor==phor )
		    {
			contour->setClosed( true );
			break;
		    }
		}

		if ( !mIsUdf(bendpointeps_) )
		    contour->keepBendPoints( bendpointeps_ );

		const int sz = contour->size();
		if ( sz<minnrvertices_ || (closedonly && !contour->isClosed()) )
		    delete contour;
		else
		{
		    const int oldnrcontours = contours.size();

		    int idx = 0;
		    while ( idx<oldnrcontours && contours[idx]->size()>=sz )
			idx++;
		    contours.insertAt( contour, idx );

		    if ( oldnrcontours == nrlargestonly_ )
			delete contours.removeSingle( oldnrcontours );
		}
	    }
	}
    }
}


void IsoContourTracer::addVertex( ODPolygon<float>& contour, bool headinsert,
				  int idx, int idy, int hor, float frac ) const
{
    mMakeVertex( vertex, idx, idy, hor, frac );

    const int prev = headinsert ? 0 : contour.size()-1;
    if ( contour.size() && vertex==contour.getVertex(prev) )
	return;

    contour.insert( (headinsert ? 0 : prev+1), vertex );
}
