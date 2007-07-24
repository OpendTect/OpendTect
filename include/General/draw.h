#ifndef draw_h
#define draw_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          26/07/2000
 RCS:           $Id: draw.h,v 1.18 2007-07-24 17:16:11 cvsbert Exp $
________________________________________________________________________

-*/

#include "enums.h"
#include "color.h"
#include "geometry.h"


class Alignment
{
public:

    enum Pos		{ Start, Middle, Stop };
			DeclareEnumUtils(Pos)

			Alignment( Pos h=Start, Pos v=Start )
			: hor_(h), ver_(v)	{}

    Pos			hor_;
    Pos			ver_;

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
			: type_(tp), size_(sz), color_(col), fontkey_(fk) {}

    bool		operator==(const MarkerStyle2D& a) const
			{ return a.type_==type_ && a.size_==size_ &&
			         a.color_==color_ && a.fontkey_==a.fontkey_; }

    Type		type_;
    int			size_;
    Color		color_;
    BufferString	fontkey_;

    inline bool		isVisible() const
			{ return type_!=None && size_>0 && color_.isVisible(); }

    void		toString(BufferString&) const;
    void		fromString(const char*);

};


class MarkerStyle3D
{
public:

    enum Type		{ None=-1,
			  Cube=0, Cone, Cylinder, Sphere, Arrow, Cross };
			DeclareEnumUtils(Type)

			MarkerStyle3D( Type tp=Cube, int sz=3,
				       Color col=Color::White )
			: type_(tp), size_(sz), color_(col)	{}

    Type		type_;
    int			size_;
    Color		color_;

    inline bool		isVisible() const
			{ return size_>0 && color_.isVisible(); }

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
			: type_(t), width_(w), color_(c)	{}

    bool		operator ==( const LineStyle& ls ) const
			{ return type_ == ls.type_ && width_ == ls.width_
			      && color_ == ls.color_; }
    bool		operator !=( const LineStyle& ls ) const
			{ return !(*this == ls); }

    Type		type_;
    int			width_;
    Color		color_;

    inline bool		isVisible() const
			{ return type_!=None && width_>0 && color_.isVisible();}

    void		toString(BufferString&) const;
    void		fromString(const char*);

};


class ArrowStyle
{
public:

    enum Type		{ HeadOnly, TwoSided, TailOnly, HeadNorTail };
    enum HeadType	{ Line, Triangle, Square, Circle, Cross };
    enum HandedNess	{ TwoHanded, LeftHanded, RightHanded };

			ArrowStyle( int boldness=1, Type t=HeadOnly )
			: type_(t)
			, linestyle_(LineStyle::Solid,boldness)
			, headtype_(Line), tailtype_(Line)
			, fixedheadsz_(true)
			, handedness_(TwoHanded)
					{ setBoldNess(boldness); }

    inline void		setBoldNess( int b )
			{ linestyle_.width_ = b; headsz_ = 3*b; }

    inline bool		hasHead() const
    			{ return headsz_ > 0 && type_ < TailOnly; }
    inline bool		hasTail() const
    			{ return headsz_ > 0
			      && (type_ == TwoSided || type_ == TailOnly); }

    Type		type_;
    LineStyle		linestyle_;	//!< contains the color
    HeadType		headtype_;
    HeadType		tailtype_;
    HandedNess		handedness_;
    int			headsz_; //!< also the tail size
    bool		fixedheadsz_; //!< if false, headsz_ is % of arrow len

};


#endif
