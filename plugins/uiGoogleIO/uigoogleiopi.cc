/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Jul 2007
-*/



#include "uigoogleiomod.h"
#include "uigoogleexppointset.h"
#include "uigoogleexppolygon.h"
#include "uigoogleexprandline.h"
#include "uigoogleexpsurv.h"
#include "uigoogleexpwells.h"
#include "uigoogleexp2dlines.h"

#include "uibuttongroup.h"
#include "uiioobjmanip.h"
#include "uicoordsystem.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodmain.h"
#include "uiseis2dfileman.h"
#include "uisurvey.h"
#include "uitoolbutton.h"
#include "uivismenuitemhandler.h"
#include "uivispartserv.h"
#include "uiwellman.h"
#include "vispicksetdisplay.h"
#include "visrandomtrackdisplay.h"

#include "coordsystem.h"
#include "odplugin.h"
#include "pickset.h"
#include "survinfo.h"

static const int cPSMnuIdx = -995;
static const int cRLMnuIdx = -995;


mDefODPluginInfo(uiGoogleIO)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"Google KML Support (GUI)",
	"OpendTect",
	"dGB Earth Sciences",
	"=od",
	"Export to Google programs (Maps,Earth)."
	    "\nThis plugin adds functionality to generate KML files "
	    "from Opendtect." ))
    return &retpi;
}


class uiGoogleIOMgr : public uiPluginInitMgr
{ mODTextTranslationClass(uiGoogleIOMgr);
public:

			uiGoogleIOMgr();
			~uiGoogleIOMgr();

private:

    uiSeis2DFileMan*	cur2dfm_ = nullptr;
    uiVisMenuItemHandler psmnuitmhandler_;
    uiVisMenuItemHandler rlmnuitmhandler_;

    void		exportSurv(CallBacker*);
    void		exportWells(CallBacker*);
    void		exportLines(CallBacker*);
    void		exportPolygon(CallBacker*);
    void		exportRandLine(CallBacker*);
    void		mkExportWellsIcon(CallBacker*);
    void		mkExportLinesIcon(CallBacker*);

    static uiString	sToolTipTxt()
			{ return tr("Export to Google KML"); }
    static uiString	sMenuTxt()
			{ return m3Dots(sToolTipTxt()); }
};


uiGoogleIOMgr::uiGoogleIOMgr()
    : uiPluginInitMgr()
    , psmnuitmhandler_( visSurvey::PickSetDisplay::sFactoryKeyword(),
			*appl().applMgr().visServer(), sMenuTxt(),
			mCB(this,uiGoogleIOMgr,exportPolygon),0,cPSMnuIdx)
    , rlmnuitmhandler_(visSurvey::RandomTrackDisplay::sFactoryKeyword(),
			*appl().applMgr().visServer(),sMenuTxt(),
			mCB(this,uiGoogleIOMgr,exportRandLine),0,cRLMnuIdx)
{
    init();
    psmnuitmhandler_.setIcon( "google" );
    rlmnuitmhandler_.setIcon( "google" );
    uiSurvey::add( uiSurvey::Util( "google",
				   tr("Export to Google Earth/Maps"),
				   mCB(this,uiGoogleIOMgr,exportSurv) ) );
    mAttachCB( uiWellMan::instanceCreated(), uiGoogleIOMgr::mkExportWellsIcon );
    mAttachCB( uiSeis2DFileMan::instanceCreated(),
	       uiGoogleIOMgr::mkExportLinesIcon );
}


uiGoogleIOMgr::~uiGoogleIOMgr()
{
    detachAllNotifiers();
}

#define mEnsureTransformOK(p,si) \
    Coords::uiCoordSystemDlg::ensureGeographicTransformOK(p,si)

void uiGoogleIOMgr::exportSurv( CallBacker* cb )
{
    mDynamicCastGet(uiSurvey*,uisurv,cb)
    if ( !uisurv )
	return;

    SurveyInfo* si = uisurv->curSurvInfo();
    if ( !si || !mEnsureTransformOK(uisurv,si) )
	return;

    uiGoogleExportSurvey dlg( uisurv );
    dlg.go();
}


void uiGoogleIOMgr::mkExportWellsIcon( CallBacker* cb )
{
    mDynamicCastGet(uiWellMan*,wm,cb)
    if ( !wm ) return;

    new uiToolButton( wm->extraButtonGroup(), "google",
		      sToolTipTxt(),
		      mCB(this,uiGoogleIOMgr,exportWells) );
}


void uiGoogleIOMgr::exportWells( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,tb,cb)
    if ( !tb || !mEnsureTransformOK(tb->mainwin(),0) )
	return;

    uiGoogleExportWells dlg( tb->mainwin() );
    dlg.go();
}


void uiGoogleIOMgr::mkExportLinesIcon( CallBacker* cb )
{
    mDynamicCastGet(uiSeis2DFileMan*,fm,cb)
    cur2dfm_ = fm;
    if ( !cur2dfm_ ) return;

    fm->getButGroup(false)->addButton( "google",
			tr("Export selected lines to Google KML"),
			mCB(this,uiGoogleIOMgr,exportLines) );
}


void uiGoogleIOMgr::exportLines( CallBacker* cb )
{
    if ( !cur2dfm_ || !mEnsureTransformOK(cur2dfm_,0) )
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
    {
	if ( !mEnsureTransformOK(&appl_,0) )
	    return;

	uiGoogleExportPointSet dlg( &appl_, ps );
	dlg.go();
    }
    else
    {
	if ( ps.size() < 3 )
	    { uiMSG().error(tr("Polygon needs at least 3 points")); return; }

	if ( !mEnsureTransformOK(&appl_,0) )
	    return;

	uiGoogleExportPolygon dlg( &appl_, ps );
	dlg.go();
    }
}


void uiGoogleIOMgr::exportRandLine( CallBacker* cb )
{
    const int displayid = rlmnuitmhandler_.getDisplayID();
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    appl_.applMgr().visServer()->getObject(displayid))
    if ( !rtd ) return;
    if ( rtd->nrNodes() < 2 )
	{ uiMSG().error(tr("Need at least 2 points")); return; }

    TypeSet<BinID> knots;
    rtd->getAllNodePos( knots );

    if ( !mEnsureTransformOK(&appl_,0) )
	return;

    TypeSet<Coord> crds;
    for ( int idx=0; idx<knots.size(); idx++ )
	crds += SI().transform( knots[idx] );

    uiGoogleExportRandomLine dlg( &appl_, crds, rtd->name() );
    dlg.go();
}


mDefODInitPlugin(uiGoogleIO)
{
    mDefineStaticLocalObject( PtrMan<uiGoogleIOMgr>, theinst_,
		= new uiGoogleIOMgr() );
    if ( !theinst_ )
	return "Cannot instantiate GoogleIO plugin";

    return nullptr;
}
