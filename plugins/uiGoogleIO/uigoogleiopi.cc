/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Jul 2007
-*/

static const char* rcsID = "$Id";

#include "uigoogleexpsurv.h"
#include "uigoogleexpwells.h"
#include "uigoogleexp2dlines.h"
#include "uigoogleexppolygon.h"
#include "uiodmain.h"
#include "uiodapplmgr.h"
#include "uisurvey.h"
#include "uimsg.h"
#include "uibutton.h"
#include "uiwellman.h"
#include "uiioobjmanip.h"
#include "uilatlong2coord.h"
#include "uiseis2dfileman.h"
#include "uivismenuitemhandler.h"
#include "uivispartserv.h"
#include "vispicksetdisplay.h"
#include "pickset.h"
#include "pixmap.h"
#include "survinfo.h"
#include "latlong.h"
#include "plugins.h"
static const int cPSMnuIdx = -1001;


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
    uiSeis2DFileMan*	cur2dfm_;
    uiVisMenuItemHandler psmnuitmhandler_;

    void		exportSurv(CallBacker*);
    void		exportWells(CallBacker*);
    void		exportLines(CallBacker*);
    void		exportPolygon(CallBacker*);
    void		mkExportWellsIcon(CallBacker*);
    void		mkExportLinesIcon(CallBacker*);
};


uiGoogleIOMgr::uiGoogleIOMgr( uiODMain& a )
    : appl_(a)
    , psmnuitmhandler_(visSurvey::PickSetDisplay::getStaticClassName(),
	    		*a.applMgr().visServer(),"Export to &Google KML ...",
    			mCB(this,uiGoogleIOMgr,exportPolygon),cPSMnuIdx)
{
    uiSurvey::add( uiSurvey::Util( "google.png",
				   "Export to Google Earth/Maps",
				   mCB(this,uiGoogleIOMgr,exportSurv) ) );
    uiWellMan::fieldsCreated()->notify(
				mCB(this,uiGoogleIOMgr,mkExportWellsIcon) );
    uiSeis2DFileMan::fieldsCreated()->notify(
				mCB(this,uiGoogleIOMgr,mkExportLinesIcon) );
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


void uiGoogleIOMgr::mkExportLinesIcon( CallBacker* cb )
{
    mDynamicCastGet(uiSeis2DFileMan*,fm,cb)
    cur2dfm_ = fm;
    if ( !cur2dfm_ ) return;

    fm->getButGroup(false)->addButton(	ioPixmap("google.png"),
	    				mCB(this,uiGoogleIOMgr,exportLines),
					"Export selected lines to Google KML" );
}


void uiGoogleIOMgr::exportLines( CallBacker* cb )
{
    if ( !cur2dfm_ || !uiLatLong2CoordDlg::ensureLatLongDefined(&appl_) )
	return;

    uiGoogleExport2DSeis dlg( cur2dfm_ );
    dlg.go();
}


void uiGoogleIOMgr::exportPolygon( CallBacker* cb )
{
    const int displayid = psmnuitmhandler_.getDisplayID();
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    appl_.applMgr().visServer()->getObject(displayid))
    if ( !psd || !psd->getSet() ) return;
    const Pick::Set& ps = *psd->getSet();
    if ( ps.disp_.connect_ == Pick::Set::Disp::None )
	{ uiMSG().error("Can only export Polygons" ); return; }
    if ( ps.size() < 3 )
	{ uiMSG().error("Polygon needs at least 3 points" ); return; }

    if ( !uiLatLong2CoordDlg::ensureLatLongDefined(&appl_) )
	return;

    uiGoogleExportPolygon dlg( &appl_, ps );
    dlg.go();
}


mExternC const char* InituiGoogleIOPlugin( int, char** )
{
    static uiGoogleIOMgr* mgr = 0;
    if ( !mgr )
	mgr = new uiGoogleIOMgr( *ODMainWin() );

    return 0;
}
