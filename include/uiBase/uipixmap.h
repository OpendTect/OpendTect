#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "color.h"
#include "namedobj.h"

mFDQtclass(QBitmap)
mFDQtclass(QPaintDevice)
mFDQtclass(QPixmap)

class BufferStringSet;
class PixmapDesc;
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
			uiPixmap(const PixmapDesc&);

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
    void		scale(int w,int h);
    void		scaleToHeight(int h);
    void		scaleToWidth(int w);

    const char*		source() const		{ return srcname_.buf(); }

    mDeprecated("Use fillDesc instead with a PixmapDesc")
    void		params(BufferStringSet&) const;
			// -> sets color param to OD::Color::noColor()
			// -> thus set the color after calling this func.
    bool		save(const char* fnm,const char* fmt=0,
			     int quality=-1) const;

    void		fillDesc(PixmapDesc&) const;

    mDeprecatedDef
    static bool		isPresent(const char*);

    mDeprecated("Can be created in constructor now")
    static uiPixmap*	createFromParams(const BufferStringSet& params);

    static const char*	sKeyXpmSrc()		{ return "[xpm]"; }
    static const char*	sKeyRgbSrc()		{ return "[uiRGBArray]"; }
    static const char*	sKeyCreatedSrc()	{ return "[created]"; }
    static const char*	sKeyGradientSrc()	{ return "[Gradient]"; }
    static const char*	sKeyColorTabSrc()	{ return "[colortable]"; }

    static const int	sPmParamSrcIdx		= 0;
    static const int	sPmParamWidthIdx	= 1;
    static const int	sPmParamHeightIdx	= 2;
    static const int	sPmParamColIdx		= 3;

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
