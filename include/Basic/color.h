#ifndef color_h
#define color_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		22-3-2000
 RCS:		$Id: color.h,v 1.1 2001-05-14 13:22:24 bert Exp $
________________________________________________________________________

Color is an RGB color object, with a transparancy. The storage is in a 4-byte
integer, similar to Qt.

-*/


#include <gendefs.h>


class Color
{
public:

			Color( unsigned char r_=255, unsigned char g_=255,
				unsigned char b_=255, unsigned char t_=0 )
			{ set( r_, g_, b_, t_ ); }

    inline bool		operator ==( const Color& c ) const
			{ return col_ == c.col_; }
    inline bool		operator !=( const Color& c ) const
			{ return col_ != c.col_; }

    inline void		lighter( float f=1.1 )
			{
			    if ( f < 0 ) f = -f;
			    set( getUChar(r()*f), getUChar(g()*f),
				 getUChar(b()*f) );
			}

    inline unsigned char r() const
			{ return (unsigned char)((col_ >> 16) & 0xff); }
    inline unsigned char g() const
			{ return (unsigned char)((col_ >> 8) & 0xff); }
    inline unsigned char b() const
			{ return (unsigned char)(col_ & 0xff); }
    inline unsigned char t() const
			{ return (unsigned char)((col_ >> 24) & 0xff); }

    inline unsigned int rgb() const
			{ return col_; }

    inline void         set( unsigned char r_, unsigned char g_,
			     unsigned char b_, unsigned char t_=0 )
                        {
			    col_ = ( (unsigned int)(t_&0xff) << 24 )
				 | ( (unsigned int)(r_&0xff) << 16 )
				 | ( (unsigned int)(g_&0xff) <<  8 )
				 |		   (b_&0xff);
                        }

    inline void         setRgb( unsigned int rgb_  )
                        { col_ = rgb_; }

    void		fill(char*) const;
    bool		use(const char*);

    static Color	NoColor;
    static Color	Black;
    static Color	White;
    static Color	DgbColor;
    static Color	Wheat;
    static Color	LightGrey;

    static const Color&	drawDef(int);

    static unsigned char getUChar( float v )
    { return v > 254.5 ? 255 : (v < 0.5 ? 0 : (unsigned char)(v+.5)); }

protected:

    unsigned int	col_;

};


#endif
