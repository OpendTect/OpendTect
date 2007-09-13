#ifndef rcol_h
#define rcol_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		12-8-1997
 RCS:		$Id: rcol.h,v 1.12 2007-09-13 19:38:38 cvsnanne Exp $
________________________________________________________________________

-*/


/*!\brief Object with row and col, which are accesable through r() and c(). */

#include "gendefs.h"
#include "plftypes.h"
#include "undefval.h"

#include <math.h>

template <class T> class TypeSet;

class RCol 
{
public:
    virtual		~RCol() {}
    int&		operator[](int idx) { return idx ? c() : r(); }
    int			operator[](int idx) const { return idx ? c() : r(); }
    virtual int&	r() 				= 0;
    virtual int		r() const			= 0;

    virtual int&	c() 				= 0;
    virtual int		c() const			= 0;

    inline od_int64	getSerialized() const;
    inline void		setSerialized( od_int64 );

    const RCol&		operator+=( const RCol& rc )
			{ r() += rc.r(); c() += rc.c(); return *this; }
    const RCol&		operator-=( const RCol& rc )
			{ r() -= rc.r(); c() -= rc.c(); return *this; }
    const RCol&		operator*=( const RCol& rc )
			{ r() *= rc.r(); c() *= rc.c(); return *this; }
    const RCol&		operator*=( int factor )
			{ r() *= factor; c() *= factor;  return *this; } 
    const RCol&		operator/=( const RCol& rc )
			{ r() /= rc.r(); c() /= rc.c();  return *this; } 
    const RCol&		operator=( const RCol& rc )
			{ r()=rc.r(); c() = rc.c(); return *this; }

    bool		operator==( const RCol& rc ) const
			{ return r()==rc.r() && c()==rc.c(); }
    bool		operator!=( const RCol& rc ) const
			{ return !(*this==rc); }

    bool		isNeighborTo( const RCol&, const RCol& step,
	    			      bool eightconnectivity=true ) const;
    			/*!<\returns true if the object is a neighbor with the 
			   provided RCol. The neighborhood is defined with
			   either eight- or four-connectivity */

    float		angleTo(const RCol&) const;
			/*!<\returns the smallest angle between the vector
			      going from 0,0 to the object and the vector
			      going from 0,0 to rc.*/
    float		clockwiseAngleTo(const RCol& rc) const;
    			/*!<\returns the angle between the vector going from
			     0,0 to the object and the vector going from 0,0
			     to rc in the clockwise direction.*/
    float		counterClockwiseAngleTo(const RCol&) const;
			/*!<\returns the angle between the vector going from
			      0,0 to the object and the vector going from 0,0
			      to rc in the counterclockwise direction.*/

    int			sqDistTo(const RCol& rc) const;
    			/*!<\returns the square of the distance between this
			  	     object and the provided one. The square
				     distance is easier to compute than the
				     real one, so if the distance only should
				     be compared with other distances, the
				     comparison can equally well be done
				     on squared distances. */

    void		fill(char*) const;
    bool		use(const char*);

    template <class T>
    inline static bool 	makeLine( const RCol& start, const RCol& stop,
				  TypeSet<T>& output, const RCol& step );
    			/*!< Makes a line from start to stop and stores it in
			     output. The function will fail if start or stop
			     is not reachable with the given step.
			 */
};


#define mRowColFunctions(clss, row, col) \
clss	operator+( const RCol& rc ) const \
	{ return clss( row+rc.r(), col+rc.c() ); } \
clss	operator-( const RCol& rc ) const \
	{ return clss( row-rc.r(), col-rc.c() ); } \
clss	operator+() const { return clss( +row, +col ); } \
clss	operator-() const { return clss( -row, -col ); } \
clss	operator*( const RCol& rc ) const \
	{ return clss( row*rc.r(), col*rc.c() ); } \
clss	operator*( int factor ) const \
	{ return clss( row*factor, col*factor ); } \
clss	operator/( const RCol& rc ) const \
	{ return clss( row/rc.r(), col/rc.c() ); } \
clss	operator/( int denominator ) const \
	{ return clss( row/denominator, col/denominator ); }


inline od_int64 RCol::getSerialized() const
{
    return (((od_uint64) r() )<<32)+
	    ((od_uint64) c() &  0xFFFFFFFF);
}



inline void RCol::setSerialized( od_int64 serialized )
{
    r() = serialized>>32;
    c() = (od_int32) (serialized & 0xFFFFFFFF);
}


/*!\brief Object that builds a line from start in the direction of dir with
  	  a step. The line is built in an iterative way, so it is possible
	  to check after everystep if the line should continue (e.g. check if
	  it has bumped into something.
*/

template <class T>
class RColLineBuilder
{
public:
    			RColLineBuilder( const RCol& start,
					   const RCol& dir,
					   const RCol& step,
					   TypeSet<T>& line);
   int			nextStep();
   			/*!<\returns 1 if the extension went well, -1 if
			     	       the direction is zero. */

protected:
   float		distToLine( const RCol& rc ) const;
   T			start;
   const T		dir;
   const T		step;
   const float		dirlen;
   TypeSet<T>&	line;
};


template <class T> inline
RColLineBuilder<T>::RColLineBuilder( const RCol& start_,
	const RCol& dir_, const RCol& step_, 
	TypeSet<T>& line_)
   : start( start_ )
   , dir( dir_ )
   , step( step_ )
   , line( line_ )
   , dirlen( sqrt(float(dir_.r()*dir_.r()+dir_.c()*dir_.c())) )
{}


template <class T> inline
int RColLineBuilder<T>::nextStep()
{
    if ( !dir.r() && !dir.c() )
	return -1;

    T bestrc;
    if ( line.size() )
    {
	const T& lastpos = line[line.size()-1];

	float disttoline = mUdf(float);

	if ( dir.r() )
	{
	    const T candidate =
	    lastpos+T(dir.r()>0?step.c():-step.r(), 0 );
	    const float dist = distToLine(candidate);
	    if ( dist<disttoline )
	    { bestrc = candidate; disttoline=dist; }
	}

	if ( dir.c() )
	{
	    const T candidate =
		lastpos+T(0,dir.c()>0?step.c():-step.c() );
	    const float dist = distToLine(candidate);
	    if ( dist<disttoline )
	    { bestrc = candidate; disttoline=dist; }
	}

	if ( dir.r() && dir.c() )
	{
	    const T candidate =
		lastpos+T( dir.r()>0?step.c():-step.r(),
				dir.c()>0?step.c():-step.c() );
	    const float dist = distToLine(candidate);
	    if ( dist<disttoline )
	    { bestrc = candidate; disttoline=dist; }
	}
    }
    else
	bestrc = start;

    line += bestrc;
    return 1;
}


template <class T>
float RColLineBuilder<T>::distToLine( const RCol& rc ) const
{
    return fabs((dir.r()*(rc.c()-start.c())-dir.c()*(rc.r()-start.r()))/dirlen);
}


template <class T> inline
bool RCol::makeLine( const RCol& start, const RCol& stop,
		     TypeSet<T>& output, const RCol& step )
{
    if ( start.r()%step.r()!=stop.r()%step.r() ||
	 start.c()%step.c()!=stop.c()%step.c() )
	return false;

    output.erase();
    if ( start==stop )
    { output += start; return true; }

    T dir = stop;
    dir -= start;

    RColLineBuilder<T> builder( start, dir, step, output );

    while ( builder.nextStep()>0 && output[output.size()-1]!=stop );
    return true;
}



#endif
