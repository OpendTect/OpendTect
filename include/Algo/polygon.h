#ifndef polygon_h
#define polygon_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	J.C. Glas
 Date:		Dec 2006
 RCS:		$Id: polygon.h,v 1.7 2007-12-04 15:45:06 cvsjaap Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "sets.h"
#include "iopar.h"

/*!\brief (Closed) sequence of connected 2-D coordinates */

template <class T>
class ODPolygon
{
public:
		ODPolygon()
		    : closed_(true), udf_(Geom::Point2D<T>::udf())	{} 
		
		ODPolygon(const TypeSet<Geom::Point2D<T> >& plg)
		    : poly_(plg), closed_(true)
		    , udf_(Geom::Point2D<T>::udf())			{}

    void	erase()				{ poly_.erase(); }
    bool	isEmpty() const			{ return poly_.isEmpty(); }
		
    int 	size() const			{ return poly_.size(); }
    bool 	validIdx(int idx) const		{ return poly_.validIdx(idx); }
    void	setEmpty()			{ poly_.erase(); }
    
    void	add(const Geom::Point2D<T>& vtx)	{ poly_+=vtx; }
    void	remove(int idx);
    void	insert(int idx,const Geom::Point2D<T>& vtx);

    bool	isInside(const Geom::Point2D<T>&,
	    		 bool inclborder,T eps) const;
    
    bool	segmentOverlaps(const Geom::Point2D<T>& pt1,
				const Geom::Point2D<T>& pt2,T eps) const;
    bool	windowOverlaps(const Interval<T>& xrange,
			       const Interval<T>& yrange,T eps) const;

    const Geom::Point2D<T>&	getVertex(int idx) const;
    const Geom::Point2D<T>&	nextVertex(int idx) const; 

    void	setClosed( bool yn )			{ closed_ = yn; }
    bool	isClosed() const			{ return closed_; }
    void	setUdf( Geom::Point2D<T> pt )		{ udf_ = pt; }
    Geom::Point2D<T> getUdf() const			{ return udf_; }
    const TypeSet<Geom::Point2D<T> >& data() const	{ return poly_; }

    Interval<T>	getRange(bool of_x) const;
    void	getData(bool of_x,TypeSet<T>&) const;

    		// not for self-intersecting polygons
    float	area() const			{ return fabs(sgnArea()); } 	
    bool	clockwise() const		{ return sgnArea()<0; } 
    bool	anticlockwise() const		{ return sgnArea()>0; } 

protected:

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
			    T eps );

    static double sgnDistToLine( const Geom::Point2D<T>& point,
				 const Geom::Point2D<T>& dirvec,
				 const Geom::Point2D<T>& posvec );

    float sgnArea() const;

    TypeSet<Geom::Point2D<T> >	poly_;
    bool			closed_;
    Geom::Point2D<T>		udf_;

};


template <class T> inline
void ODPolygon<T>::getData( bool forx, TypeSet<T>& ts ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const Geom::Point2D<T>& vtx = getVertex( idx );
	ts += forx ? vtx.x : vtx.y;
    }
}


template <class T> inline
void fillPar( IOPar& iop, const ODPolygon<T>& poly, const char* key )
{
    iop.setYN( IOPar::compKey(key,"Closed"), poly.isClosed() );
    iop.set( IOPar::compKey(key,"Undef"), poly.getUdf().x, poly.getUdf().y );
    TypeSet<T> ts; poly.getData( true, ts );
    iop.set( IOPar::compKey(key,"Data.X"), ts );
    ts.erase(); poly.getData( false, ts );
    iop.set( IOPar::compKey(key,"Data.Y"), ts );
}


