#ifndef pixmap_h
#define pixmap_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id$
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

  Recommended: put your icons in the data/icons.Default directory, and
  specify .png files without any path.

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
			ioPixmap(const ColTab::Sequence&,int w,int h,bool hor);
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



/*! \brief pixmap with the depth of 2 (b/w)

If the file does not exist, or is of an unknown format, the pixmap becomes
a null pixmap. 

If format is specified, attempts to read the pixmap using the specified format.
If format is not specified (default), the loader reads a few bytes from the
header to guess the file format. 

*/

mClass ioBitmap : public ioPixmap
{
public:
			ioBitmap(const char* filename,const char* fmt=0); 
    QBitmap* 		Bitmap();
    const QBitmap*  	Bitmap() const;

};


mGlobal void supportedImageFormats(BufferStringSet&);


#endif
