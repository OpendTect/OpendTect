
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2018
-*/


#include "uihdf5settings.h"
#include "hdf5access.h"
#include "odplugin.h"



class uiHDF5PIMgr	: public CallBacker
{ mODTextTranslationClass(uiHDF5PIMgr)
public:

				uiHDF5PIMgr();

    void			settDlgLaunchCB(CallBacker*);

};


uiHDF5PIMgr::uiHDF5PIMgr()
{
    mAttachCB( uiSettingsDlg::instanceCreated(), uiHDF5PIMgr::settDlgLaunchCB );
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


mDefODPluginInfo(uiHDF5)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"HDF5 Support", "HDF5 File Access",
	mODPluginCreator, mODPluginVersion,
	"Adds HDF5-based data storage for various data types") );
    retpi.useronoffselectable_ = true;
    retpi.url_ = "hdfgroup.org";
    mSetPackageDisplayName( retpi, HDF5::Access::sHDF5PackageDispName() );
    retpi.uidispname_ = retpi.uipackagename_;
    return &retpi;
}


mDefODInitPlugin(uiHDF5)
{
    mDefineStaticLocalObject( uiHDF5PIMgr*, mgr, = 0 );
    if ( mgr )
	return 0;
    mgr = new uiHDF5PIMgr;

    return 0;
}
