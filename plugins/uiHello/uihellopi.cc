/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: uihellopi.cc,v 1.4 2003-11-02 22:37:59 bert Exp $";

#include "uimsg.h"

#ifndef PLAN_B

extern "C" const char* InituiHelloPlugin( int, char** )
{
    uiMSG().message( "Hello world" );
    return 0; // All OK - no error messages
}


#else /* PLAN_B is defined */


#include "ui3dapplication.h"
#include "ui3dapplman.h"
#include "uidtectman.h"
#include "uiapplserv.h"
#include "uimenu.h"
#include "uidialog.h"
#include "uigeninput.h"
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

    void		dispMsg(CallBacker*);
};


void uiHelloMgr::init( CallBacker* )
{
    if ( initdone ) return;

    applman->utilMnu()->insertItem(
	new uiMenuItem("&Diplay Hello Message ...",mCB(this,uiHelloMgr,dispMsg) ) );

    initdone = true;
}


class uiHelloMsgBringer : public uiDialog
{
public:

uiHelloMsgBringer( uiParent* p )
    : uiDialog(p,Setup("Hello Message Window",
			"Specify hello message"))
{
    txtfld = new uiGenInput( this, "Hello message",
	    			StringInpSpec("Hello world") );
    typfld = new uiGenInput( this, "Message type",
	    			BoolInpSpec("Info","Warning") );
    typfld->attach( alignedBelow, txtfld );
}

bool acceptOK( CallBacker* )
{
    const char* typedtxt = txtfld->text();
    if ( ! *typedtxt )
    {
	uiMSG().error( "Please type a text" );
	return false;
    }
    if ( typfld->getBoolValue() )
	uiMSG().message( typedtxt );
    else
	uiMSG().warning( typedtxt );
    return true;
}

    uiGenInput*	txtfld;
    uiGenInput*	typfld;

};


void uiHelloMgr::dispMsg( CallBacker* )
{
    uiHelloMsgBringer dlg( applman->applService().parent() );
    dlg.go();
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
