#ifndef pixmap_h
#define pixmap_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: pixmap.h,v 1.7 2004-09-14 06:36:43 kristofer Exp $
________________________________________________________________________

-*/

#include "uidobj.h"
#include "iodrawimpl.h"

class QPixmap;
class QBitmap;
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



//!A pixmap with the depth of 2 (b/w)
class ioBitmap : public ioPixmap
{
public:
			/*! \brief Constructs a bitmap from the file fileName. 

			If the file does not exist, or is of an unknown format,
			the pixmap becomes a null pixmap. 

			If format is specified, attempts to read the pixmap 
			using the specified format. If format is not specified
			(default), the loader reads a few bytes from the header
			to guess the file format. 

			*/
			ioBitmap( const char* fileName, const char * format=0 ); 
    QBitmap* 		Bitmap()		{ return (QBitmap*)qpixmap; }
    const QBitmap*  	Bitmap() const		{ return (QBitmap*)qpixmap; }
};


#endif
