#ifndef polygon_h
#define polygon_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		Dec 2006
 RCS:		$Id: polygon.h,v 1.31 2012/01/11 23:25:38 cvsnanne Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "sets.h"
#include "iopar.h"
#include "bufstring.h"

#include <math.h>

/*!\brief (Closed) sequence of connected 2-D coordinates */

template <class T>
class ODPolygon
{
public:
		ODPolygon()
		    : closed_(true), udf_(Geom::Point2D<T>::udf())
		    , xrg_(mUdf(T),mUdf(T)), yrg_(mUdf(T),mUdf(T))	{} 
		
		ODPolygon(const TypeSet<Geom::Point2D<T> >& plg)
		    : poly_(plg), closed_(true)
		    , udf_(Geom::Point2D<T>::udf())
		    , xrg_(mUdf(T),mUdf(T)), yrg_(mUdf(T),mUdf(T))	{}

    void	erase();
    bool	isEmpty() const			{ return poly_.isEmpty(); }
		
    int 	size() const			{ return poly_.size(); }
    bool 	validIdx(int idx) const		{ return poly_.validIdx(idx); }
    void	setEmpty()			{ erase(); }
    
    void	add(const Geom::Point2D<T>& vtx);
    void	remove(int idx);
    void	insert(int idx,const Geom::Point2D<T>& vtx);

    bool	isInside(const Geom::Point2D<T>&,
	    		 bool inclborder,T eps) const;
    
    int		isInside(const ODPolygon& testpoly,T eps=0) const;
    		/*  0: testpoly fully outside (borders don't touch)
		    2: testpoly fully  inside (borders don't touch)
		    1: all intermediate cases */

    bool	segmentMeetsBorder(const Geom::Point2D<T>& pt1,
				   const Geom::Point2D<T>& pt2,T eps) const;
    bool	windowOverlaps(const Interval<T>& xrange,
			       const Interval<T>& yrange,T eps) const;

    				// defined for closed polygon
    const Geom::Point2D<T>&	getVertex(int idx) const;
    const Geom::Point2D<T>&	nextVertex(int idx) const; 
    const Geom::Point2D<T>&	prevVertex(int idx) const;

    void	setClosed( bool yn )			{ closed_ = yn; }
    bool	isClosed() const			{ return closed_; }
    void	setUdf( Geom::Point2D<T> pt )		{ udf_ = pt; }
    Geom::Point2D<T> getUdf() const			{ return udf_; }
    const TypeSet<Geom::Point2D<T> >& data() const	{ return poly_; }

    Interval<T>	getRange(bool of_x) const;
    void	getData(bool of_x,TypeSet<T>&) const;

    void	removeZeroLengths();
    bool	isUTurn(int idx) const;
    bool	isSelfIntersecting() const;

    void	convexHull();
    
    		// not for self-intersecting polygons
    float	area() const			{ return fabs(sgnArea()); } 	
    bool	clockwise() const		{ return sgnArea()<0; } 
    bool	anticlockwise() const		{ return sgnArea()>0; } 

    void	reverse();

    double	distTo(const Geom::Point2D<T>& refpt,int* segmentidxptr=0,
		       double* fractionptr=0) const;

    double	maxDistToBorderEstimate(double maxrelerr=0.001) const;

    bool	operator==(const ODPolygon<T>&) const;

protected:
 
    static int doSegmentsMeet( const Geom::Point2D<T>& p1,
			       const Geom::Point2D<T>& p2,
			       const Geom::Point2D<T>& q1,
			       const Geom::Point2D<T>& q2,
			       T eps );

    static bool	isOnSegment( const Geom::Point2D<T>& pt,
	    		     const Geom::Point2D<T>& pt0,
	    		     const Geom::Point2D<T>& pt1,
	   		     T eps );
    static bool isOnHalfLine( const Geom::Point2D<T>& point,
			      const Geom::Point2D<T>& dirvec,
			      const Geom::Point2D<T>& endvec,
			      T eps );
    static bool isEdgeCrossing( const Geom::Point2D<T>& raydir,
	    			const Geom::Point2D<T>& raysrc,
				const Geom::Point2D<T>& vtx1,
				const Geom::Point2D<T>& vtx2 );

    static bool isOnLine( const Geom::Point2D<T>& point,
			  const Geom::Point2D<T>& dirvec,
			  const Geom::Point2D<T>& posvec,
			  T eps );

