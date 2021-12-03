#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/12/1999
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "color.h"
#include "namedobj.h"

mFDQtclass(QBitmap)
mFDQtclass(QPaintDevice)
mFDQtclass(QPixmap)

class BufferStringSet;
namespace ColTab { class Sequence; }
class uiRGBArray;


/*!\brief Off-screen pixel-based paint device

  Icons pixmaps can be created from the identifier, see odiconfile.h. It is,
  basically, the file name without extension.

*/

mExpClass(uiBase) uiPixmap : public NamedObject
{
public:

			uiPixmap();
			uiPixmap(int w,int h);
			uiPixmap(const char* icon_identifier);
			uiPixmap(const uiRGBArray&);
			uiPixmap(const char* xpm[]);
			uiPixmap(const mQtclass(QPixmap&));
			uiPixmap(const uiPixmap&);

    virtual		~uiPixmap();

    void		convertFromRGBArray(const uiRGBArray&);

    mQtclass(QPixmap*)	qpixmap()		{ return qpixmap_; }
    const mQtclass(QPixmap*) qpixmap() const	{ return qpixmap_; }

    void		fill(const OD::Color&);
    void		fill(const ColTab::Sequence&,bool hor);
    void		fillGradient(const OD::Color& col1,
				     const OD::Color& col2,bool hor);

    int			width() const;
    int			height() const;
    bool		isEmpty() const;

    const char*		source() const		{ return srcname_.buf(); }

    bool		save(const char* fnm,const char* fmt=0,
			     int quality=-1) const;

    static bool		isPresent(const char*);

protected:

    mQtclass(QPixmap*)	qpixmap_;
    BufferString	srcname_;
};



mGlobal(uiBase) void supportedImageFormats(BufferStringSet&,bool forread=false,
					   bool withprintformats=false);
mGlobal(uiBase) void getImageFormatDescs(BufferStringSet& descs,bool forread,
					bool withprintformats=false);
mGlobal(uiBase) void getImageFileFilter(BufferString& filter,bool forread,
					bool withprintformats=false);

