#ifndef pixmap_h
#define pixmap_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: pixmap.h,v 1.5 2003-09-08 13:02:59 nanne Exp $
________________________________________________________________________

-*/

#include "uidobj.h"
#include "iodrawimpl.h"

class QPixmap;
class QPaintDevice;

class ArrayRGB;
class Color;

//! off-screen pixel-based paint device
class ioPixmap : public UserIDObject , public ioDrawAreaImpl
{
public:
			ioPixmap() : qpixmap( 0 ) {}
			ioPixmap( const ArrayRGB& );
			ioPixmap( const char * xpm[] );
			ioPixmap( int w, int h, int depth = -1);
			ioPixmap( const QPixmap& );

			/*! \brief Constructs a pixmap from the file fileName. 

			If the file does not exist, or is of an unknown format,
			the pixmap becomes a null pixmap. 

			If format is specified, attempts to read the pixmap 
			using the specified format. If format is not specified
			(default), the loader reads a few bytes from the header
			to guess the file format. 

			*/
			ioPixmap( const char* fileName, const char * format=0 ); 
    virtual		~ioPixmap();

    bool		convertFromArrayRGB(const ArrayRGB&);

    QPixmap* 		Pixmap()		{ return qpixmap; }
    const QPixmap*  	Pixmap() const		{ return qpixmap; }

    void		fill(const Color&);

    int			width();
    int         	height();

    
protected:
    
    virtual QPaintDevice* mQPaintDevice();         
    QPixmap*		qpixmap; 

};


#endif
