/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Jul 2007
-*/

static const char* rcsID = "$Id";

#include "uigoogleexpsurv.h"
#include "uigoogleexpwells.h"
#include "uiodmain.h"
#include "uisurvey.h"
#include "uilatlong2coord.h"
#include "uibutton.h"
#include "uiwellman.h"
#include "uimsg.h"
#include "pixmap.h"
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
    void		exportWells(CallBacker*);
    void		mkExportWellsIcon(CallBacker*);
};


uiGoogleIOMgr::uiGoogleIOMgr( uiODMain& a )
    : appl_(a)
{
    uiSurvey::add( uiSurvey::Util( "google.png",
				   "Export to Google Earth/Maps",
				   mCB(this,uiGoogleIOMgr,exportSurv) ) );
    uiWellMan::fieldsCreated()->notify(
				mCB(this,uiGoogleIOMgr,mkExportWellsIcon) );
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


void uiGoogleIOMgr::mkExportWellsIcon( CallBacker* cb )
{
    mDynamicCastGet(uiWellMan*,wm,cb)
    if ( !wm ) return;

    uiToolButton* tb = new uiToolButton( wm, "Google", ioPixmap("google.png"),
				mCB(this,uiGoogleIOMgr,exportWells) );
    tb->setToolTip( "Export to Google KML" );
    wm->addTool( tb );
}


void uiGoogleIOMgr::exportWells( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,tb,cb)
    mDynamicCastGet(uiWellMan*,wm,tb->parent())
    if ( !wm || !uiLatLong2CoordDlg::ensureLatLongDefined(&appl_) )
	return;

    uiGoogleExportWells dlg( wm );
    dlg.go();
}


mExternC const char* InituiGoogleIOPlugin( int, char** )
{
    static uiGoogleIOMgr* mgr = 0;
    if ( !mgr )
	mgr = new uiGoogleIOMgr( *ODMainWin() );

    return 0;
}
