
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: uimadpi.cc,v 1.16 2009-07-22 16:01:28 cvsbert Exp $";

#include "uimadagascarmain.h"
#include "uiodmenumgr.h"
#include "uimenu.h"
#include "uitoolbar.h"
#include "envvars.h"
#include "filegen.h"
#include "filepath.h"
#include "separstr.h"
#include "maddefs.h"
#include "madio.h"
#include "uimsg.h"
#include "plugins.h"
#include "ioman.h"

mExternC int GetuiMadagascarPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC PluginInfo* GetuiMadagascarPluginInfo()
{
    static PluginInfo retpi = {
	"Madagascar link",
	"dGB (Bert)",
	"3.0",
    	"Enables the Madagascar link." };
    return &retpi;
}


bool checkEnvVars( BufferString& msg )
{
    BufferString rsfdir = GetEnvVar( "RSFROOT" );
    if ( rsfdir.isEmpty() || !File_isDirectory(rsfdir.buf()) )
    {
	msg = "RSFROOT is either not set or invalid";
	return false;
    }
    
    return true;
}


class uiMadagascarLink :  public CallBacker
{
public:
			uiMadagascarLink(uiODMain&);
			~uiMadagascarLink();

    uiODMenuMgr&	mnumgr;
    uiMadagascarMain*	madwin_;

    void		doMain(CallBacker*);
    void		updateToolBar(CallBacker*);
    void		updateMenu(CallBacker*);
    void		survChg(CallBacker*);

};


uiMadagascarLink::uiMadagascarLink( uiODMain& a )
    	: mnumgr(a.menuMgr())
        , madwin_(0)
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
    mnumgr.dtectTB()->addButton( "madagascar.png",
	    			 mCB(this,uiMadagascarLink,doMain),
				 "Madagascar link" );
}


void uiMadagascarLink::updateMenu( CallBacker* )
{
    delete madwin_; madwin_ = 0;
    uiMenuItem* newitem = new uiMenuItem( "&Madagascar ...",
	    				  mCB(this,uiMadagascarLink,doMain) );
    mnumgr.procMnu()->insertItem( newitem );
}


void uiMadagascarLink::survChg( CallBacker* )
{
    if ( !madwin_ ) return;

    madwin_->askSave(false);
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
	madwin_ = new uiMadagascarMain( 0 );

    madwin_->show();
}


mExternC const char* InituiMadagascarPlugin( int, char** )
{
    static uiMadagascarLink* lnk = 0;
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
