#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geometry.h"
#include <math.h>

/*!
\brief Clips a line between two points by a rectangle. The line may be
completely outside, completely inside or partially inside. If partially
inside, new endpoints are calculated.
*/

template <class T>
mClass(Algo) LineRectangleClipper
{
public:
    inline			LineRectangleClipper(const Geom::Rectangle<T>&);

    inline void			setLine(const Geom::Point2D<T>& start,
	    				const Geom::Point2D<T>& stop);

    inline bool				isIntersecting() const;
    inline bool				isStartChanged() const;
    inline bool				isStopChanged() const;
    inline const Geom::Point2D<T>&	getStart() const;
    inline const Geom::Point2D<T>&	getStop() const;

protected:
    inline const T	castDouble2T(double) const;

    bool		isintersecting_;
    bool		startchanged_;
    bool		stopchanged_;
    Geom::Point2D<T>	start_;
    Geom::Point2D<T>	stop_;

    Geom::Rectangle<T>	rect_;
};


template <class T> inline
void clipPolyLine( const Geom::Rectangle<T>& rect, 
		   const TypeSet<Geom::Point2D<T> >& polyline,
		   ObjectSet<TypeSet<Geom::Point2D<T> > >& result )
{
    deepErase( result );

    LineRectangleClipper<T> clipper( rect );

    TypeSet<Geom::Point2D<T> >* currentline = 0;

    for ( int idx=1; idx<polyline.size(); idx++ )
    {
	clipper.setLine( polyline[idx-1], polyline[idx] );
	if ( !clipper.isIntersecting() )
	{
	    currentline = 0;
	    continue;
	}

	if ( currentline && clipper.isStartChanged() )
	    currentline = 0;

	if ( !currentline )
	{
	    currentline = new TypeSet<Geom::Point2D<T> >;
	    result += currentline;
	    (*currentline) += clipper.getStart();
	}

	(*currentline) += clipper.getStop();
	if ( clipper.isStopChanged() )
	    currentline = 0;
    }
}


template <class T> inline
LineRectangleClipper<T>::LineRectangleClipper( const Geom::Rectangle<T>& r )
    : rect_( r )
    , isintersecting_( true )
    , startchanged_( false )
    , stopchanged_( false )
{
    rect_.sortCorners();
}


template <class T> inline
const T LineRectangleClipper<T>::castDouble2T( double d ) const
{
    const T t1 = (T) d;
    const T t2 = (T) Math::Floor( d + 0.5 );
    return fabs(d-t1) < fabs(d-t2) ? t1 : t2;
}


#define mBoundaryClip( delta, offset ) \
{ \
    if ( delta ) \
    { \
	double tnew = (double) (offset) / (delta); \
	if ( (delta) < 0 ) \
	{ \
	    if ( tnew > tstop )	 return; \
	    if ( tnew > tstart ) tstart = tnew; \
	} \
	else \
	{ \
	    if ( tnew < tstart ) return; \
	    if ( tnew < tstop )	 tstop = tnew; \
	} \
    } \
    else \
	if ( (offset) < 0 ) return; \
}

#define mAdjustPoint( which ) \
{ \
    Geom::Point2D<T> newpoint; \
    newpoint.x_ = start.x_ + castDouble2T( dx * t##which ); \
    newpoint.y_ = start.y_ + castDouble2T( dy * t##which ); \
    which##changed_ = which##_ != newpoint; \
    which##_ = newpoint; \
}

/* LineRectangleClipper applies the Liang&Barsky line-clipping algorithm */
template <class T> inline
void LineRectangleClipper<T>::setLine(const Geom::Point2D<T>& start,
				const Geom::Point2D<T>& stop)
{
    isintersecting_ = false;
    start_ = start; 
    stop_ = stop;
    startchanged_ = false; 
    stopchanged_ = false;

    double tstart = 0.0; 
    double tstop = 1.0;

    const double dx = stop.x_ - start.x_;
    mBoundaryClip( -dx, start.x_ - rect_.left()  );
    mBoundaryClip(  dx, rect_.right() - start.x_ );

    const double dy = stop.y_ - start.y_;
    mBoundaryClip( -dy, start.y_ - rect_.top()   );
    mBoundaryClip(  dy, rect_.bottom() - start.y_ );

    isintersecting_ = true;

    if ( tstart > 0.0 )
	mAdjustPoint( start );

    if ( tstop < 1.0 )
	mAdjustPoint( stop );
}


template <class T> inline
bool LineRectangleClipper<T>::isIntersecting() const
{ return isintersecting_; }


template <class T> inline
bool LineRectangleClipper<T>::isStartChanged() const
{ return startchanged_; }


template <class T> inline
bool LineRectangleClipper<T>::isStopChanged() const
{ return stopchanged_; }


template <class T> inline
const Geom::Point2D<T>& LineRectangleClipper<T>::getStart() const
{ return start_; }


template <class T> inline
const Geom::Point2D<T>& LineRectangleClipper<T>::getStop() const
{ return stop_; }
