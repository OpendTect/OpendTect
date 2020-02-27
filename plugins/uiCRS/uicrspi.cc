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


mDefODPluginInfo(uiCRS)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Coordinate Reference System (GUI)",
	"OpendTect",
	"dGB (Raman Singh)",
	"=od",
	"User interface for providing a library of Coordinate Reference Systems"
	    " that can be set at Survey level" ));
    return &retpi;
}



class uiCRSMgr : public uiPluginInitMgr
{ mODTextTranslationClass(uiCRSMgr)
public:
		uiCRSMgr();
		~uiCRSMgr();

private:

    void	beforeSurveyChange() override;
    void	dTectMenuChanged() override;
    void	convertCB(CallBacker*);

    Coords::uiConvertGeographicPos*	convdlg_ = nullptr;
};


uiCRSMgr::uiCRSMgr()
    : uiPluginInitMgr()
{
    init();
}


uiCRSMgr::~uiCRSMgr()
{
    detachAllNotifiers();
    deleteAndZeroPtr( convdlg_ );
}


void uiCRSMgr::dTectMenuChanged()
{
    uiAction* act = new uiAction( tr("CRS Position Conversion ...") );
    mAttachCB( act->triggered, uiCRSMgr::convertCB );
    uiAction* prevact =
	appl_.menuMgr().toolsMnu()->findAction( "Show Log File" );
    appl_.menuMgr().toolsMnu()->insertAction( act, -1, prevact );
}


void uiCRSMgr::beforeSurveyChange()
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
	convdlg_ = new Coords::uiConvertGeographicPos( &appl_, crs, centerpos );
    convdlg_->show();
}


mDefODInitPlugin(uiCRS)
{
    mDefineStaticLocalObject( PtrMan<uiCRSMgr>, theinst_, = new uiCRSMgr() );

    Coords::uiProjectionBasedSystem::initClass();

    return nullptr;
}



mDefODPluginSurvRelToolsLoadFn(uiCRS)
{
    Coords::uiProjectionBasedSystem::initClass();
}
