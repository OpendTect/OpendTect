/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Khushnood Qadir
 * DATE     : September 2019
-*/
#include "uicolopmod.h"

#include "file.h"
#include "filepath.h"
#include "uiodapplmgr.h"
#include "uibutton.h"
#include "uiioobjmanip.h"
#include "uiioobjselgrp.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uiseiswvltman.h"
#include "uitoolbar.h"

#include "odplugin.h"
#include "oddirs.h"
#include "oscommand.h"


mDefODPluginInfo(uiCOLOP)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"COLOP (GUI)",
	"OpendTect",
	"Peter Zahuczki",
	"=od",
	"A link to the COLOP tool.\n"
	    "This is the User interface of the link. See\n"
	    "http://peter.zahuczki.hu/index.php/myapps/clp"
	    "\nfor info on COLOP" ));
    return &retpi;
}


class uiColopLink : public uiPluginInitMgr
{ mODTextTranslationClass(uiColopLink);
public:
			uiColopLink();
			~uiColopLink();

private:

    void		dTectMenuChanged() override;

    void		doColop(CallBacker*);
    void		updateWaveletMan(CallBacker*);
};


uiColopLink::uiColopLink()
    : uiPluginInitMgr()
{
    init();
    mAttachCB( uiSeisWvltMan::instanceCreated(), uiColopLink::updateWaveletMan);
    updateWaveletMan(nullptr);
}


uiColopLink::~uiColopLink()
{
    detachAllNotifiers();
}


void uiColopLink::dTectMenuChanged()
{
    appl().menuMgr().procMnu()->insertAction(
			    new uiAction( m3Dots(tr("COLOP")),
					  mCB(this,uiColopLink,doColop) ) );
}


void uiColopLink::updateWaveletMan( CallBacker* cb )
{
    mDynamicCastGet(uiSeisWvltMan*,swm,cb)
    if ( !swm ) return;

    new uiToolButton( swm->selGroup()->getManipGroup(),
		       "COLOP", toUiString("COLOP"),
		       mCB(this,uiColopLink,doColop) );
}


void uiColopLink::doColop( CallBacker* )
{
    File::Path fp( BufferString(GetExecPlfDir()), "COLOP" );
#ifdef __win__
    fp.setExtension( "exe" );
#endif
    BufferString scriptfnm( fp.fullPath() );
    if ( !File::exists(scriptfnm) )
    {
	const uiString fileerrstr = tr( "%1%2" ).arg( uiStrings::sFileName() )
				   .arg( scriptfnm );
	gUiMsg(nullptr).error( uiStrings::phrCannotFind(fileerrstr) );
	return;
    }

    OS::MachineCommand machcomm( scriptfnm, true );
    OS::CommandLauncher cl( machcomm );
    if ( !cl.startServer(333) )
    {
	gUiMsg(nullptr).error( cl.errorMsg() );
	return;
    }
}


mDefODInitPlugin(uiCOLOP)
{
    mDefineStaticLocalObject( PtrMan<uiColopLink>, theinst_,
			      = new uiColopLink() );
    if ( !theinst_ )
	return "Cannot instantiate uiColop plugin";

    return nullptr;
}
