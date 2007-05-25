#ifndef linerectangleclipper_h
#define linerectangleclipper_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Dec 2006
 RCS:		$Id: linerectangleclipper.h,v 1.1 2007-05-25 15:18:31 cvskris Exp $
________________________________________________________________________

-*/


/*!Clips a line between to points by a rectangle. The line may be completely
   outside, completely inside or partially inside. If partially inside, new
   endpoints are calculated.
*/

template <class T>
class LineRectangleClipper
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
{}


template <class T> inline
void LineRectangleClipper<T>::setLine(const Geom::Point2D<T>& start,
				const Geom::Point2D<T>& stop)
{
    start_ = start;
    stop_ = stop;
    isintersecting_ = true;
    startchanged_ = false;
    stopchanged_ = false;
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


#endif
