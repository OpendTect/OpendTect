#ifndef position_H
#define position_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		21-6-1996
 Contents:	Positions: Inline/crossline and Coordinate
 RCS:		$Id: position.h,v 1.18 2003-10-28 08:07:44 nanne Exp $
________________________________________________________________________

-*/

#include <gendefs.h>


/*!\brief a cartesian coordinate in 2D space. */

class Coord
{
public:
		Coord() : x(0), y(0)		{}
		Coord( double cx, double cy )	
		: x(cx), y(cy)			{}

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
class Coord3 : public Coord
{
public:
		Coord3() : z(0)					{}
		Coord3(const Coord& a, double z_ )
		    : Coord(a), z(z_)				{}
		Coord3(const Coord3& xyz )
		    : Coord( xyz.x, xyz.y )
		    , z( xyz.z )				{}
    		Coord3( double x_, double y_, double z_ )
		    : Coord(x_,y_), z(z_)			{}

    double&	operator[]( int idx )
		{ return idx ? (idx==1 ? y : z) : x; }
    double	operator[]( int idx ) const
		{ return idx ? (idx==1 ? y : z) : x; }

    Coord3	operator+(const Coord3&) const;
    Coord3	operator-(const Coord3&) const;


    Coord3&	operator+=(const Coord3&);
    Coord3&	operator-=(const Coord3&);

    bool	operator==(const Coord3&) const;
    bool	operator!=(const Coord3&) const;
    bool	isDefined() const;

    double	distance( const Coord3& b ) const;


    double	z;

};



/*!\brief 2D coordinate and a value. */

class CoordValue
{
public:
		CoordValue( double x=0, double y=0, float v=mUndefValue )
		: coord(x,y), value(v)		{}
		CoordValue( const Coord& c, float v=mUndefValue )
		: coord(c), value(v)		{}
    bool	operator==( const CoordValue& cv ) const
		{ return cv.coord == coord; }
    bool	operator!=( const CoordValue& cv ) const
		{ return cv.coord != coord; }

    Coord	coord;
    float	value;
};


/*!\brief 3D coordinate and a value. */

class Coord3Value
{
public:
    		Coord3Value( double x=0, double y=0, double z=0, 
			     float v=mUndefValue )
		: coord(x,y,z), value(v) 	{}
		Coord3Value( const Coord3& c, float v=mUndefValue )
		: coord(c), value(v)		{}
    bool	operator==( const Coord3Value& cv ) const
		{ return cv.coord == coord; }
    bool	operator!=( const Coord3Value& cv ) const
		{ return cv.coord != coord; }

    Coord3	coord;
    float	value;
};


/*!\brief positioning in a seismic survey: inline/crossline. */

class BinID
{
public:
		BinID() : inl(0), crl(0)			{}
		BinID( int il, int cl=1 ) : inl(il), crl(cl)	{}

    BinID	operator+( const BinID& bi ) const;
    BinID	operator-( const BinID& bi ) const;
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


#endif
