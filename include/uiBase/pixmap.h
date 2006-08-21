#ifndef pixmap_h
#define pixmap_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: pixmap.h,v 1.13 2006-08-21 17:14:45 cvsbert Exp $
________________________________________________________________________

-*/

#include "namedobj.h"
#include "iodrawimpl.h"
class QPixmap;
class QBitmap;
class QPaintDevice;
class ArrayRGB;
class Color;


/*!\brief Off-screen pixel-based paint device

  The most important constructor is the one accepting a file name.
  If the filename starts with '>', it is assumed to be a filename only, no
  directory. The icon will be loaded from the $WORK/data/icons.cur directory,
  or if not found there, from $WORK/data/icons.Default.
  If the file does not exist, or is of an unknown format, the pixmap becomes a
  null pixmap. If format is specified, attempts to read the pixmap using the
  specified format. If format is not specified (default), the loader reads
  a few bytes from the header to guess the file format. 

*/

class ioPixmap : public NamedObject,
		 public ioDrawAreaImpl
{
public:
			ioPixmap() : qpixmap_(0)		{}
			ioPixmap(const ArrayRGB&);
			ioPixmap(const char* xpm[]);
			ioPixmap(int w,int h);
			ioPixmap(const QPixmap&);
			ioPixmap(const ioPixmap&);
			ioPixmap(const char* filename,const char* fmt=0);
			ioPixmap(const char* colortablename,int w,int h);
    virtual		~ioPixmap();

    void		convertFromArrayRGB(const ArrayRGB&);

    QPixmap* 		Pixmap()		{ return qpixmap_; }
    const QPixmap*  	Pixmap() const		{ return qpixmap_; }

    void		fill(const Color&);

    int			width();
    int         	height();

    const char*		source() const		{ return srcname_.buf(); }

protected:
    
    virtual QPaintDevice* mQPaintDevice();         
    QPixmap*		qpixmap_; 
    BufferString	srcname_;

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
    QBitmap* 		Bitmap()		{ return (QBitmap*)qpixmap_; }
    const QBitmap*  	Bitmap() const		{ return (QBitmap*)qpixmap_; }
};


#endif
