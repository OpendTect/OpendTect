/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: uihellopi.cc,v 1.3 2003-11-02 18:02:39 bert Exp $";

#include "uimsg.h"

#ifndef PLAN_B

extern "C" const char* InituiHelloPlugin( int, char** )
{
    uiMSG().message( "Hello world" );
    return 0; // All OK - no error messages
}


#else /* that is, PLAN_B is defined */


#include "ui3dapplication.h"
#include "ui3dapplman.h"
#include "uidtectman.h"
#include "uimenu.h"
#include "plugins.h"

extern "C" int GetuiHelloPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetuiHelloPluginInfo()
{
    static PluginInfo retpi = {
	"uiHello plugin - plan B",
	"Bert",
	"1.0.1",
	"This is the more extensive variant of the uiHello example.\n"
   	"See the plugin manual for details." };
    return &retpi;
}


// OK: we need an object to receive the CallBacks. In serious software,
// that may be a 'normal' object inheriting from CallBacker.

class uiHelloMgr :  public CallBacker
{
public:

			uiHelloMgr( ui3DApplMan* a )
			    : applman(a), initdone(false)	{}

    void		init(CallBacker*);

    ui3DApplMan*	applman;
    bool		initdone;

    void		sayHello(CallBacker*);
};


void uiHelloMgr::init( CallBacker* )
{
    if ( initdone ) return;

    applman->utilMnu()->insertItem(
	    new uiMenuItem("&Say hello",mCB(this,uiHelloMgr,sayHello) ) );

    initdone = true;
}


void uiHelloMgr::sayHello( CallBacker* )
{
    uiMSG().message( "Hello world" );
}


extern "C" const char* InituiHelloPlugin( int, char** )
{
    static uiHelloMgr* mgr = 0;
    if ( !mgr ) mgr = new uiHelloMgr( dTectMainWin() );

    if ( mgr->applman->uidMan()->isFinalised() )
	mgr->init( mgr->applman->uidMan() );
    else
	mgr->applman->uidMan()->finalised.notify(mCB(mgr,uiHelloMgr,init));

    return 0; // All OK - no error messages
}


#endif /* ifdef PLAN_B */
