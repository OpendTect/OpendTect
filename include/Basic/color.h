#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"
class BufferStringSet;

/*!
\brief Color is an RGB color object, with a transparancy. The storage is in
a 4-byte integer, similar to Qt.
*/
namespace OD
{
mExpClass(Basic) Color
{
public:

			Color( unsigned char r_=255, unsigned char g_=255,
				unsigned char b_=255, unsigned char t_=0 );
			Color( unsigned int rgbval );

    bool		operator ==( const Color& c ) const;
    bool		operator !=( const Color& c ) const;

    unsigned char	r() const;
    unsigned char	g() const;
    unsigned char	b() const;
    unsigned char	t() const;
    float		rF() const { return getFloat( r() ); }
    float		gF() const { return getFloat( g() ); }
    float		bF() const { return getFloat( b() ); }
    float		tF() const { return getFloat( t() ); }

    bool		isVisible() const;

    unsigned int	rgb() const;
    unsigned int&	rgb();

    void		set( unsigned char r_, unsigned char g_,
			     unsigned char b_, unsigned char t_=0 );

    float		average() const { return ((float) r()+g()+b())/3.0f; }
    Color		complementaryColor() const;
    TypeSet<Color>	complimentaryColors(int) const;
    Color		lighter(float fac) const;
    Color		darker( float fac ) const
					{ return lighter( -fac ); }
    float		contrast(const Color&) const;

    float		getRelLuminance() const;
    void		setRgb( unsigned int rgb_  );
    void		setTransparency( unsigned char t_ );
    void		setTransparencyF( float tf );
    void		setHSV(unsigned char h,unsigned char s,unsigned char v);
    void		getHSV(unsigned char&,unsigned char&,
			       unsigned char&) const;
    void		setStdStr(const char*); //!< e.g. "#00ff32"
    const char*		getStdStr(bool withhash=true,
				  int transpopt=0) const;
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
    static Color	Red()		{ return Color(255,0,0,0); }
    static Color	White()		{ return Color(255,255,255,0); }
    static Color	Yellow()	{ return Color(255,255,0,0); }

    static Color	interpolate(const Color&,const Color&,float frac=0.5);

    static unsigned char getUChar( float v );
    static float	 getFloat(unsigned char);

    static int		nrStdDrawColors();
    static Color	stdDrawColor(int);

    const char*		largeUserInfoString() const;
    const char*		getDescription() const;
    bool		fromDescription(const char*);
    static const BufferStringSet& descriptions();
    static const TypeSet<Color>& descriptionCenters();

protected:

    unsigned int	col_;
};

}


namespace Values {

/*!
\brief Undefined Color.
*/

template<>
mClass(Basic) Undef<OD::Color>
{
public:
    static OD::Color	val()			{ return OD::Color::NoColor(); }
    static bool		hasUdf()		{ return false; }
    static bool		isUfd(OD::Color&)	{ return false; }
    static void		setUdf(OD::Color&)	{}
};

}
