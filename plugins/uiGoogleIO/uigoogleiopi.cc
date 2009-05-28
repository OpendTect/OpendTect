/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert Bril
 * DATE     : Jul 2007
-*/

static const char* rcsID = "$Id";

#include "uigoogleexpsurv.h"
#include "uiodmain.h"
#include "uisurvey.h"
#include "uilatlong2coord.h"
#include "uimsg.h"
#include "survinfo.h"
#include "latlong.h"
#include "plugins.h"

mExternC int GetuiGoogleIOPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC PluginInfo* GetuiGoogleIOPluginInfo()
{
    static PluginInfo retpi = {
	"Google I/O",
	"dGB",
	"=od",
	"IO with Google programs (Maps,Earth)." };
    return &retpi;
}


class uiGoogleIOMgr : public CallBacker
{
public:

    			uiGoogleIOMgr(uiODMain&);

    uiODMain&		appl_;

    void		exportSurv(CallBacker*);
};


uiGoogleIOMgr::uiGoogleIOMgr( uiODMain& a )
    : appl_(a)
{
    uiSurvey::add( uiSurvey::Util( "google.png",
				   "Export to Google Earth/Maps",
				   mCB(this,uiGoogleIOMgr,exportSurv) ) );
}


void uiGoogleIOMgr::exportSurv( CallBacker* cb )
{
    mDynamicCastGet(uiSurvey*,uisurv,cb)
    if ( !uisurv
    || !uiLatLong2CoordDlg::ensureLatLongDefined(uisurv,uisurv->curSurvInfo()) )
	return;

    uiGoogleExportSurvey dlg( uisurv );
    dlg.go();
}


mExternC const char* InituiGoogleIOPlugin( int, char** )
{
    static uiGoogleIOMgr* mgr = 0;
    if ( !mgr )
	mgr = new uiGoogleIOMgr( *ODMainWin() );

    return 0;
}