    static bool isRightOfLine( const Geom::Point2D<T>& point,
			       const Geom::Point2D<T>& dirvec,
			       const Geom::Point2D<T>& posvec );

    static bool doCoincide( const Geom::Point2D<T>& point1,
			    const Geom::Point2D<T>& point2,
			    T eps=mDefEps );

    static double sgnDistToLine( const Geom::Point2D<T>& point,
				 const Geom::Point2D<T>& dirvec,
				 const Geom::Point2D<T>& posvec );

    float sgnArea() const;

    static double distToSegment( const Geom::Point2D<T>& p1,
	    			 const Geom::Point2D<T>& p2,
				 const Geom::Point2D<T>& refpt,
				 double* fractionptr=0 );

    TypeSet<Geom::Point2D<T> >	poly_;
    bool			closed_;
    Geom::Point2D<T>		udf_;

    mutable Interval<T>		xrg_;
    mutable Interval<T>		yrg_;
};


template <class T> inline
bool ODPolygon<T>::operator==( const ODPolygon<T>& poly ) const
{
    if ( data() != poly.data() ) return false;
    return true;
}


template <class T> inline
void ODPolygon<T>::getData( bool forx, TypeSet<T>& ts ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const Geom::Point2D<T>& vtx = poly_[idx];
	ts += forx ? vtx.x : vtx.y;
    }
}


template <class T> inline
void fillPar( IOPar& iop, const ODPolygon<T>& poly, const char* inpkey )
{
    const BufferString keywd( inpkey ); const char* key = keywd.buf();
    iop.setYN( IOPar::compKey(key,"Closed"), poly.isClosed() );
    iop.set( IOPar::compKey(key,"Undef"), poly.getUdf().x, poly.getUdf().y );
    TypeSet<T> ts; poly.getData( true, ts );
    iop.set( IOPar::compKey(key,"Data.X"), ts );
    ts.erase(); poly.getData( false, ts );
    iop.set( IOPar::compKey(key,"Data.Y"), ts );
}


template <class T> inline
void usePar( const IOPar& iop, ODPolygon<T>& poly, const char* inpkey )
{
    const BufferString keywd( inpkey ); const char* key = keywd.buf();
    bool yn = false; iop.getYN( IOPar::compKey(key,"Closed"), yn );
    poly.setClosed( yn );
    Geom::Point2D<T> pt; iop.get( IOPar::compKey(key,"Undef"), pt.x, pt.y );
    poly.setUdf( pt );

    if (   !iop.find( IOPar::compKey(key,"Data.X") )
	|| !iop.find( IOPar::compKey(key,"Data.Y") ) )
	return;

    poly.setEmpty(); TypeSet<T> tsx, tsy;
    iop.get( IOPar::compKey(key,"Data.X"), tsx );
    iop.get( IOPar::compKey(key,"Data.Y"), tsy );
    const int sz = tsx.size() > tsy.size() ? tsy.size() : tsx.size();
    for ( int idx=0; idx<sz; idx++ )
        poly.add( Geom::Point2D<T>(tsx[idx],tsy[idx]) );
}


template <class T> inline
void ODPolygon<T>::insert( int idx, const Geom::Point2D<T>& vtx )
{
    if ( idx>=0 && idx<=size() )
	poly_.insert( idx, vtx );
    xrg_.set( mUdf(T), mUdf(T) );
    yrg_.set( mUdf(T), mUdf(T) );
}


template <class T> inline
void ODPolygon<T>::erase()
{
    poly_.erase();
    xrg_.set( mUdf(T), mUdf(T) );
    yrg_.set( mUdf(T), mUdf(T) );
}


template <class T> inline
void ODPolygon<T>::add( const Geom::Point2D<T>& vtx )
{
    poly_+=vtx;
    xrg_.set( mUdf(T), mUdf(T) );
    yrg_.set( mUdf(T), mUdf(T) );
}


template <class T> inline
void ODPolygon<T>::remove( int idx )
{
    if ( poly_.validIdx(idx) )
	poly_.remove( idx );
    xrg_.set( mUdf(T), mUdf(T) );
    yrg_.set( mUdf(T), mUdf(T) );
}


template <class T> inline
const Geom::Point2D<T>& ODPolygon<T>::getVertex( int idx ) const
{ return poly_.validIdx(idx) ? poly_[idx] : udf_; }


