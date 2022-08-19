/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "qtclss.h"

#include "uimenu.h"
#include "uiodmenumgr.h"

#include "odplugin.h"


mDefODPluginInfo(uiQtApp)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"QT Application plugin (GUI)",
	"OpendTect",
	"dGB Earth Sciences (Bert)",
	"=od",
	"Example of how to call a Qt something from a plugin." ))
    return &retpi;
}


class uiQtAppMgr : public uiPluginInitMgr
{
public:
			uiQtAppMgr();

private:

    QtClss*		qtclss_ = nullptr;

    void		dTectMenuChanged() override;
    void		cleanup() override;

    void		doStuff(CallBacker*);
};


uiQtAppMgr::uiQtAppMgr()
    : uiPluginInitMgr()
{
    init();
}


void uiQtAppMgr::dTectMenuChanged()
{
    appl().menuMgr().utilMnu()->insertAction(
	new uiAction( toUiString("Qt Thing"), mCB(this,uiQtAppMgr,doStuff) ) );
}


void uiQtAppMgr::doStuff( CallBacker* )
{
    delete qtclss_;
    qtclss_ = new QtClss( appl().qWidget() );
    qtclss_->go();
}


void uiQtAppMgr::cleanup()
{
    qtclss_ = nullptr;
    uiPluginInitMgr::cleanup();
}


mDefODInitPlugin(uiQtApp)
{
    mDefineStaticLocalObject( PtrMan<uiQtAppMgr>, theinst_,
		    = new uiQtAppMgr() );

    if ( !theinst_ )
	return "Cannot instantiate the QtApp plugin";

    return nullptr;
}
