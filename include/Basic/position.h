#ifndef position_H
#define position_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		21-6-1996
 Contents:	Positions: Inline/crossline and Coordinate
 RCS:		$Id: position.h,v 1.9 2003-01-23 07:36:16 kristofer Exp $
________________________________________________________________________

-*/

#include <geometry.h>


/*!\brief a cartesian coordinate in 2D space. */

class Coord
{
public:
		Coord()				{ x = y = 0; }
		Coord( double cx, double cy )	{ x = cx; y = cy; }

		Coord( const Point<double>& p )	{ x = p.x(); y = p.y(); }
		operator Point<double>() const { return Point<double>(x,y); }

    void	operator+=( double dist )	{ x += dist; y += dist; }
    void	operator+=( const Coord& crd )	{ x += crd.x; y += crd.y; }
    void	operator-=( const Coord& crd )	{ x -= crd.x; y -= crd.y; }
    Coord	operator+( const Coord& crd ) const
		{ Coord res = *this; res.x+=crd.x; res.y+=crd.y; return res; }
    Coord	operator-( const Coord& crd ) const
		{ Coord res = *this; res.x-=crd.x; res.y-=crd.y; return res; }
    bool	operator==( const Coord& crd ) const
		{ return mIS_ZERO(x-crd.x) && mIS_ZERO(y-crd.y); }
    bool	operator!=( const Coord& crd ) const
		{ return ! (crd == *this); }
    double	distance(const Coord&) const;
    void	fill(char*) const;
    bool	use(const char*);

    double	x;
    double	y;
};


/*!\brief a cartesian coordinate in 3D space. */
class Coord3
{
public:
    		Coord3 ( float x_, float y_, float z_ );
		Coord3();

    float&	operator[](int idx);
    float	operator[](int idx) const;

    Coord3&	operator+=( const Coord3& );
    Coord3&	operator-=( const Coord3& );

    bool	operator==( const Coord3& b ) const;
    bool	operator!=(const Coord3& b ) const;
    bool	isDefined() const;

    float	dist( const Coord3& b ) const;

    float	x, y, z;
};



/*!\brief coordinate and a value. */

class CoordValue
{
public:
		CoordValue( double x=0, double y=0, float v=mUndefValue )
		: coord(x,y), value(v)	{}
		CoordValue( const Coord& c, float v=mUndefValue )
		: coord(c), value(v)		{}
    bool	operator==( const CoordValue& cv ) const
		{ return cv.coord == coord; }
    bool	operator!=( const CoordValue& cv ) const
		{ return cv.coord != coord; }

    Coord	coord;
    float	value;
};


/*!\brief positioning in a seismic survey: inline/crossline. */

class BinID
{
public:
		BinID() : inl(0), crl(0)			{}
		BinID( int il, int cl=1 ) : inl(il), crl(cl)	{}

    void	operator+=( const BinID& bi )
			{ inl += bi.inl; crl += bi.crl; }
    bool	operator==( const BinID& bi ) const
			{ return inl == bi.inl && crl == bi.crl; }
    bool	operator!=( const BinID& bi ) const
			{ return ! (bi == *this); }
    void	fill(char*) const;
    bool	use(const char*);


    int		inl;
    int		crl;

};


/*!\brief BinID and a value. */

class BinIDValue
{
public:
		BinIDValue( int inl=0, int crl=0, float v=mUndefValue )
		: binid(inl,crl), value(v)	{}
		BinIDValue( const BinID& b, float v=mUndefValue )
		: binid(b), value(v)		{}

    bool	operator==( const BinIDValue& biv ) const
		{ return biv.binid == binid; }
    bool	operator!=( const BinIDValue& biv ) const
		{ return biv.binid != binid; }

    BinID	binid;
    float	value;
};


/*!\brief BinID, Z and value. */

class BinIDZValue : public BinIDValue
{
public:
		BinIDZValue( int inl=0, int crl=0, float zz=0,
			     float v=mUndefValue )
		: BinIDValue(inl,crl,v), z(zz)		{}
		BinIDZValue( const BinID& b, float zz=0, float v=mUndefValue )
		: BinIDValue(b,v), z(zz)		{}

    float	z;
};


class CoordTransform
{
public:

    virtual		~CoordTransform()	{}

    virtual Coord	getTransXY(const Coord&)= 0;
    virtual Coord	getTrueXY(const Coord&)	= 0;

};


#endif
