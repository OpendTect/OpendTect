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

#include "ioman.h"
#include "odplugin.h"
#include "survinfo.h"


mDefODPluginInfo(uiCRS)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Coordinate Reference System",
	"OpendTect",
	"dGB (Raman)",
	"6.2",
	"User interface for providing a library of Coordinate Reference Systems"
	    " that can be set at Survey level" ));
    return &retpi;
}


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
    , convdlg_(0)
{
    uiAction* act = new uiAction( tr("CRS Position Conversion ...") );
    mAttachCB( act->triggered, uiCRSMgr::convertCB );
    uiAction* prevact =
	appl_->menuMgr().toolsMnu()->findAction( "Command Driver" );
    appl_->menuMgr().toolsMnu()->insertAction( act, -1, prevact );

    mAttachCB( IOM().surveyToBeChanged, uiCRSMgr::survChg );
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
    const TrcKeyZSampling survtkzs = SI().sampling( true );
    const Coord centerpos = survtkzs.hsamp_.center().getCoord();
    if ( !convdlg_ )
	convdlg_ = new Coords::uiConvertGeographicPos( appl_, crs, centerpos );
    convdlg_->show();
}



mDefODInitPlugin(uiCRS)
{
    mDefineStaticLocalObject( PtrMan<uiCRSMgr>, theinst_, = 0 );
    if ( theinst_ ) return 0;

    theinst_ = new uiCRSMgr( ODMainWin() );

    Coords::uiProjectionBasedSystem::initClass();
    return 0;
}
