#ifndef odiconfile_h
#define odiconfile_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "namedobj.h"


/*!\brief Constructs file names for OD icons.

  Pixmap-based icons have a problem of scale. If the pixmap is smaller than the
  size on screen you will see a 'blocky' == bad icon. If on the other hand
  the pixmap is a lot larger you will also not get a good icon - no matter how
  good the down-scale algorithm.

  Thus, we need pixmaps that are exactly right or a little bit larger - upto
  about 50% off is reasonably problem-free.

  Historically, we targeted icons for toolbuttons only. These are what could
  be called 'large' (usually between 32x32 and 48x48). Nowadays we also
  add icons on buttons, trees, lists etc., too. These will be 'small' (usually
  between 16x16 and 24x24).

  Therefore, for icons that can be used in both worlds, provide one of about
  48x48 and give it the usual name. Then add your small one of about 24x24 so
  there are then 2 icons in the icon set - like:
  "myicon.png" and "myicon_small.png".

  The icon identifier that is required is usually the file name for 'large'
  without '.png'. You can also pass the file name itself, or a full path.
  Note that *only* PNG type files are supported.

  The necessary icon will be sought in the user-selected icon set first. If the
  requested size cannot be found, then the other size will be used. If that
  still doesn't result in an icon file, then the same procedure will be
  applied in the 'Default' set (if that isn't already the current set).

  You can get an 'empty' icon by passing a null or empty string. This is OK but
  uiPixmap does not like that.

  If the requested icon is simply not there then a pErrMsg will follow;
  "iconnotfound.png" will be displayed.

 */

namespace OD
{

mExpClass(General) IconFile : public NamedObject
{
public:

			IconFile(const char* identifier=0);

    void		set(const char* identifier);
    static bool		isPresent(const char* identifier);

    BufferString	fullFileName(bool smalltype=false) const;

protected:

    enum State		{ Exists, SmallOnly, Empty, Explicit, NotFound };

    bool		usedeficons_;
    bool		fromdefault_;
    BufferString	icdirnm_;
    BufferString	deficdirnm_;
    BufferString	fullpath_;
    State		state_;

    bool		tryFind(const char*,bool smalltype,State);
    BufferString	getIconFileName(const char*,bool def,
					bool smalltype) const;

};


} // namespace OD



#endif
