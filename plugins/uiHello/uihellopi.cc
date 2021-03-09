/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

#include "uihellomod.h"

#include "uidialog.h"
#include "uigeninput.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmain.h"
#include "uiodmenumgr.h"

#include "odplugin.h"


mDefODPluginInfo(uiHello)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"Hello World plugin (GUI)",
	"Hello World",
	"dGB",
	"1.1.1",
	"This is the GUI variant of the uiHello example.\n"
	"See the plugin manual for details.") );
    return &retpi;
}


// OK: we need an object to receive the CallBacks. In serious software,
// that may be a 'normal' object inheriting from CallBacker.

class uiHelloMgr : public uiPluginInitMgr
{ mODTextTranslationClass(uiHelloMgr);
public:
			uiHelloMgr();

private:

    void		dTectMenuChanged() override;

    void		dispMsg(CallBacker*);
};


uiHelloMgr::uiHelloMgr()
    : uiPluginInitMgr()
{
    init();
}


void uiHelloMgr::dTectMenuChanged()
{
    appl().menuMgr().utilMnu()->insertAction(
			new uiAction( m3Dots(tr("Display Hello Message")),
					mCB(this,uiHelloMgr,dispMsg) ) );
}


class uiHelloMsgBringer : public uiDialog
{ mODTextTranslationClass(uiHelloMsgBringer);
public:

uiHelloMsgBringer( uiParent* p )
    : uiDialog(p,Setup(tr("Hello Message Window"),tr("Specify hello message"),
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

bool acceptOK()
{
    const char* typedtxt = txtfld_->text();
    if ( ! *typedtxt )
    {
	uiMSG().error( tr("Please type a message text") );
	return false;
    }
    if ( typfld_->getBoolValue() )
	uiMSG().message( toUiString(typedtxt) );
    else
	uiMSG().warning( toUiString(typedtxt) );

    const bool doclose = closefld_->getBoolValue();
    return doclose;
}

    uiGenInput*	txtfld_;
    uiGenInput*	typfld_;
    uiGenInput*	closefld_;

};


void uiHelloMgr::dispMsg( CallBacker* )
{
    uiHelloMsgBringer dlg( &appl() );
    dlg.go();
}


mDefODInitPlugin(uiHello)
{
    mDefineStaticLocalObject( PtrMan<uiHelloMgr>, theinst_,
				= new uiHelloMgr() );
    if ( !theinst_ )
	return "Cannot instantiate Hello plugin";

    return nullptr; // All OK - no error messages
}
