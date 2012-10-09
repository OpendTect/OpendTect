#ifndef draw_h
#define draw_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          26/07/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "enums.h"
#include "color.h"
#include "geometry.h"


mClass Alignment
{
public:

    enum Pos	{ Start, Stop, Center };
    enum HPos	{ Left, Right, HCenter };
		DeclareEnumUtils(HPos)
    enum VPos	{ Top, Bottom, VCenter };
		DeclareEnumUtils(VPos)

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


mClass MarkerStyle2D
{
public:

    enum Type			{ None, Square, Circle, Cross, Plus, Target,
				  HLine, VLine, Plane, Triangle, Arrow };
				DeclareEnumUtils(Type)

				MarkerStyle2D( Type tp=Square, int sz=2,
					       Color col=Color::Black(),
					       float rot=0);

    bool			operator==(const MarkerStyle2D& a) const;
    const MarkerStyle2D&	operator=(const MarkerStyle2D& a);

    Type			type_;
    int				size_;
    Color			color_;
    float			rotation_; //clockwise rotion angle in degrees. 

    bool			isVisible() const;

    void			toString(BufferString&) const;
    void			fromString(const char*);

};


mClass MarkerStyle3D
{
public:

    enum Type		{ None=-1,
			  Cube=0, Cone, Cylinder, Sphere, Arrow, Cross, 
			  Point, Plane };
			DeclareEnumUtils(Type)

			MarkerStyle3D( Type tp=Cube, int sz=3,
				       Color col=Color::White() );

    Type		type_;
    int			size_;
    Color		color_;

    bool		isVisible() const;

    void		toString(BufferString&) const;
    void		fromString(const char*);

    bool		operator==(const MarkerStyle3D& b) const;
    bool		operator!=(const MarkerStyle3D& b) const;
};


mClass LineStyle
{
public:

    enum Type		{ None, Solid, Dash, Dot, DashDot, DashDotDot };
			// This enum should not be changed: it is cast
			// directly to a UI enum.
			DeclareEnumUtils(Type)

			LineStyle( Type t=Solid,int w=1,Color c=Color::Black() );

    bool		operator ==( const LineStyle& ls ) const;
    bool		operator !=( const LineStyle& ls ) const;

    Type		type_;
    int			width_;
    Color		color_;

    bool		isVisible() const;

    void		toString(BufferString&) const;
    void		fromString(const char*);

};


mClass FillPattern
{
public:

    			FillPattern( int typ=0, int opt=0 )
			    : type_(typ), opt_(opt)		{}

    static void	getTypeNames(BufferStringSet&);
    static void	getOptNames(int,BufferStringSet&);

    int		type_;
    int		opt_;

};



mClass ArrowHeadStyle
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


mClass ArrowStyle
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


#endif
