#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2014
________________________________________________________________________

-*/

#include "generalmod.h"
#include "namedobj.h"
#include "bufstringset.h"
#include "odcommonenums.h"


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

  Therefore, for icons that can be used in both worlds, provide at least one
  of about 48x48 and give it the usual name. Then add one or more pixmaps with
  different resolutions, like a small one of about 24x24 so there are then 2
  icons in the icon set - like:
  "myicon.png" and "myicon.small.png".

  The icon identifier that is required is usually the file name for 'large'
  without '.png'. You can also pass the file name itself, or a full path.
  Note that *only* PNG type files are supported.

  The necessary icons will be sought in the user-selected icon set first. If
  nothing can be found there, the then the same procedure will be
  applied in the 'Default' set (if that isn't already the current set).

  You can get an 'empty' icon by passing a null or empty string. This is OK but
  uiIcon does not like that.

  If the requested icon is simply not there then a pErrMsg will follow;
  "iconnotfound.png" will be displayed.

 */


namespace OD
{

mExpClass(General) IconFile : public NamedObject
{
public:

    typedef OD::StdActionType StdActionType;

			IconFile(const char* identifier=0);
			IconFile(StdActionType);

    void		set(const char* identifier);
    void		set( StdActionType t )	{ set( getIdentifier(t) ); }

    static const char*	getIdentifier(StdActionType);
    static bool		isPresent(const char* identifier);

    bool		haveData() const	{ return !nms_.isEmpty(); }
    const BufferStringSet& fileNames() const	{ return nms_; }

    static const char*	notFoundIconFileName();

protected:

    bool		trydeficons_;
    BufferString	icdirnm_;
    BufferString	deficdirnm_;
    BufferStringSet	nms_;

    bool		findIcons(const char*,bool shortname);

private:

    void		init(const char*);

};

} // namespace OD

