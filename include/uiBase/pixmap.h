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

#include "uibasemod.h"
#include "namedobj.h"

mFDQtclass(QBitmap)
mFDQtclass(QPaintDevice)
mFDQtclass(QPixmap)

class BufferStringSet;
namespace ColTab { class Sequence; }
class uiRGBArray;


/*!\brief Off-screen pixel-based paint device

  Icons pixmaps can be created from the identifier, see OD::IconFile: the file
  name without extension.

*/

mExpClass(uiBase) ioPixmap : public NamedObject
{
public:
			ioPixmap() : qpixmap_(0)		{}
			ioPixmap(const uiRGBArray&);
			ioPixmap(const char* xpm[]);
			ioPixmap(int w,int h);
			ioPixmap(const mQtclass(QPixmap&));
			ioPixmap(const ioPixmap&);
			ioPixmap(const char* icon_identifier);
			ioPixmap(const ColTab::Sequence&,int w,int h,bool hor);
    virtual		~ioPixmap();

    void		convertFromRGBArray(const uiRGBArray&);

    mQtclass(QPixmap*)	qpixmap()		{ return qpixmap_; }
    const mQtclass(QPixmap*) qpixmap() const	{ return qpixmap_; }

    void		fill(const Color&);

    int			width() const;
    int			height() const;
    bool		isEmpty() const;

    const char*		source() const		{ return srcname_.buf(); }

    bool		save(const char* fnm,const char* fmt=0,
			     int quality=-1) const;

    static bool		isPresent(const char*);
    static void		supportedImageFormats(BufferStringSet&);

    // DEPRECATED: will be gone after 5.0. Use only PNG icons.
			ioPixmap(const char* filename,const char* fmt);

protected:

    mQtclass(QPixmap*)	qpixmap_;
    BufferString	srcname_;
};



// DEPRECATED: will be gone after 5.0. Was never used.
/*! \brief pixmap with the depth of 2 (b/w)

If the file does not exist, or is of an unknown format, the pixmap becomes
a null pixmap.

If format is specified, attempts to read the pixmap using the specified format.
If format is not specified (default), the loader reads a few bytes from the
header to guess the file format.

*/

mExpClass(uiBase) ioBitmap : public ioPixmap
{
public:
			ioBitmap(const char* filename,const char* fmt=0);
    mQtclass(QBitmap*)	Bitmap();
    const mQtclass(QBitmap*)	Bitmap() const;

};

mGlobal(uiBase) void supportedImageFormats(BufferStringSet&);

#endif