template <class T> inline
const Geom::Point2D<T>& ODPolygon<T>::nextVertex( int idx ) const
{ return getVertex( idx+1<size() ? idx+1 : 0 ); }


template <class T> inline
const Geom::Point2D<T>& ODPolygon<T>::prevVertex( int idx ) const
{ return getVertex( idx ? idx-1 : size()-1 ); }


template <class T> inline
Interval<T> ODPolygon<T>::getRange( bool forx ) const
{
    if ( poly_.isEmpty() ) return Interval<T>( udf_.x, udf_.y );
    Geom::Point2D<T> vtx0 = poly_[0];
    Interval<T> ret = forx ? xrg_ : yrg_;
    if ( !mIsUdf(ret.start) && !mIsUdf(ret.stop) )
	return ret;
    ret.start = ret.stop = forx ? vtx0.x : vtx0.y;
    for ( int idx=1; idx<size(); idx++ )
    {
	const Geom::Point2D<T>& vtx = poly_[idx];
	const T val = forx ? vtx.x : vtx.y;
	if ( val < ret.start )		ret.start = val;
	else if ( val > ret.stop )	ret.stop = val;
    }
    if ( forx )
	xrg_ = ret;
    else
	yrg_ = ret;
    return ret;
}


template <class T> inline
bool ODPolygon<T>::isInside( const Geom::Point2D<T>& point,
			     bool inclborder, T eps ) const
{
    const T abseps = eps<0 ? -eps : eps;
    if ( (!mIsUdf(xrg_.start) && !mIsUdf(xrg_.stop) &&
	  (xrg_.start>point.x+abseps || xrg_.stop<point.x-abseps)) ||
	 (!mIsUdf(yrg_.start) && !mIsUdf(yrg_.stop) &&
	  (yrg_.start>point.y+abseps || yrg_.stop<point.y-abseps)) )
	return false;

    const Geom::Point2D<T> arbitrarydir( 1, 0 );

    bool nrcrossingsodd = false;
    for ( int idx=0; idx<size(); idx++ )
    {
	const Geom::Point2D<T>& vtxcurr = poly_[idx];
	const Geom::Point2D<T>& vtxnext = nextVertex( idx );

	if ( isOnSegment(point, vtxcurr, vtxnext, eps) )
	    return inclborder;
	if ( isEdgeCrossing(arbitrarydir, point, vtxcurr, vtxnext) )
	    nrcrossingsodd = !nrcrossingsodd;
    }

    return nrcrossingsodd;
}


template <class T> inline
bool ODPolygon<T>::segmentMeetsBorder( const Geom::Point2D<T>& pt1,
				       const Geom::Point2D<T>& pt2,
				       T eps ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const Geom::Point2D<T>& vtxcurr = poly_[idx];
	const Geom::Point2D<T>& vtxnext = nextVertex( idx );

	if ( doSegmentsMeet(pt1, pt2, vtxcurr, vtxnext, eps) )
	    return true;
    }

    return false;
}


template <class T> inline
bool ODPolygon<T>::windowOverlaps( const Interval<T>& xrange,
				   const Interval<T>& yrange,
				   T eps ) const
{
    ODPolygon window;
    window.add( Geom::Point2D<T>(xrange.start, yrange.start) );
    window.add( Geom::Point2D<T>(xrange.stop, yrange.start) );
    window.add( Geom::Point2D<T>(xrange.stop,  yrange.stop) );
    window.add( Geom::Point2D<T>( xrange.start, yrange.stop) );

    return isInside( window, eps );
}


template <class T> inline
int ODPolygon<T>::isInside( const ODPolygon& testpoly, T eps ) const
{
    if ( isEmpty() || testpoly.isEmpty() )
	return 0;

    for ( int idx=0; idx<size(); idx++ )
    {
	const Geom::Point2D<T>& vtxcurr = poly_[idx];
	const Geom::Point2D<T>& vtxnext = nextVertex( idx );

	if ( testpoly.segmentMeetsBorder(vtxcurr, vtxnext, eps) )
	    return 1;
    }

    if ( isInside(testpoly.poly_[0], false, eps) )
	return 2;

    return testpoly.isInside(poly_[0], false, eps) ? 1 : 0; 
}


template <class T> inline
void ODPolygon<T>::removeZeroLengths()
{
    const int startidx = isClosed() ? size()-1 : size()-2;
    for ( int idx=startidx; idx>=0; idx-- )
    {
	if ( poly_[idx]==nextVertex(idx) && size()>1 )
	    remove(idx);
    }
}
	

