#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		22-3-2000
________________________________________________________________________

-*/


#include "basicmod.h"
#include "gendefs.h"
class BufferStringSet;


/*!
\brief Color is an RGB color object, with a transparancy. The storage is in
a 4-byte integer, similar to Qt.
*/

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
    Color		lighter(float fac) const;
    Color		darker( float fac ) const
					{ return lighter( -fac ); }

    void		setRgb( unsigned int rgb_  );
    void		setTransparency( unsigned char t_ );
    void		setTransparencyF( float tf ); //!< should be from 0 to 1
    void		setHSV(unsigned char h,unsigned char s,unsigned char v);
    void		getHSV(unsigned char&,unsigned char&,
			       unsigned char&) const;
    void		setStdStr(const char*); //!< e.g. "#00ff32"
    BufferString	getStdStr(bool withhash=true,
				  int transpopt=0) const;
			//!< without hash Google KML standard -> order reversed
			//!< transpopt -1=opacity 0=not 1=transparency

    void		fill(BufferString&) const;
    bool		use(const char*);

    static Color	NoColor()	{ return Color(0,0,0,255); }

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

    static unsigned char getUChar( float v );
    static float	 getFloat(unsigned char);

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

    BufferString	largeUserInfoString() const;

protected:

    unsigned int	col_;
};


namespace Values {

/*!\brief Undefined Color. */

template<>
mClass(Basic) Undef<Color>
{
public:
    static Color	val()			{ return Color::NoColor(); }
    static bool		hasUdf()		{ return false; }
    static bool		isUfd(Color& col)	{ return false; }
    static void		setUdf(Color& col)	{}
};

}
