#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          26/07/2000
________________________________________________________________________

-*/

#include "generalmod.h"
#include "enums.h"
#include "color.h"
#include "geometry.h"
#include "uistring.h"
#include "uistringset.h"


namespace OD
{

mExpClass(General) Alignment
{
public:

    enum Pos	{ Start, Stop, Center };
    enum HPos	{ Left, Right, HCenter };
		mDeclareEnumUtils(HPos)
    enum VPos	{ Top, Bottom, VCenter };
		mDeclareEnumUtils(VPos)

		Alignment(HPos h=Left,VPos v=Top);
		Alignment(Pos h,Pos v);
		mImplSimpleEqOpers2Memb( Alignment, hor_, ver_ )

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




#define mAlignment(h,v) OD::Alignment(OD::Alignment::h,OD::Alignment::v)
#define mDeclAlignment(nm,h,v) \
OD::Alignment nm( OD::Alignment::h, OD::Alignment::v )


mExpClass(General) MarkerStyle2D
{
public:

    enum Type			{ None, Square, Circle, Cross, Plus, Target,
				  HLine, VLine, Plane, Triangle, Arrow };
				mDeclareEnumUtils(Type)

				MarkerStyle2D( Type tp=Square, int sz=1,
					       Color col=Color::Black(),
					       float rot=0);

    bool			operator==(const MarkerStyle2D&) const;
				mImplSimpleIneqOper( MarkerStyle2D )

    Type			type_;
    int				size_;
    Color			color_;
    float			rotation_; //clockwise rotion angle in degrees.

    bool			isVisible() const;

    void			toString(BufferString&) const;
    void			fromString(const char*);

};


mExpClass(General) MarkerStyle3D
{
public:

    enum Type		{ None=0,
			  Cube, Cone, Cylinder, Sphere, Arrow, Cross,
			  Point, Plane };
			mDeclareEnumUtils(Type)

			MarkerStyle3D( Type tp=Cube, int sz=3,
				       Color col=Color::White() );
			mImplSimpleEqOpers3Memb( MarkerStyle3D,
				type_, size_, color_ )

    Type		type_;
    int			size_;
    Color		color_;

    bool		isVisible() const;

    void		toString(BufferString&) const;
    void		fromString(const char*,bool v6_or_earlier=false);

    static MarkerStyle2D::Type getMS2DType(MarkerStyle3D::Type);

};


mExpClass(General) LineStyle
{
public:

    enum Type		{ None, Solid, Dash, Dot, DashDot, DashDotDot };
			// This enum should not be changed: it is cast
			// directly to a UI enum.
			mDeclareEnumUtils(Type)

			LineStyle(Type t=Solid,int w=1,Color c=Color::Black());

			mImplSimpleEqOpers3Memb( LineStyle,
				type_, width_, color_ )

    Type		type_;
    int			width_;
    Color		color_;

    bool		isVisible() const;

    void		toString(BufferString&) const;
    void		fromString(const char*);

};


mExpClass(General) FillPattern
{ mODTextTranslationClass(FillPattern)
public:

			FillPattern( int typ=0, int opt=0 )
			    : type_(typ), opt_(opt)		{}
			mImplSimpleEqOpers2Memb( FillPattern, type_, opt_ )


    static void	getTypeNames(BufferStringSet&);
    static void	getOptNames(int,BufferStringSet&);

    static void getTypeNamesForDisp(uiStringSet&);
    static void getOptNamesForDisp(int,uiStringSet&);

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
    LineStyle		linestyle_;	//!< contains the color
    ArrowHeadStyle	headstyle_;
    ArrowHeadStyle	tailstyle_;

};


mClass(General) PlotAnnotation
{
public:

    enum LineType	{ Normal=0, Bold=1, HighLighted=2 };

			PlotAnnotation()
			    : pos_(mUdf(float))
			    , txt_(uiString::empty())
			    , linetype_(Normal)			{}

    float		pos_;
    uiString		txt_;
    LineType		linetype_;

    OD::PlotAnnotation&	operator=( const PlotAnnotation& from )
			{
			    pos_ = from.pos_;
			    linetype_ = from.linetype_;
			    txt_ = from.txt_;
			    return *this;
			}

			mImplSimpleEqOpers2Memb( PlotAnnotation,
				pos_, linetype_ )

    bool		isNormal() const
			{ return linetype_ == Normal; }

};

};