template <class T> inline
bool ODPolygon<T>::isUTurn( int idx ) const
{
    if ( !validIdx(idx) || ( !isClosed() && (idx==0 || idx==size()-1) ) )
	return false;

    const Geom::Point2D<T>& vec1 = prevVertex(idx) - poly_[idx];
    const Geom::Point2D<T>& vec2 = nextVertex(idx) - poly_[idx];

    return vec1.x*vec2.y-vec1.y*vec2.x==0 && vec1.x*vec2.x+vec1.y*vec2.y>0;
}


template <class T> inline
bool ODPolygon<T>::isSelfIntersecting() const
{
    ODPolygon<T> plg = *this;
    plg.removeZeroLengths();

    const int stopidx = plg.isClosed() ? plg.size() : plg.size()-1;
    for ( int idx=0; idx<stopidx; idx++ )
    {
	if ( plg.isUTurn(idx) )
	    return true;

	const Geom::Point2D<T>& vtxcurr = plg.poly_[idx];
	const Geom::Point2D<T>& vtxnext = plg.nextVertex( idx );

	for ( int idy=0; idy<stopidx; idy++ )
	{
	    const int dif = abs( idx-idy );
	    if ( dif<=1 || dif>=plg.size()-1 )
		continue;

	    const Geom::Point2D<T>& pt1 = plg.poly_[idy];
	    const Geom::Point2D<T>& pt2 = plg.nextVertex( idy );

	    if ( vtxcurr==pt1 || vtxcurr==pt2 )
		return true;

	    if ( isEdgeCrossing(vtxnext-vtxcurr, vtxcurr, pt1, pt2) &&
		 isEdgeCrossing(vtxcurr-vtxnext, vtxnext, pt1, pt2) )
		return true;
	}
    }
    return false;
}


template <class T> inline
int ODPolygon<T>::doSegmentsMeet( const Geom::Point2D<T>& p1,
				  const Geom::Point2D<T>& p2,
				  const Geom::Point2D<T>& q1,
				  const Geom::Point2D<T>& q2,
				  T eps )
{
    if ( isOnSegment(p1, q1, q2, eps) || isOnSegment(p2, q1, q2, eps) ||
	 isOnSegment(q1, p1, p2, eps) || isOnSegment(q2, p1, p2, eps) )
	return 1;

    if ( p1==p2 || q1==q2 || !isEdgeCrossing(p2-p1, p1, q1, q2) ||
	 !isEdgeCrossing(p1-p2, p2, q1, q2) )
	return 0;

    return 2;
}


template <class T> inline
bool ODPolygon<T>::isOnSegment( const Geom::Point2D<T>& pt,
				const Geom::Point2D<T>& pt0,
				const Geom::Point2D<T>& pt1,
				T eps )
{
    return isOnHalfLine( pt, pt1-pt0, pt0, eps ) &&
	   isOnHalfLine( pt, pt0-pt1, pt1, eps );
}


template <class T> inline
bool ODPolygon<T>::isOnHalfLine( const Geom::Point2D<T>& point,
				 const Geom::Point2D<T>& dirvec,
				 const Geom::Point2D<T>& endvec,
				 T eps )
{
     if ( doCoincide(point, endvec, eps) )
	 return true;
     if ( !isOnLine(point, dirvec, endvec, eps) )
	 return false;
     const Geom::Point2D<T> rot90dirvec( -dirvec.y, dirvec.x );
     return isRightOfLine( point, rot90dirvec, endvec );
}


template <class T> inline
bool ODPolygon<T>::isEdgeCrossing( const Geom::Point2D<T>& raydir,
				   const Geom::Point2D<T>& raysrc,
				   const Geom::Point2D<T>& vtx1,
				   const Geom::Point2D<T>& vtx2 )
{
    const bool vtx1right = isRightOfLine( vtx1, raydir, raysrc );
    const bool vtx2right = isRightOfLine( vtx2, raydir, raysrc );

    if ( vtx1right && !vtx2right )
	return !isRightOfLine( raysrc, vtx2-vtx1, vtx1 );
    if ( !vtx1right && vtx2right )
	return !isRightOfLine( raysrc, vtx1-vtx2, vtx2 );
    return false;
}


