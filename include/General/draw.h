#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "enums.h"
#include "color.h"
#include "geometry.h"
#include "odcommonenums.h"
#include "uistring.h"


mExpClass(General) Alignment
{
public:

    enum Pos	{ Start, Stop, Center };
    enum HPos	{ Left, Right, HCenter };
		mDeclareEnumUtils(HPos)
    enum VPos	{ Top, Bottom, VCenter };
		mDeclareEnumUtils(VPos)

		Alignment( HPos h=Left, VPos v=Top );
		Alignment( Pos h, Pos v );

    HPos	hPos() const		{ return hor_; }
    VPos	vPos() const		{ return ver_; }
    Pos		pos(bool hor) const;

    void	set( HPos h, VPos v )	{ hor_ = h; ver_ = v; }
    void	set( HPos h )		{ hor_ = h; }
    void	set( VPos v )		{ ver_ = v; }
    void	set(Pos h,Pos v);

    int		uiValue() const;
    void	setUiValue(int);

    static HPos	opposite( HPos p );
    static VPos	opposite( VPos p );

protected:

    HPos	hor_;
    VPos	ver_;
};


#define mAlignment(h,v) Alignment(Alignment::h,Alignment::v)
#define mDeclAlignment(nm,h,v) Alignment nm( Alignment::h, Alignment::v )


mExpClass(General) MarkerStyle2D
{
public:

    enum Type			{ None, Square, Circle, Cross, Plus, Target,
				  HLine, VLine, Plane, Triangle, Arrow };
				mDeclareEnumUtils(Type)

				MarkerStyle2D( Type tp=Square, int sz=1,
					       OD::Color col=OD::Color::Black(),
					       float rot=0);

    bool			operator==(const MarkerStyle2D& a) const;
    const MarkerStyle2D&	operator=(const MarkerStyle2D& a);

    Type			type_;
    int				size_;
    OD::Color			color_;
    float			rotation_; //clockwise rotion angle in degrees.

    bool			isVisible() const;

    void			toString(BufferString&) const;
    void			fromString(const char*);

};


mExpClass(General) MarkerStyle3D
{
public:

    enum Type		{ None=-1,
			  Cube=0, Cone, Cylinder, Sphere, Arrow, Cross,
			  Point, Plane };
			mDeclareEnumUtils(Type)

			MarkerStyle3D( Type tp=Cube, int sz=3,
				       OD::Color col=OD::Color::White() );

    Type		type_;
    int			size_;
    OD::Color		color_;

    bool		isVisible() const;

    void		toString(BufferString&) const;
    void		fromString(const char*);

    bool		operator==(const MarkerStyle3D& b) const;
    bool		operator!=(const MarkerStyle3D& b) const;
    static  MarkerStyle2D::Type getMS2DType(MarkerStyle3D::Type);
};


namespace OD
{

mExpClass(General) LineStyle
{
public:

    enum Type		{ None, Solid, Dash, Dot, DashDot, DashDotDot };
			// This enum should not be changed: it is cast
			// directly to a UI enum.
			mDeclareEnumUtils(Type)

			LineStyle(Type t=Solid,int w=1,
						OD::Color c=OD::Color::Black());

    bool		operator ==( const LineStyle& ls ) const;
    bool		operator !=( const LineStyle& ls ) const;

    Type		type_;
    int			width_;
    OD::Color		color_;

    bool		isVisible() const;

    void		toString(BufferString&) const;
    void		fromString(const char*);

};

} // namespace OD


mExpClass(General) FillPattern
{
public:

			FillPattern( int typ=0, int opt=0 )
			    : type_(typ), opt_(opt)		{}

    static void	getTypeNames(BufferStringSet&);
    static void	getOptNames(int,BufferStringSet&);

    int		type_;
    int		opt_;

    void	setNoFill()		{ type_ = opt_ = 0; }
    void	setFullFill()		{ type_ = 1; opt_ = 0; }
    void	setMediumDotted()	{ type_ = 1; opt_ = 4; }

};



mExpClass(General) ArrowHeadStyle
{
public:
    enum Type		{ Line, Triangle, Square, Cross };
    enum HandedNess	{ TwoHanded, LeftHanded, RightHanded };

		ArrowHeadStyle( int sz=1, Type t=Line, HandedNess h=TwoHanded );

    void	setBoldNess(int b);

    int		sz_;
    Type	type_;
    HandedNess	handedness_;

};


mExpClass(General) ArrowStyle
{
public:

    enum Type		{ HeadOnly, TwoSided, TailOnly, HeadNorTail };

			ArrowStyle( int boldness=1, Type t=HeadOnly );
    void		setBoldNess( int b );

    bool		hasHead() const;
    bool		hasTail() const;

    Type		type_;
    OD::LineStyle	linestyle_;	//!< contains the color
    ArrowHeadStyle	headstyle_;
    ArrowHeadStyle	tailstyle_;

};


mClass(General) PlotAnnotation
{
public:

    enum LineType	{ Normal=0, Bold=1, HighLighted=2 };

			PlotAnnotation()
			    : pos_(mUdf(float))
			    , txt_(uiString::emptyString())
			    , linetype_(Normal)			{}

    float		pos_;
    uiString		txt_;
    LineType		linetype_;

    PlotAnnotation&	operator=( const PlotAnnotation& from )
			{
			    pos_ = from.pos_;
			    linetype_ = from.linetype_;
			    txt_ = from.txt_;
			    return *this;
			}

    bool		operator==( const PlotAnnotation& from ) const
			{
			    return pos_ == from.pos_
				&& linetype_ == from.linetype_;
			}

    bool		isNormal() const
			{ return linetype_ == Normal; }

};


mExpClass(General) WellSymbol
{
public:
				WellSymbol(OD::WellType tp=OD::UnknownWellType,
					   int sz=8,
					   OD::Color pcol=OD::Color::Black())
				: type_(tp),size_(sz),color_(pcol)	{}

    OD::WellType		type_;
    int				size_;
    OD::Color			color_;
};
