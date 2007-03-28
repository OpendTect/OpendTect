#ifndef draw_h
#define draw_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          26/07/2000
 RCS:           $Id: draw.h,v 1.12 2007-03-28 12:18:57 cvsbert Exp $
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

#define mAlign(h,v) Alignment(Alignment::h,Alignment::v)


class MarkerStyle2D
{
public:

    enum Type		{ None, Square, Circle, Cross };
			DeclareEnumUtils(Type)

			MarkerStyle2D( Type tp=Square, int sz=2,
				       Color col=Color::Black,
				       const char* fk=0 )
			: type(tp), size(sz), color(col), fontkey(fk)	{}

    bool		operator==(const MarkerStyle2D& a) const
			{ return a.type==type && a.size==size &&
			         a.color==color && a.fontkey==a.fontkey; }

    Type		type;
    int			size;
    Color		color;
    BufferString	fontkey;

    inline bool		isVisible() const
			{ return type!=None && size>0 && color.isVisible(); }

    void		toString(BufferString&) const;
    void		fromString(const char*);

};


class MarkerStyle3D
{
public:

    enum Type		{ Cube, Cone, Cylinder, Sphere, Arrow, Cross };
			DeclareEnumUtils(Type)

			MarkerStyle3D( Type tp=Cube, int sz=3,
				       Color col=Color::White )
			: type(tp), size(sz), color(col)	{}

    Type		type;
    int			size;
    Color		color;

    inline bool		isVisible() const
			{ return size>0 && color.isVisible(); }

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

    inline bool		isVisible() const
			{ return type!=None && width>0 && color.isVisible(); }

    void		toString(BufferString&) const;
    void		fromString(const char*);

};


#endif