template <class T> inline
void usePar( const IOPar& iop, ODPolygon<T>& poly, const char* key )
{
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
{ if ( idx>=0 && idx<=size() ) poly_.insert( idx, vtx ); }

template <class T> inline
void ODPolygon<T>::remove( int idx )
{ if ( poly_.validIdx(idx) ) poly_.remove( idx ); }

template <class T> inline
const Geom::Point2D<T>& ODPolygon<T>::getVertex( int idx ) const
{ return poly_.validIdx(idx) ? poly_[idx] : udf_; }


template <class T> inline
const Geom::Point2D<T>& ODPolygon<T>::nextVertex( int idx ) const
{ return getVertex( idx+1<size() ? idx+1 : 0 ); }


template <class T> inline
Interval<T> ODPolygon<T>::getRange( bool forx ) const
{
    if ( poly_.isEmpty() ) return Interval<T>( udf_.x, udf_.y );
    Geom::Point2D<T> vtx0 = getVertex( 0 );
    Interval<float> ret( vtx0.x, vtx0.x );
    for ( int idx=1; idx<size(); idx++ )
    {
	const Geom::Point2D<T>& vtx = getVertex( idx );
	const T val = forx ? vtx.x : vtx.y;
	if ( val < ret.start )		ret.start = val;
	else if ( val > ret.stop )	ret.stop = val;
    }
    return ret;
}


template <class T> inline
bool ODPolygon<T>::isInside( const Geom::Point2D<T>& point,
			     bool inclborder, T eps ) const
{
    const Geom::Point2D<T> arbitrarydir( 1, 0 );

    bool nrcrossingsodd = false;
    for ( int idx=0; idx<size(); idx++ )
    {
	const Geom::Point2D<T>& vtxcurr = getVertex( idx );
	const Geom::Point2D<T>& vtxnext = nextVertex( idx );

	if ( isOnSegment(point, vtxcurr, vtxnext, eps) )
	    return inclborder;
	if ( isEdgeCrossing(arbitrarydir, point, vtxcurr, vtxnext) )
	    nrcrossingsodd = !nrcrossingsodd;
    }

    return nrcrossingsodd;
}


template <class T> inline
bool ODPolygon<T>::segmentOverlaps( const Geom::Point2D<T>& pt1,
				    const Geom::Point2D<T>& pt2,
				    T eps ) const
{
    if ( pt1 != pt2 )
    {
	for ( int idx=0; idx<size(); idx++ )
	{
	    const Geom::Point2D<T>& vtxcurr = getVertex( idx );
	    const Geom::Point2D<T>& vtxnext = nextVertex( idx );

	    if ( isOnSegment(vtxcurr, pt1, pt2, eps) || 
		 isOnSegment(vtxnext, pt1, pt2, eps) )
		return true;

	    if ( isEdgeCrossing(pt2-pt1, pt1, vtxcurr, vtxnext) &&
		 isEdgeCrossing(pt1-pt2, pt2, vtxcurr, vtxnext) )
		return true;
	}
    }
	
    return isInside( pt1, true, eps );
}


template <class T> inline
bool ODPolygon<T>::windowOverlaps( const Interval<T>& xrange,
				   const Interval<T>& yrange,
				   T eps ) const
{
    const Geom::Point2D<T> pt1( xrange.start, yrange.start );
    const Geom::Point2D<T> pt2( xrange.stop,  yrange.start );
    const Geom::Point2D<T> pt3( xrange.stop,  yrange.stop );
    const Geom::Point2D<T> pt4( xrange.start, yrange.stop );

    if ( segmentOverlaps(pt1, pt2, eps) || segmentOverlaps(pt2, pt3, eps) ||
	 segmentOverlaps(pt3, pt4, eps) || segmentOverlaps(pt4, pt1, eps) )
	return true;
    
    const Geom::Point2D<T>& arbivtx = getVertex( 0 );

    return xrange.includes(arbivtx.x) && yrange.includes(arbivtx.y);
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
	return isRightOfLine( raysrc, vtx2-vtx1, vtx1 );
    if ( !vtx1right && vtx2right )
	return isRightOfLine( raysrc, vtx1-vtx2, vtx2 );
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
			       T eps=mDefEps )
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

    for ( int idx=1; idx<size()-1; idx++ )
    {
	const Geom::Point2D<T>& pt0 = getVertex( 0 );
	const Geom::Point2D<T>& pt1 = getVertex( idx );
	const Geom::Point2D<T>& pt2 = nextVertex( idx );
	area2 += (pt1.x-pt0.x) * (pt2.y-pt0.y) - (pt2.x-pt0.x) * (pt1.y-pt0.y);
    }

    return 0.5*area2;
}


#endif
