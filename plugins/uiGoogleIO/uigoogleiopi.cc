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


static bool prepLatLong( uiParent* p, SurveyInfo* si )
{
    if ( !si ) return false;
    if ( si->latlong2Coord().isOK() ) return true;

    uiLatLong2CoordDlg dlg( p, si->latlong2Coord(), si );
    if ( !dlg.go() ) return false;

    si->getLatlong2Coord() = dlg.ll2C();
    if ( !si->latlong2Coord().isOK() )
    {
	uiMSG().error( "Sorry, your Lat/Long definition has a problem" );
	return false;
    }
    if ( !si->write() )
    {
	uiMSG().error( "Could not write the definitions to your '.survey' file"
		    "\nThe definition will work this OpendTect session only" );
	return false;
    }

    return true;
}


void uiGoogleIOMgr::exportSurv( CallBacker* cb )
{
    mDynamicCastGet(uiSurvey*,uisurv,cb)
    if ( !uisurv || !prepLatLong(uisurv,uisurv->curSurvInfo()) )
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
