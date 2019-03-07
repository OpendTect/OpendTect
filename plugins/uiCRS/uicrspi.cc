/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : April 2017
-*/


#include "uicrssystem.h"

#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uiodmain.h"

#include "dbman.h"
#include "odplugin.h"
#include "survinfo.h"
#include "trckeyzsampling.h"


class uiCRSMgr : public CallBacker
{ mODTextTranslationClass(uiCRSMgr)
public:
		uiCRSMgr(uiODMain*);
		~uiCRSMgr();

    void	convertCB(CallBacker*);
    void	survChg(CallBacker*);

    uiODMain*	appl_;
    Coords::uiConvertGeographicPos*	convdlg_;
};


uiCRSMgr::uiCRSMgr( uiODMain* appl )
    : appl_(appl)
    , convdlg_(nullptr)
{
    uiAction* act = new uiAction( tr("CRS Position Conversion ...") );
    mAttachCB( act->triggered, uiCRSMgr::convertCB );
    uiAction* prevact =
	appl_->menuMgr().toolsMnu()->findAction( "Show Log File" );
    appl_->menuMgr().toolsMnu()->insertAction( act, -1, prevact );

    mAttachCB( DBM().surveyToBeChanged, uiCRSMgr::survChg );
}


uiCRSMgr::~uiCRSMgr()
{
    detachAllNotifiers();
    deleteAndZeroPtr( convdlg_ );
}


void uiCRSMgr::survChg( CallBacker* )
{
    deleteAndZeroPtr( convdlg_ );
}


void uiCRSMgr::convertCB( CallBacker* )
{
    ConstRefMan<Coords::CoordSystem> crs = SI().getCoordSystem();
    TrcKeyZSampling survtkzs;
    SI().getSampling( survtkzs, OD::UsrWork );
    const Coord centerpos = survtkzs.hsamp_.center().getCoord();
    if ( !convdlg_ )
	convdlg_ = new Coords::uiConvertGeographicPos( appl_, crs, centerpos );
    convdlg_->show();
}


mDefODInitPlugin(uiCRS)
{
    mDefineStaticLocalObject( PtrMan<uiCRSMgr>, theinst_, = nullptr );
    if ( theinst_ ) return nullptr;

    theinst_ = new uiCRSMgr( ODMainWin() );
    if ( !theinst_ )
	return nullptr;

    Coords::uiProjectionBasedSystem::initClass();
    return nullptr;
}


mDefODPluginInfo(uiCRS)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"CRS: Coordinate Reference Systems - powered by PROJ.4",
	mODCRSPluginPackage,
	mODPluginCreator, mODPluginVersion,
	"Provides support for Coordinate Reference Systems "
		    "using the PROJ.4 services" ) );
    retpi.useronoffselectable_ = false;
    retpi.url_ = "proj4.org";
    mSetPackageDisplayName( retpi,
			    Coords::uiProjectionBasedSystem::pkgDispNm() );
    retpi.uidispname_ = retpi.uipackagename_;
    return &retpi;
}

mDefODPluginSurvRelToolsLoadFn(uiCRS)
{
    Coords::uiProjectionBasedSystem::initClass();
}
