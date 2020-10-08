#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		22-3-2000
________________________________________________________________________

-*/


#include "basicmod.h"
#include "uistring.h"
class BufferStringSet;


/*!\brief Color is an RGB color object, with a transparancy.
  The storage is in a 4-byte integer, similar to Qt. */

mExpClass(Basic) Color
{
public:

    typedef unsigned char	CompType;
    typedef od_uint32		RGBRepType;

			Color(CompType r_=255, CompType g_=255,
				CompType b_=255, CompType t_=0);
			Color(RGBRepType);
			mImplSimpleEqOpers1Memb(Color,rgb_)

    CompType		r() const;
    CompType		g() const;
    CompType		b() const;
    CompType		t() const;
    float		rR() const		{ return comp2Ratio( r() ); }
    float		gR() const		{ return comp2Ratio( g() ); }
    float		bR() const		{ return comp2Ratio( b() ); }
    float		tR() const		{ return comp2Ratio( t() ); }

    bool		isVisible() const;

    RGBRepType		rgb() const		{ return rgb_; }
    RGBRepType&		rgb()			{ return rgb_; }
    void		setRgb( RGBRepType v )	{ rgb_ = v; }

    void		set(CompType r_,CompType g_,CompType b_,CompType t_=0);

    float		average() const { return ((float)r() + g() + b())/3.f; }
    Color		complementaryColor() const;
    Color		lighter(float fac) const;
    Color		darker( float fac ) const
					{ return lighter( -fac ); }
    static Color	between(Color,Color,float relpos=0.5f);

    void		setTransparency(CompType);
    void		setTransparencyF(float);
    void		setTransparencyAsRatio(float);
    void		setHSV(CompType h,CompType s,CompType v);
    void		getHSV(CompType&,CompType&,CompType&) const;
    void		setStdStr(const char*); //!< e.g. "#00ff32"
    BufferString	getStdStr(bool withhash=true,int transpopt=0) const;
			//!< without hash Google KML standard -> order reversed
			//!< transpopt -1=opacity 0=not 1=transparency
    static void		convertToStr(const TypeSet<Color>&,BufferStringSet&);
    static void		convertFromStr(const BufferStringSet&,TypeSet<Color>&);

    void		fill(BufferString&) const;
    bool		use(const char*);

    static Color	NoColor()	{ return Color(0,0,0,255); }

    static Color	Anthracite()	{ return Color(50, 50, 50, 0); }
    static Color	Black()		{ return Color(0,0,0,0); }
    static Color	Blue()		{ return Color(0,0,255,0); }
    static Color	DgbColor()	{ return Color(2,240,4,0); }
    static Color	Green()		{ return Color(0,255,0,0); }
    static Color	LightGrey()	{ return Color(211,211,211,0); }
    static Color	Orange()	{ return Color(255,170,0); }
    static Color	Peach()		{ return Color(255,218,185,0); }
    static Color	Pink()		{ return Color(255,0,255,0); }
    static Color	Red()		{ return Color(255,0,0,0); }
    static Color	White()		{ return Color(255,255,255,0); }
    static Color	Yellow()	{ return Color(255,255,0,0); }

    static CompType	fComp2Comp(float);
    static CompType	ratio2Comp(float);
    static float	comp2Ratio(CompType);

			// Std draw colors are distinct colors for data series
    static int		nrStdDrawColors();
    static Color	stdDrawColor(int);
    static Color	stdDrawColor(const char*);

			// The 'description' uses the standard Html color names
			// if the color is not exactly on one, you will see
			// a tilde '~' in front of the color
    BufferString	getDescription() const;
    bool		fromDescription(const char*);
    static Color	getColorFromDescription(const char*);
    static void		getDescriptions(BufferStringSet&);
    static void		getDescriptionCenters(TypeSet<Color>&);

    uiString		userInfoString(bool withdetails=false) const;

protected:

    RGBRepType		rgb_;

public:

    mDeprecated static	CompType getUChar( float v ) { return fComp2Comp(v); }
    mDeprecated static	float getFloat( CompType v ) { return comp2Ratio(v); }
    mDeprecated float	rF() const		{ return rR(); }
    mDeprecated float	gF() const		{ return gR(); }
    mDeprecated float	bF() const		{ return bR(); }
    mDeprecated float	tF() const		{ return tR(); }

};


namespace Values {

/*!\brief Undefined Color. */

template<>
mClass(Basic) Undef<Color>
{
public:
    static Color	val()			{ return Color::NoColor(); }
    static bool		hasUdf()		{ return false; }
    static bool		isUfd(Color&)		{ return false; }
    static void		setUdf(Color&)		{}
};

}