template <class T> inline
bool ODPolygon<T>::isOnLine( const Geom::Point2D<T>& point,
			     const Geom::Point2D<T>& dirvec,
			     const Geom::Point2D<T>& posvec,
			     T eps )
{
    const double signeddist = sgnDistToLine( point, dirvec, posvec );
    return signeddist * signeddist <= eps * eps;
}

template <class T> inline
bool ODPolygon<T>::isRightOfLine( const Geom::Point2D<T>& point,
				  const Geom::Point2D<T>& dirvec,
				  const Geom::Point2D<T>& posvec )
{
    return sgnDistToLine( point, dirvec, posvec ) > 0;
}


template <class T> inline
bool ODPolygon<T>::doCoincide( const Geom::Point2D<T>& point1,
			       const Geom::Point2D<T>& point2,
			       T eps )
{
    return point1.sqDistTo( point2 ) <= eps * eps;
}


template <class T> inline
double ODPolygon<T>::sgnDistToLine( const Geom::Point2D<T>& point,
				    const Geom::Point2D<T>& dirvec,
				    const Geom::Point2D<T>& posvec )
{
    const double nolinedist = 0;

    const double dirveclen = dirvec.distTo( Geom::Point2D<T>(0,0) );
    if ( mIsZero(dirveclen, mDefEps) )
	return nolinedist;
    const double substpointinlineeqn =
	dirvec.y * ( point.x - posvec.x )-dirvec.x * ( point.y - posvec.y );
    return substpointinlineeqn / dirveclen;
}


template <class T> inline
float ODPolygon<T>::sgnArea() const
{
    float area2 = 0.0;

    const Geom::Point2D<T>& pt0 = poly_[0];
    for ( int idx=1; idx<size()-1; idx++ )
    {
	const Geom::Point2D<T>& pt1 = poly_[idx];
	const Geom::Point2D<T>& pt2 = nextVertex( idx );
	area2 += (pt1.x-pt0.x) * (pt2.y-pt0.y) - (pt2.x-pt0.x) * (pt1.y-pt0.y);
    }

    return 0.5*area2;
}


template <class T> inline
void ODPolygon<T>::convexHull()
{
    // Code based on the Graham scan algorithm (1972)

    setClosed( true );
    if ( size()<2 )
	return;

    // Find guaranteed vertex of the convex hull to become pivot
    Geom::Point2D<T> pivot = poly_[0];
    for ( int idx=1; idx<size(); idx++ )
    {
	const Geom::Point2D<T>& vtx = poly_[idx];
	if ( vtx.x<pivot.x || (vtx.x==pivot.x && vtx.y<pivot.y) )
	    pivot = vtx;
    }	

    // Remove all pivot copies
    for ( int idx=size()-1; idx>=0; idx-- )
    {
	if ( pivot == poly_[idx] )
	    poly_.remove( idx, false );
    }

    // Angular sort of all pivot-to-point segments
    for ( int idx=size()-2; idx>=0; idx-- )
    {
	const Geom::Point2D<T>& vtx = poly_[idx];
	for ( int idy=size()-1; idy>idx; idy-- )
	{
	    const Geom::Point2D<T>& vty = poly_[idy];
	    const double dist = sgnDistToLine( vty, vtx-pivot, pivot );

	    if ( dist > 0 )
		continue;

	    if ( dist < 0 )
		poly_.insert( idy+1, vtx );
	    else if ( pivot.sqDistTo(vtx) > pivot.sqDistTo(vty) )
		poly_[idy] = vtx;

	    poly_.remove( idx );
	    break;
	}
    }

    // Expand convex hull incrementally by backward removal of inner points
    for ( int idx=size()-3; idx>=0; idx-- )
    {
	const Geom::Point2D<T>& vtx = poly_[idx];
	while ( idx<size()-2 )
	{
	    const Geom::Point2D<T>& vty = poly_[idx+1];
	    const Geom::Point2D<T>& vtz = poly_[idx+2];
	    if ( isRightOfLine(vtz, vty-vtx, vtx) )
		break;

	    poly_.remove( idx+1 );
	}
    }

    poly_ += pivot;
    
    xrg_.set( mUdf(T), mUdf(T) );
    yrg_.set( mUdf(T), mUdf(T) );
}


