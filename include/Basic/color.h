#ifndef color_h
#define color_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		22-3-2000
 RCS:		$Id: color.h,v 1.24 2012-08-03 13:00:11 cvskris Exp $
________________________________________________________________________

Color is an RGB color object, with a transparancy. The storage is in a 4-byte
integer, similar to Qt.

-*/


#include "basicmod.h"
#include "gendefs.h"
class BufferStringSet;
template <class T> class TypeSet;


mClass(Basic) Color
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

    bool		isVisible() const;

    unsigned int	rgb() const;
    unsigned int&	rgb();

    void         	set( unsigned char r_, unsigned char g_,
			     unsigned char b_, unsigned char t_=0 );

    Color		complementaryColor() const;
    Color		operator*(float) const;
    void		lighter( float f=1.1 );
    void        	setRgb( unsigned int rgb_  );
    void		setTransparency( unsigned char t_ );
    void		setHSV(unsigned char h,unsigned char s,unsigned char v);
    void		getHSV(unsigned char&,unsigned char&,
	    		       unsigned char&) const;
    void		setStdStr(const char*); //!< e.g. "#00ff32"
    const char*		getStdStr(bool withhash=true,
	    			  int transpopt=0) const;
    			//!< without hash Google KML standard -> order reversed
    			//!< transpopt -1=opacity 0=not 1=transparency

    void		fill(char*) const;
    bool		use(const char*);

    static Color	NoColor()	{ return  Color( 0, 0, 0, 255 ); }

    static Color	Black()		{ return  Color( 0, 0, 0, 0 ); }
    static Color	White()		{ return  Color( 255, 255, 255, 0 ); }
    static Color	DgbColor()	{ return  Color( 2, 240, 4, 0 ); }	
    static Color	LightGrey()	{ return  Color( 211, 211, 211, 0 ); }
    static Color	Peach()		{ return  Color( 255, 218, 185, 0 ); }

    static unsigned char getUChar( float v );

    static int		nrStdDrawColors();
    static Color	stdDrawColor(int);

    const char*				largeUserInfoString() const;
    const char*				getDescription() const;
    bool				fromDescription(const char*);
    static const BufferStringSet&	descriptions();
    static const TypeSet<Color>&	descriptionCenters();

protected:

    unsigned int	col_;
};


#endif

