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
	    "<a href=\"http://peter.zahuczki.hu/index.php/myapps/clp\">"
	    "\nfor info on COLOP" ));
    return &retpi;
}


class uiColopLink : public CallBacker
{ mODTextTranslationClass(uiColopLink);
public:
			uiColopLink(uiODMain&);
			~uiColopLink();

    uiODMain&		appl_;
    uiODMenuMgr&	mnumgr;

    void		doColop(CallBacker*);
    void		updateMenu(CallBacker*);
    void		updateWaveletMan(CallBacker*);
};


uiColopLink::uiColopLink( uiODMain& a )
    : mnumgr(a.menuMgr())
    , appl_(a)
{
    mAttachCB( mnumgr.dTectMnuChanged, uiColopLink::updateMenu );
    mAttachCB( uiSeisWvltMan::instanceCreated(),
				 uiColopLink::updateWaveletMan );
    updateMenu(0);
    updateWaveletMan(0);
}


uiColopLink::~uiColopLink()
{
    detachAllNotifiers();
}


void uiColopLink::updateMenu( CallBacker* )
{
    uiAction* newitem = new uiAction( m3Dots(tr("COLOP")),
				mCB(this,uiColopLink,doColop) );
    appl_.menuMgr().procMnu()->insertItem( newitem );
}


void uiColopLink::updateWaveletMan( CallBacker* cb )
{
    mDynamicCastGet(uiSeisWvltMan*,swm,cb)
    if ( !swm ) return;

    new uiToolButton( swm->selGroup()->getManipGroup(), "COLOP",
				"COLOP",
				mCB(this,uiColopLink,doColop) );
}


void uiColopLink::doColop( CallBacker* )
{
    FilePath fp( GetExecPlfDir(), "COLOP" );
#ifdef __win__
    fp.setExtension( "exe" );
#endif
    BufferString scriptfnm( fp.fullPath() );
    if ( !File::exists(scriptfnm) )
    {
	uiMSG().error( mINTERNAL("COLOP.exe not found") );
	return;
    }

    OS::MachineCommand machcomm( scriptfnm );
    OS::CommandExecPars execpars( OS::RunInBG );
    OS::CommandLauncher cl( machcomm, true );
    if ( !cl.execute(execpars) )
    {
	uiMSG().error( mINTERNAL("Cannot start COLOP app") );
	return;
    }
}


mDefODInitPlugin(uiCOLOP)
{
    mDefineStaticLocalObject( PtrMan<uiColopLink>, theinst_,
			      = new uiColopLink(*ODMainWin()) );
    return nullptr;
}
