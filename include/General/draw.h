#ifndef draw_h
#define draw_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          26/07/2000
 RCS:           $Id: draw.h,v 1.8 2004-04-14 14:04:18 nanne Exp $
________________________________________________________________________

-*/

#include "enums.h"
#include "color.h"


class Alignment
{
public:
    enum Pos		{ Start, Middle, Stop };
			DeclareEnumUtils(Pos)

			Alignment( Pos h=Start, Pos v=Start )
			: hor(h), ver(v)	{}

    Pos			hor;
    Pos			ver;

};


class MarkerStyle2D
{
public:

    enum Type		{ None, Square, Circle, Cross };
			DeclareEnumUtils(Type)

			MarkerStyle2D( Type tp=Square, int sz=2,
				       Color col=Color::Black,
				       const char* fk=0 )
			: type(tp), size(sz), color(col), fontkey(fk)	{}

    Type		type;
    int			size;
    Color		color;
    BufferString	fontkey;

    void		toString(BufferString&) const;
    void		fromString(const char*);

};


class MarkerStyle3D
{
public:

    enum Type		{ None, Cube, Cone, Cylinder, Sphere, Arrow };
			DeclareEnumUtils(Type)

			MarkerStyle3D( Type tp=Cube, int sz=3,
				       Color col=Color::White )
			: type(tp), size(sz), color(col)	{}

    Type		type;
    int			size;
    Color		color;

    void		toString(BufferString&) const;
    void		fromString(const char*);

};


class LineStyle
{
public:

    enum Type		{ None, Solid, Dash, Dot, DashDot, DashDotDot };
			// This enum should not be changed: it is cast
			// directly to a UI enum.
			DeclareEnumUtils(Type)

			LineStyle( Type t=Solid,int w=1,Color c=Color::Black )
			: type(t), width(w), color(c)	{}

    Type		type;
    int			width;
    Color		color;

    void		toString(BufferString&) const;
    void		fromString(const char*);

};


#endif
