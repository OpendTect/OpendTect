
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2018
-*/


#include "uihdf5settings.h"
#include "hdf5access.h"
#include "odplugin.h"


mDefODPluginInfo(uiODHDF5)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"HDF5 Link (GUI)",
	"OpendTect",
	"dGB (Bert Bril)",
	"=od",
	"Adds HDF5-based data storage for various data types") );
    return &retpi;
}


class uiHDF5PIMgr : public CallBacker
{ mODTextTranslationClass(uiHDF5PIMgr)
public:

				uiHDF5PIMgr();
				~uiHDF5PIMgr();

    void			settDlgLaunchCB(CallBacker*);

};


uiHDF5PIMgr::uiHDF5PIMgr()
{
    mAttachCB( uiSettingsDlg::instanceCreated(), uiHDF5PIMgr::settDlgLaunchCB );
}


uiHDF5PIMgr::~uiHDF5PIMgr()
{
    detachAllNotifiers();
}


void uiHDF5PIMgr::settDlgLaunchCB( CallBacker* cb )
{
    mDynamicCastGet( uiSettingsDlg*, dlg, cb )
    if ( !dlg )
	{ pErrMsg("Huh"); return; }

    uiSettingsGroup* settgrp
	= dlg->getGroup( uiStorageSettingsGroup::sFactoryKeyword() );
    mDynamicCastGet( uiStorageSettingsGroup*, ssgrp, settgrp )
    if ( !ssgrp )
	{ pErrMsg("Huh?"); return; }

    (void)new uiHDF5Settings( *ssgrp );
}



mDefODInitPlugin(uiODHDF5)
{
    mDefineStaticLocalObject( PtrMan<uiHDF5PIMgr>, hdf5mgr,= new uiHDF5PIMgr());

    return nullptr;
}
