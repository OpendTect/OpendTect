/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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

    OS::MachineCommand machcomm( scriptfnm, true );
    if ( !machcomm.execute(OS::RunInBG) )
    {
	uiMSG().error( mINTERNAL("Cannot start COLOP app") );
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
