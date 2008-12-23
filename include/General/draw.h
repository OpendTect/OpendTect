#ifndef draw_h
#define draw_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          26/07/2000
 RCS:           $Id: draw.h,v 1.22 2008-12-23 11:05:17 cvsdgb Exp $
________________________________________________________________________

-*/

#include "enums.h"
#include "color.h"
#include "geometry.h"

namespace OD
{
    enum Alignment
    {
	AlignLeft = 0x0001,
	AlignRight = 0x0002,
	AlignHCenter = 0x0004,
	AlignJustify = 0x0008,
	AlignAbsolute = 0x0010,
	AlignTop = 0x0020,
	AlignBottom = 0x0040,
	AlignVCenter = 0x0080,
	AlignCenter = AlignVCenter | AlignHCenter
    };
}


class Alignment
{
public:
			Alignment( OD::Alignment h=OD::AlignLeft,
				   OD::Alignment v=OD::AlignBottom )
			: hor_(h), ver_(v)	{}

    OD::Alignment	hor_;
    OD::Alignment	ver_;
};


#define mAlign(h,v) Alignment(OD::h,OD::v)


class MarkerStyle2D
{
public:

    enum Type		{ None, Square, Circle, Cross };
			DeclareEnumUtils(Type)

			MarkerStyle2D( Type tp=Square, int sz=2,
				       Color col=Color::Black() )
			: type_(tp), size_(sz), color_(col)	{}

    bool		operator==(const MarkerStyle2D& a) const
			{ return a.type_==type_ && a.size_==size_ &&
			         a.color_==color_; }
    MarkerStyle2D&	operator=(const MarkerStyle2D& a) 
			{ type_ = a.type_ ;
			  size_ = a.size_;
			  color_ = a.color_;
		          return *this;	}

    Type		type_;
    int			size_;
    Color		color_;

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
				       Color col=Color::White() )
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

			LineStyle( Type t=Solid,int w=1,Color c=Color::Black() )
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


class ArrowHeadStyle
{
public:
    enum Type		{ Line, Triangle, Square, Cross };
    enum HandedNess	{ TwoHanded, LeftHanded, RightHanded };

			ArrowHeadStyle( int sz=1, Type t=Line,
			      HandedNess h=TwoHanded )
			    : sz_(sz), type_(t), handedness_(h)	{}

    inline void		setBoldNess( int b )	{ sz_ = 3*b; }

    int		sz_;
    Type	type_;
    HandedNess	handedness_;

};


class ArrowStyle
{
public:

    enum Type		{ HeadOnly, TwoSided, TailOnly, HeadNorTail };

			ArrowStyle( int boldness=1, Type t=HeadOnly )
			: type_(t)
			, linestyle_(LineStyle::Solid,boldness)
			{ setBoldNess(boldness); }

    inline void		setBoldNess( int b )
			{ linestyle_.width_ = b;
			  headstyle_.setBoldNess(b);
			  tailstyle_.setBoldNess(b); }

    inline bool		hasHead() const
    			{ return headstyle_.sz_ > 0 && type_ < TailOnly; }
    inline bool		hasTail() const
    			{ return tailstyle_.sz_ > 0
			      && (type_ == TwoSided || type_ == TailOnly); }

    Type		type_;
    LineStyle		linestyle_;	//!< contains the color
    ArrowHeadStyle	headstyle_;
    ArrowHeadStyle	tailstyle_;

};


#endif
