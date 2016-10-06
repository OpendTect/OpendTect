
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/


#include "uimadagascarmain.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uitoolbar.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "dbman.h"
#include "maddefs.h"
#include "madio.h"
#include "odplugin.h"
#include "separstr.h"


mDefODPluginInfo(uiMadagascar)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Madagascar link",
	"OpendTect",
	"dGB (Bert, Raman)",
	"3.2",
	"A link to the Madagascar system."
	    "\nSee http://opendtect.org/links/madagascar.html"
	    " for info on Madagascar."));
    return &retpi;
}


static bool checkEnvVars( BufferString& msg )
{
    BufferString rsfdir = GetEnvVar( "RSFROOT" );
    if ( rsfdir.isEmpty() || !File::isDirectory(rsfdir.buf()) )
    {
	msg = "RSFROOT is either not set or invalid";
	return false;
    }

    return true;
}


class uiMadagascarLink	: public CallBacker
{ mODTextTranslationClass(uiMadagascarLink);
public:
			uiMadagascarLink(uiODMain&);
			~uiMadagascarLink();

    uiODMain&		appl_;
    uiODMenuMgr&	mnumgr;
    uiMadagascarMain*	madwin_;
    bool		ishidden_;

    void		doMain(CallBacker*);
    void		updateToolBar(CallBacker*);
    void		updateMenu(CallBacker*);
    void		survChg(CallBacker*);
    void		winHide(CallBacker*);

};


uiMadagascarLink::uiMadagascarLink( uiODMain& a )
    : mnumgr(a.menuMgr())
    , madwin_(0)
    , ishidden_(false)
    , appl_(a)
{
    mAttachCB( mnumgr.dTectTBChanged, uiMadagascarLink::updateToolBar );
    mAttachCB( mnumgr.dTectMnuChanged, uiMadagascarLink::updateMenu );
    mAttachCB( DBM().surveyToBeChanged, uiMadagascarLink::survChg );
    updateToolBar(0);
    updateMenu(0);
}


uiMadagascarLink::~uiMadagascarLink()
{
    detachAllNotifiers();
}


void uiMadagascarLink::updateToolBar( CallBacker* )
{
}


void uiMadagascarLink::updateMenu( CallBacker* )
{
    delete madwin_; madwin_ = 0; ishidden_ = false;
    uiAction* newitem = new uiAction( m3Dots(tr("Madagascar")),
					  mCB(this,uiMadagascarLink,doMain),
					  "madagascar" );
    mnumgr.procMnu()->insertItem( newitem );
}


void uiMadagascarLink::survChg( CallBacker* )
{
    if ( !madwin_ ) return;

    madwin_->askSave(false);
}


void uiMadagascarLink::winHide( CallBacker* )
{
    ishidden_ = true;
}


void uiMadagascarLink::doMain( CallBacker* )
{
    BufferString errmsg;
    if ( !checkEnvVars(errmsg) )
    {
	uiMSG().error( mToUiStringTodo(errmsg) );
	return;
    }

    if ( !madwin_ )
    {
	madwin_ = new uiMadagascarMain( &appl_ );
	madwin_->windowHide.notify( mCB(this,uiMadagascarLink,winHide) );
    }

    ishidden_ = false;
    madwin_->show();
    madwin_->raise();
}


mDefODInitPlugin(uiMadagascar)
{
    mDefineStaticLocalObject( PtrMan<uiMadagascarLink>, theinst_, = 0 );
    if ( theinst_ ) return 0;

    theinst_ = new uiMadagascarLink( *ODMainWin() );
    if ( !theinst_ )
	return ODMad::PI().errMsg().getFullString().str();

    DBMan::CustomDirData cdd( ODMad::cMadDirIDNr(), ODMad::sKeyMadagascar(),
			      "Madagascar data" );
    uiRetVal uirv = DBMan::addCustomDataDir( cdd );
    if ( !uirv.isOK() )
    {
	mDeclStaticString(ret);
	ret = uirv.getText();
	return ret.str();
    }

#ifdef MAD_UIMSG_IF_FAIL
    if ( ODMad::PI().errMsg().isSet() )
	uiMSG().error( ODMad::PI().errMsg() );
#endif

    return 0;
}
