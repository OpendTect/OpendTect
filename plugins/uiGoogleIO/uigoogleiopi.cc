/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Jul 2007
-*/



#include "uigoogleiomod.h"
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
#include "uipicksetman.h"
#include "uirandomlineman.h"
#include "uiseis2dfileman.h"
#include "uisurvey.h"
#include "uitoolbutton.h"
#include "uivismenuitemhandler.h"
#include "uivispartserv.h"
#include "uiwellman.h"
#include "vispicksetdisplay.h"
#include "visrandomtrackdisplay.h"
#include "visseis2ddisplay.h"
#include "viswelldisplay.h"
#include "welldata.h"
#include "wellman.h"

#include "coordsystem.h"
#include "odplugin.h"
#include "pickset.h"
#include "survinfo.h"

static const int cPSMnuIdx = -995;
static const int cRLMnuIdx = -995;
static const int cWLMnuIdx = -995;
static const int cSeis2DMnuIdx = -995;


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
    uiVisMenuItemHandler wlmnuitmhandler_;
    uiVisMenuItemHandler ln2duitmhandler_;

    void		exportSurv(CallBacker*);
    void		exportManWells(CallBacker*);
    void		exportManLines(CallBacker*);
    void		exportPointPolygon(CallBacker*);
    void		expManPointPoly(CallBacker*);
    void		exportRandLine(CallBacker*);
    void		expManRandomLine(CallBacker*);
    void		exportWell(CallBacker*);
    void		mkExportWellsIcon(CallBacker*);
    void		mkExportPointPolyIcon(CallBacker*);
    void		mkExportRandomLine(CallBacker*);
    void		mkExportLinesIcon(CallBacker*);
    void		export2DSeisLine(CallBacker*);

    static uiString	sToolTipTxt()
			{ return tr("Export to GIS Format"); }
    static uiString	sMenuTxt()
			{ return m3Dots(sToolTipTxt()); }
    static BufferString strIcon()   { return BufferString( "google" ); }
};


uiGoogleIOMgr::uiGoogleIOMgr()
    : uiPluginInitMgr()
    , psmnuitmhandler_(visSurvey::PickSetDisplay::sFactoryKeyword(),
			*appl().applMgr().visServer(), sMenuTxt(),
			mCB(this,uiGoogleIOMgr,exportPointPolygon),0,cPSMnuIdx)
    , rlmnuitmhandler_(visSurvey::RandomTrackDisplay::sFactoryKeyword(),
			*appl().applMgr().visServer(),sMenuTxt(),
			mCB(this,uiGoogleIOMgr,exportRandLine),0,cRLMnuIdx)
    , wlmnuitmhandler_(visSurvey::WellDisplay::sFactoryKeyword(),
			*appl().applMgr().visServer(),sMenuTxt(),
			mCB(this,uiGoogleIOMgr,exportWell),0,cWLMnuIdx)
    , ln2duitmhandler_(visSurvey::Seis2DDisplay::sFactoryKeyword(),
			*appl().applMgr().visServer(), sMenuTxt(),
		    mCB(this,uiGoogleIOMgr,export2DSeisLine),0,cSeis2DMnuIdx)
{
    init();
    psmnuitmhandler_.setIcon( strIcon() );
    rlmnuitmhandler_.setIcon( strIcon() );
    wlmnuitmhandler_.setIcon( strIcon() );
    ln2duitmhandler_.setIcon( strIcon() );
    uiSurvey::add( uiSurvey::Util( strIcon(),
				   tr("Export to GIS Format"),
				   mCB(this,uiGoogleIOMgr,exportSurv) ) );
    mAttachCB(uiWellMan::instanceCreated(),uiGoogleIOMgr::mkExportWellsIcon);
    mAttachCB(uiPickSetMan::instanceCreated(),
					uiGoogleIOMgr::mkExportPointPolyIcon);
    mAttachCB(uiSeis2DFileMan::instanceCreated(),
					    uiGoogleIOMgr::mkExportLinesIcon);
    mAttachCB(uiRandomLineMan::instanceCreated(),
					    uiGoogleIOMgr::mkExportRandomLine);
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

    uiGISExportSurvey dlg( &appl_, uisurv->curSurvInfo() );
    dlg.go();
}