template <class T> inline
void ODPolygon<T>::reverse()
{
    const int sz = poly_.size();
    for ( int idx=0; idx<sz/2-1; idx++ )
    {
	Geom::Point2D<T> temp = poly_[idx];
	poly_[idx] = poly_[sz-1-idx];
	poly_[sz-1-idx] = temp;
    }
    
    xrg_.set( mUdf(T), mUdf(T) );
    yrg_.set( mUdf(T), mUdf(T) );
}


template <class T> inline
double ODPolygon<T>::distToSegment( const Geom::Point2D<T>& p1,
				    const Geom::Point2D<T>& p2,
				    const Geom::Point2D<T>& refpt,
				    double* fractionptr )
{
    double frac = 0;

    if ( p1 != p2 )
    {
	const Geom::Point2D<T> dif = p2 - p1;
	const double numerator = dif.x*(refpt.x-p1.x) + dif.y*(refpt.y-p1.y);
	frac = numerator / (dif.x*dif.x + dif.y*dif.y);

	if ( frac < 0 )
	    frac = 0;
	if ( frac > 1 )
	    frac = 1;
    }

    if ( fractionptr )
	*fractionptr = frac;

    const Geom::Point2D<T> pointonseg( (T)(p1.x * (1-frac) + p2.x * frac),
	    			       (T)(p1.y * (1-frac) + p2.y * frac) );
    return refpt.distTo( pointonseg );
}


template <class T> inline
double ODPolygon<T>::distTo( const Geom::Point2D<T>& refpt,
			     int* segmentidxptr, double* fractionptr ) const
{
    const int sz = size();
    if ( !sz )
	return mUdf(double);

    double mindist = MAXDOUBLE;
    int mindistidx;
    double mindistfrac;

    for ( int idx=(isClosed() ? sz-1 : sz-2); idx>=0; idx-- )
    {
	const Geom::Point2D<T>& pt1 = getVertex( idx );
	const Geom::Point2D<T>& pt2 = nextVertex( idx );
	double frac;
	const double dist = distToSegment( pt1, pt2, refpt, &frac );

	if ( mindist >= dist )
	{
	    mindist = dist;
	    if ( segmentidxptr )
		*segmentidxptr = idx;
	    if ( fractionptr )
		*fractionptr = frac;
	}
    }

    return mindist;
}


template <class T> inline
double ODPolygon<T>::maxDistToBorderEstimate( double maxrelerr ) const
{
    if ( maxrelerr <= 0.0 )
	return mUdf(double);

    if ( size() < 3 )
	return isEmpty() ? mUdf(double) : 0.0;

    const double upperbound = mMIN( 0.5 * getRange(true).width(),
				    0.5 * getRange(false).width() );
    if ( !upperbound )
	return 0.0;

    ODPolygon<double> poly;
    for ( int idx=0; idx<size(); idx++ )
    {
	const Geom::Point2D<T>& pt = getVertex(idx);
	poly.add( Geom::Point2D<double>(pt.x, pt.y) );
    }
	
    double maxdist = 0.0;
    for ( int idx=0; idx<poly.size(); idx++ )
    {
	Geom::Point2D<double> curpt = 
	    (poly.prevVertex(idx)+poly.getVertex(idx)+poly.nextVertex(idx)) / 3;

	if ( !poly.isInside(curpt, false, mDefEps) )
	    continue;

	double curdist = poly.distTo( curpt );
	double gamma = 0.1 * upperbound;

	for ( int step=0; step<100; step++ ) 
	{
	    if ( curdist > maxdist )
		maxdist = curdist;

	    // Gradient ascent
	    Geom::Point2D<double> pt1 = curpt;
	    Geom::Point2D<double> pt2 = curpt;
	    const double delta = 0.0001 * upperbound;
	    pt1.x += delta;
	    pt2.y += delta;
	    const double dist1 = poly.distTo( pt1 );
	    const double dist2 = poly.distTo( pt2 );
	    Geom::Point2D<double> nextpt( dist1-curdist, dist2-curdist );
	    nextpt *= gamma/delta;
	    nextpt += curpt;

	    if ( !poly.isInside(nextpt, false, mDefEps) )
	    {
		gamma *= 0.5;
		continue;
	    }

	    double nextdist = poly.distTo( nextpt );

	    if ( nextdist <= curdist )
	    {
		gamma *= 0.5;

		if ( curpt.distTo(nextpt) <= 2*maxrelerr*curdist )
		    break;
	    }

	    curpt = nextpt;
	    curdist = nextdist;
	}
    }	
    return maxdist;
}


#endif
