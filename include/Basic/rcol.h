#ifndef rcol_h
#define rcol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		12-8-1997
 RCS:		$Id: rcol.h,v 1.19 2010-04-09 07:58:38 cvsbert Exp $
________________________________________________________________________

-*/


/*!\brief Object with row and col, which are accesable through r() and c(). */

#include "gendefs.h"
#include "plftypes.h"
template <class T> class TypeSet;


mClass RCol 
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
    r() = (od_int32) (serialized>>32);
    c() = (od_int32) (serialized & 0xFFFFFFFF);
}


#endif
