
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uimadagascarmain.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uitoolbar.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
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
{
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
    mnumgr.dTectTBChanged.notify( mCB(this,uiMadagascarLink,updateToolBar) );
    mnumgr.dTectMnuChanged.notify( mCB(this,uiMadagascarLink,updateMenu) );
    IOM().surveyToBeChanged.notify( mCB(this,uiMadagascarLink,survChg) );
    updateToolBar(0);
    updateMenu(0);
}


uiMadagascarLink::~uiMadagascarLink()
{
    delete madwin_;
}


void uiMadagascarLink::updateToolBar( CallBacker* )
{
    mnumgr.dtectTB()->addButton( "madagascar", "Madagascar link",
	    			 mCB(this,uiMadagascarLink,doMain) );
}


void uiMadagascarLink::updateMenu( CallBacker* )
{
    delete madwin_; madwin_ = 0; ishidden_ = false;
    uiAction* newitem = new uiAction( "Madagascar ...",
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
	uiMSG().error( errmsg );
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
    mDefineStaticLocalObject( uiMadagascarLink*, lnk, = 0 );
    if ( lnk ) return 0;

    IOMan::CustomDirData cdd( ODMad::sKeyMadSelKey(), ODMad::sKeyMadagascar(),
	    		      "Madagascar data" );
    MultiID id = IOMan::addCustomDataDir( cdd );
    if ( id != ODMad::sKeyMadSelKey() )
	return "Cannot create 'Madagascar' directory in survey";

#ifdef MAD_UIMSG_IF_FAIL
    if ( !ODMad::PI().errMsg().isEmpty() )
	uiMSG().error( ODMad::PI().errMsg() );
#endif

    lnk = new uiMadagascarLink( *ODMainWin() );
    return lnk ? 0 : ODMad::PI().errMsg().buf();
}
