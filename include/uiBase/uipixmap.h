#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/12/1999
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "namedobj.h"
#include "coltab.h"

mFDQtclass(QBitmap)
mFDQtclass(QPaintDevice)
mFDQtclass(QPixmap)

class BufferStringSet;
namespace ColTab { class Sequence; }
namespace File { class FormatList; }
class uiRGBArray;


namespace OD
{
    mGlobal(uiBase) void GetSupportedImageFormats(File::FormatList&,
					  bool forread,bool withprint=false);
}


/*!\brief Off-screen pixel-based paint device

  Icons pixmaps can be created from the identifier, see odiconfile.h. It is,
  basically, the file name without extension.

*/

mExpClass(uiBase) uiPixmap : public NamedObject
{
public:

			uiPixmap();
			uiPixmap(int w,int h,Color col=Color::NoColor());
			uiPixmap(const char* icon_identifier);
			uiPixmap(const uiRGBArray&);
			uiPixmap(const char* xpm[]);
			uiPixmap(const mQtclass(QPixmap&));
			uiPixmap(const uiPixmap&);

    virtual		~uiPixmap();

    void		convertFromRGBArray(const uiRGBArray&);

    mQtclass(QPixmap*)	qpixmap()		{ return qpixmap_; }
    const mQtclass(QPixmap*) qpixmap() const	{ return qpixmap_; }

    void		fill(const Color&);

    int			width() const;
    int			height() const;
    bool		isEmpty() const;

    const char*		source() const		{ return srcname_; }
    void		setSource( const char* src ) { srcname_ = src; }

    bool		save(const char* fnm,const char* fmt=0,
			     int quality=-1) const;

    static bool		isPresent(const char*);

protected:

    mQtclass(QPixmap*)	qpixmap_;
    BufferString	srcname_;
};
