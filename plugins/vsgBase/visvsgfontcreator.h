#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsgbasemod.h"

#include "fontdata.h"
#include "notify.h"
#include "objectset.h"
#include "thread.h"
#include "typeset.h"
#include "vsggeoupdater.h"

#include <vsg/core/ref_ptr.h>
#include <vsg/text/Font.h>

#include <map>
#include <set>
#include <string>

namespace dGBVis
{

/*!\brief: makes vsg::Font objects from FontData, and caches them.

  The point size is not part of a font: it is applied by the label. Fonts are
  shared by family, bold or not, and italic or not. Windows builds the glyphs
  with DirectWrite, Linux with Fontconfig and FreeType. Elsewhere create()
  returns null. On Windows the universal families of FontData (Helvetica,
  Courier and Times) are looked up as Arial, Courier New and Times New Roman.
  On Linux Fontconfig resolves the requested family, including a strong alias
  such as Nimbus Sans L to Nimbus Sans. A family it does not know is rejected.

  request() makes the font on a worker thread. fontLoaded is triggered on the
  visualization thread, through the common vsgGeo::Updater, so there must be a
  common viewer (DataObject::setCommonViewer). It is also triggered when the
  font could not be made: get() is then still null. A font that is made before
  there is a common viewer is announced when the viewer is set. When the common
  viewer is reset the cache is emptied: compiled fonts keep the Vulkan device
  alive.
*/

mExpClass(vsgBase) FontCreator : public CallBacker
{
public:

    static FontCreator&		instance();

    static vsg::ref_ptr<vsg::Font> create(const FontData&);
				/*!<Blocks until the font is made, bypasses the
				    cache. Can be called from any thread. */

    vsg::ref_ptr<vsg::Font>	get(const FontData&) const;
				//!<Null while the font is not in the cache

    void			request(const FontData&);
				//!<Does nothing if cached or already loading

    CNotifier<FontCreator,const FontData&>	fontLoaded;

    void			announceLoaded();
				//!<Called by the Updater

    void			clear();
				/*!<Waits for loading fonts and empties
				    the cache. Call before the Vulkan device
				    goes away. */

private:

				FontCreator();
				~FontCreator();

    struct			LoadJob;

    static const char*		keyFor(const FontData&);
				/*!<Into a per-thread buffer. Valid until the
				    next call on this thread. */
    void			loadCB(CallBacker*);
    void			postAnnouncement();
    void			viewerChangedCB(CallBacker*);

    mutable Threads::Mutex	lock_;
    std::map<std::string,vsg::ref_ptr<vsg::Font>>	cache_;
    std::set<std::string>	loading_;
    TypeSet<FontData>		loaded_;
    ObjectSet<LoadJob>		jobs_;
    ObjectSet<Threads::Thread>	workers_;
    vsg::ref_ptr<vsgGeo::Updatable> announcer_;
};

} // namespace dGBVis
