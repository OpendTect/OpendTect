/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

#include "uihellomod.h"

#include "odplugin.h"

#include "uidialog.h"
#include "uigeninput.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uistrings.h"

mExternC(uiHello) int GetuiHelloPluginType();
mExternC(uiHello) PluginInfo* GetuiHelloPluginInfo();
mExternC(uiHello) const char* InituiHelloPlugin(int,char**);

int GetuiHelloPluginType()
{
    return PI_AUTO_INIT_LATE;
}


PluginInfo* GetuiHelloPluginInfo()
{
    mDefineStaticLocalObject( PluginInfo, info, )
    info.dispname_ = "Hello World plugin (GUI)";
    info.productname_ = "Hello World";
    info.creator_ = "Nanne";
    info.version_ = "1.1.1";
    info.text_ = "This is the GUI variant of the uiHello example.\n"
		 "See the plugin manual for details.";
    return &info;
}


// OK: we need an object to receive the CallBacks. In serious software,
// that may be a 'normal' object inheriting from CallBacker.

class uiHelloMgr :  public CallBacker
{ mODTextTranslationClass(uiHelloMgr);
public:
			uiHelloMgr(uiODMain&);

    uiODMain&		appl_;
    void		dispMsg(CallBacker*);
};


uiHelloMgr::uiHelloMgr( uiODMain& a )
	: appl_(a)
{
    uiAction* newitem = new uiAction( m3Dots(tr("Display Hello Message")),
					  mCB(this,uiHelloMgr,dispMsg) );
    appl_.menuMgr().utilMnu()->insertItem( newitem );
}


class uiHelloMsgBringer : public uiDialog
{ mODTextTranslationClass(uiHelloMsgBringer);
public:

uiHelloMsgBringer( uiParent* p )
    : uiDialog(p,Setup("Hello Message Window","Specify hello message",
			mNoHelpKey))
{
    txtfld_ = new uiGenInput( this, tr("Hello message"),
				StringInpSpec("Hello world") );

    typfld_ = new uiGenInput( this, tr("Message type"),
		BoolInpSpec(true,uiStrings::sInfo(),uiStrings::sWarning()) );
    typfld_->attach( alignedBelow, txtfld_ );

    closefld_ = new uiGenInput( this, tr("Close window"), BoolInpSpec(false) );
    closefld_->attach( alignedBelow, typfld_ );
}

bool acceptOK( CallBacker* )
{
    const char* typedtxt = txtfld_->text();
    if ( ! *typedtxt )
    {
	uiMSG().error( tr("Please type a message text") );
	return false;
    }
    if ( typfld_->getBoolValue() )
	uiMSG().message( typedtxt );
    else
	uiMSG().warning( typedtxt );

    const bool doclose = closefld_->getBoolValue();
    return doclose;
}

    uiGenInput*	txtfld_;
    uiGenInput*	typfld_;
    uiGenInput*	closefld_;

};


void uiHelloMgr::dispMsg( CallBacker* )
{
    uiHelloMsgBringer dlg( &appl_ );
    dlg.go();
}


const char* InituiHelloPlugin( int argc, char** argv )
{
    mDefineStaticLocalObject( PtrMan<uiHelloMgr>, theinst_, = nullptr );
    if ( theinst_ ) return nullptr;

    theinst_ = new uiHelloMgr( *ODMainWin() );
    if ( !theinst_ )
	return "Cannot instantiate Hello plugin";

    return nullptr; // All OK - no error messages
}