void uiGoogleIOMgr::mkExportWellsIcon( CallBacker* cb )
{
    mDynamicCastGet(uiWellMan*,wm,cb)
    if ( !wm )
	return;

    new uiToolButton( wm->extraButtonGroup(), strIcon(),
		      sToolTipTxt(),
		      mCB(this,uiGoogleIOMgr,exportManWells) );
}


void uiGoogleIOMgr::mkExportPointPolyIcon( CallBacker* cb )
{
    mDynamicCastGet(uiPickSetMan*,picksetman,cb)
    if ( !picksetman )
	return;

    new uiToolButton( picksetman->extraButtonGroup(), strIcon(), sToolTipTxt(),
	mCB(this,uiGoogleIOMgr,expManPointPoly) );
}

void uiGoogleIOMgr::mkExportRandomLine( CallBacker* cb )
{
    mDynamicCastGet(uiRandomLineMan*,rlman,cb)
    if ( !rlman )
	return;

    new uiToolButton( rlman->extraButtonGroup(), strIcon(), sToolTipTxt(),
			mCB(this,uiGoogleIOMgr,expManRandomLine) );
}


void uiGoogleIOMgr::exportManWells( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,tb,cb)
    if ( !tb || !mEnsureTransformOK(tb->mainwin(),nullptr) )
	return;

    uiGISExportWells dlg( tb->mainwin() );
    dlg.go();
}


void uiGoogleIOMgr::mkExportLinesIcon( CallBacker* cb )
{
    mDynamicCastGet(uiSeis2DFileMan*,fm,cb)
    cur2dfm_ = fm;
    if ( !cur2dfm_ )
	return;

    fm->getButGroup(false)->addButton( strIcon(),
			tr("Export selected lines to Google KML"),
			mCB(this,uiGoogleIOMgr,exportManLines) );
}


void uiGoogleIOMgr::exportManLines( CallBacker* )
{
    if ( !cur2dfm_ || !mEnsureTransformOK(cur2dfm_,nullptr) )
	return;

    uiGISExport2DSeis dlg( &appl_, cur2dfm_ );
    dlg.go();
}


void uiGoogleIOMgr::export2DSeisLine( CallBacker* )
{
    const int displayid = ln2duitmhandler_.getDisplayID();
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2dd,
		    appl_.applMgr().visServer()->getObject(displayid))
    if ( !s2dd )
	return;

    uiGISExport2DSeis dlg( &appl_, nullptr, s2dd->name() );
    dlg.go();
}


void uiGoogleIOMgr::expManPointPoly( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,tb,cb)
    if ( !tb || !mEnsureTransformOK(tb->mainwin(),nullptr) )
	return;

    uiGISExportPolygon dlg( tb->mainwin() );
    dlg.go();
}


void uiGoogleIOMgr::expManRandomLine( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,tb,cb)
    if ( !tb || !mEnsureTransformOK(tb->mainwin(),nullptr) )
	return;

    uiGISExportRandomLine dlg( tb->mainwin() );
    dlg.go();
}


void uiGoogleIOMgr::exportPointPolygon( CallBacker* cb )
{
    const int displayid = psmnuitmhandler_.getDisplayID();
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    appl_.applMgr().visServer()->getObject(displayid))
    if ( !psd || !psd->getSet() )
	return;

    uiGISExportPolygon dlg( &appl_, psd->getMultiID() );
    dlg.go();
}


void uiGoogleIOMgr::exportRandLine( CallBacker* cb )
{
    const int displayid = rlmnuitmhandler_.getDisplayID();
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    appl_.applMgr().visServer()->getObject(displayid))
    if ( !rtd )
	return;

    if ( rtd->nrNodes() < 2 )
    {
	uiMSG().error( tr("Need at least 2 points") );
	return;
    }

    TrcKeyPath nodes;
    rtd->getAllNodePos( nodes );

    if ( !mEnsureTransformOK(&appl_,0) )
	return;

    TypeSet<Coord> crds;
    for ( const auto& node : nodes )
	crds += node.getCoord();

    uiGISExportRandomLine dlg( &appl_, &crds, rtd->name() );
    dlg.go();
}


void uiGoogleIOMgr::exportWell( CallBacker* cb )
{
    const int displayid = wlmnuitmhandler_.getDisplayID();
    mDynamicCastGet(visSurvey::WellDisplay*,wld,
		    appl_.applMgr().visServer()->getObject(displayid))
    if ( !wld )
	return;


   uiGISExportWells dlg( &appl_, wld->getMultiID() );
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
