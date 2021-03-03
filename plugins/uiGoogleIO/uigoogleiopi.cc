/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
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
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodmain.h"
#include "uiseis2dfileman.h"
#include "uisurveymanager.h"
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

uiString sKMLFileUiString()
{
    return od_static_tr( "uiGoogleExport", "KML Files" );
}

uiString sOutFileName()
{
    return od_static_tr("sOutFileName","Output File Name");
}

uiString sExportTypLbl()
{
    return od_static_tr("sOutFileName","Export Type");
}

mDefODPluginInfo( uiGoogleIO )
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Google KML",
	"OpendTect",
	"dGB",
	"=od",
	"Export to Google programs (Maps,Earth)."
	    "\nThis plugin adds functionality to generate KML files "
	    "from Opendtect."));
    return &retpi;
}


class uiGoogleIOMgr : public uiPluginInitMgr
{ mODTextTranslationClass(uiGoogleIOMgr);
public:

			uiGoogleIOMgr();
			~uiGoogleIOMgr();

    static void		exportSurv(CallBacker*);
    static uiString	sMenuTxt()
			{ return m3Dots(tr("Export to GIS Format")); }
    static uiString	sUtilTTText()
			{ return tr("Export to Google Earth/Maps"); }

private:

    uiSeis2DFileMan*	cur2dfm_ = nullptr;
    uiVisMenuItemHandler psmnuitmhandler_;
    uiVisMenuItemHandler rlmnuitmhandler_;


    void		exportWells(CallBacker*);
    void		exportLines(CallBacker*);
    void		exportPolygon(CallBacker*);
    void		exportRandLine(CallBacker*);
    void		mkExportWellsIcon(CallBacker*);
    void		mkExportLinesIcon(CallBacker*);

};


uiGoogleIOMgr::uiGoogleIOMgr()
    : uiPluginInitMgr()
    , psmnuitmhandler_(visSurvey::PickSetDisplay::sFactoryKeyword(),
		    *appl().applMgr().visServer(),
		   sMenuTxt(),
		    mCB(this,uiGoogleIOMgr,exportPolygon),0,cPSMnuIdx)
    , rlmnuitmhandler_(visSurvey::RandomTrackDisplay::sFactoryKeyword(),
			*appl().applMgr().visServer(),sMenuTxt(),
			mCB(this,uiGoogleIOMgr,exportRandLine),0,cRLMnuIdx)
{
    init();
    psmnuitmhandler_.setIcon( "google" );
    rlmnuitmhandler_.setIcon( "google" );
    mAttachCB( uiWellMan::instanceCreated(), uiGoogleIOMgr::mkExportWellsIcon );
    mAttachCB( uiSeis2DFileMan::instanceCreated(),
	       uiGoogleIOMgr::mkExportLinesIcon );
}


uiGoogleIOMgr::~uiGoogleIOMgr()
{
    detachAllNotifiers();
}


void uiGoogleIOMgr::exportSurv( CallBacker* cb )
{
    mDynamicCastGet(uiSurveyManager*,uisurv,cb)
    if ( !uisurv || !uisurv->curSurvInfo() ||
	 !uisurv->curSurvInfo()->getCoordSystem()->geographicTransformOK() )
	return;

    uiGISExportSurvey dlg( uisurv );
    dlg.go();
}


void uiGoogleIOMgr::mkExportWellsIcon( CallBacker* cb )
{
    mDynamicCastGet(uiWellMan*,wm,cb)
    if ( !wm ) return;

    new uiToolButton( wm->extraButtonGroup(), "google",
				sMenuTxt(),
				mCB(this,uiGoogleIOMgr,exportWells) );
}


void uiGoogleIOMgr::exportWells( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,tb,cb)
    if ( !tb || !SI().getCoordSystem()->geographicTransformOK() )
	return;

    uiGISExportWells dlg( tb->mainwin() );
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
    if ( !cur2dfm_ || !SI().getCoordSystem()->geographicTransformOK() )
	return;

    uiGISExport2DSeis dlg( cur2dfm_ );
    dlg.go();
}


void uiGoogleIOMgr::exportPolygon( CallBacker* cb )
{
    const int displayid = psmnuitmhandler_.getDisplayID();
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    appl_.applMgr().visServer()->getObject(displayid))
    if ( !psd || !psd->getSet() ) return;
    const Pick::Set& ps = *psd->getSet();
    if ( ps.connection() == Pick::Set::Disp::None )
	{ gUiMsg().error(tr("Can only export Polygons")); return; }
    if ( ps.size() < 3 )
	{ gUiMsg().error(tr("Polygon needs at least 3 points")); return; }

    if ( !SI().getCoordSystem()->geographicTransformOK() )
	return;

    uiGISExportPolygon dlg( &appl_, ps );
    dlg.go();
}


void uiGoogleIOMgr::exportRandLine( CallBacker* cb )
{
    const int displayid = rlmnuitmhandler_.getDisplayID();
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    appl_.applMgr().visServer()->getObject(displayid))
    if ( !rtd ) return;
    if ( rtd->nrNodes() < 2 )
	{ gUiMsg().error(tr("Need at least 2 points")); return; }

    TypeSet<BinID> knots;
    rtd->getAllNodePos( knots );

    if ( !SI().getCoordSystem()->geographicTransformOK() )
	return;

    TypeSet<Coord> crds;
    for ( int idx=0; idx<knots.size(); idx++ )
	crds += SI().transform( knots[idx] );

    uiGoogleExportRandomLine dlg( &appl_, crds, toUiString(rtd->name()) );
    dlg.go();
}


class uiGoogleIOSurveyManagerUtil : public uiSurveyManager::Util
{
public:

uiGoogleIOSurveyManagerUtil()
    : uiSurveyManager::Util( "google", uiGoogleIOMgr::sUtilTTText(),
			mSCB(uiGoogleIOMgr::exportSurv) )
{
}

virtual Util* clone() const
{
    return new uiGoogleIOSurveyManagerUtil( *this );
}

virtual bool willRunFor( const SurveyInfo& si ) const
{
    return si.getCoordSystem() ? si.getCoordSystem()->geographicTransformOK()
			       : false;
}

};


mDefODPluginSurvRelToolsLoadFn( uiGoogleIO )
{
    uiSurveyManager::add( uiGoogleIOSurveyManagerUtil() );
}


mDefODInitPlugin(uiGoogleIO)
{
    mDefineStaticLocalObject( PtrMan<uiGoogleIOMgr>, googleiomgr_,
				    = new uiGoogleIOMgr() );

    mCallODPluginSurvRelToolsLoadFn( uiGoogleIO );

    return nullptr;
}
