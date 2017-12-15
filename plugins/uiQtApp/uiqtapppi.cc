
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2009
-*/

static const char* rcsID mUsedVar = "$Id$";

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


class uiQtAppMgr :  public CallBacker
{
public:
			uiQtAppMgr(uiODMain*);

    uiODMain*		appl_;
    QtClss*		qtclss_;

    void		doStuff(CallBacker*);
};


uiQtAppMgr::uiQtAppMgr( uiODMain* a )
    : appl_(a)
    , qtclss_(0)
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    mnumgr.utilMnu()->insertAction(
	new uiAction( toUiString("Qt Thing"), mCB(this,uiQtAppMgr,doStuff) ) );
}


void uiQtAppMgr::doStuff( CallBacker* )
{
    if ( qtclss_ ) delete qtclss_;
    qtclss_ = new QtClss( appl_->getWidget(0) );
    qtclss_->go();
}


mDefODInitPlugin(uiQtApp)
{
    mDefineStaticLocalObject( PtrMan<uiQtAppMgr>, theinst_, = 0 );
    if ( theinst_ ) return 0;

    theinst_ = new uiQtAppMgr( ODMainWin() );
    if ( !theinst_ )
	return "Cannot instantiate QtApp plugin";

    return 0;
}
