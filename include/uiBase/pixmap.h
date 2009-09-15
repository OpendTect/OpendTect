#ifndef pixmap_h
#define pixmap_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: pixmap.h,v 1.23 2009-09-15 11:40:56 cvsnanne Exp $
________________________________________________________________________

-*/

#include "namedobj.h"
#include "iodrawimpl.h"

class QBitmap;
class QPaintDevice;
class QPixmap;

class BufferStringSet;
class Color;
namespace ColTab { class Sequence; }
class uiRGBArray;


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

mClass ioPixmap : public NamedObject, public ioDrawAreaImpl
{
public:
			ioPixmap() : qpixmap_(0)		{}
			ioPixmap(const uiRGBArray&);
			ioPixmap(const char* xpm[]);
			ioPixmap(int w,int h);
			ioPixmap(const QPixmap&);
			ioPixmap(const ioPixmap&);
			ioPixmap(const char* filename,const char* fmt=0);
			ioPixmap(const ColTab::Sequence&,int w,int h);
    virtual		~ioPixmap();

    void		convertFromRGBArray(const uiRGBArray&);

    QPixmap* 		qpixmap()		{ return qpixmap_; }
    const QPixmap*  	qpixmap() const		{ return qpixmap_; }

    void		fill(const Color&);

    int			width() const;
    int         	height() const;
    bool		isEmpty() const;

    const char*		source() const		{ return srcname_.buf(); }

    bool		save(const char* fnm,const char* fmt=0,
	    		     int quality=-1) const;
    static void		supportedImageFormats(BufferStringSet&);

protected:
    
    QPixmap*		qpixmap_; 
    BufferString	srcname_;

    virtual QPaintDevice* qPaintDevice();         

};



//!A pixmap with the depth of 2 (b/w)
mClass ioBitmap : public ioPixmap
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
			ioBitmap(const char* filename,const char* fmt=0); 
    QBitmap* 		Bitmap();
    const QBitmap*  	Bitmap() const;
};


mGlobal void supportedImageFormats(BufferStringSet&);


#endif
