#ifndef polygon_h
#define polygon_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	J.C. Glas
 Date:		Dec 2006
 RCS:		$Id: polygon.h,v 1.4 2007-11-12 14:20:08 cvsjaap Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "sets.h"

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
		
    int 	size() const			{ return poly_.size(); }
    bool 	validIdx(int idx) const		{ return poly_.validIdx(idx); }
    
    void	add(const Geom::Point2D<T>& vtx)	{ poly_+=vtx; }
    void	remove(int idx);
    void	insert(int idx,const Geom::Point2D<T>& vtx);

    bool	isInside(const Geom::Point2D<T>&,
	    		 bool inclborder,T eps) const;

    const Geom::Point2D<T>&	getVertex(int idx) const;
    const Geom::Point2D<T>&	nextVertex(int idx) const; 

    void	setClosed(bool yn)			{ closed_=yn; }
    bool	isClosed() const			{ return closed_; }

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

    TypeSet<Geom::Point2D<T> >	poly_;
    bool			closed_;
    Geom::Point2D<T>		udf_;
};


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

#endif
