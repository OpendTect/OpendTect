
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Jan 2009
-*/

#include "qtclss.h"

#include "uimenu.h"
#include "uiodmenumgr.h"

#include "odplugin.h"


mDefODPluginInfo(uiQtApp)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"QT Application plugin",
	mODPluginTutorialsPackage,
	mODPluginCreator, mODPluginVersion,
	"Example of how to use something from Qt from a plugin.") );
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
    qtclss_ = new QtClss( appl().getWidget(0) );
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
	return "Cannot instantiate QtApp plugin";

    return nullptr;
}
