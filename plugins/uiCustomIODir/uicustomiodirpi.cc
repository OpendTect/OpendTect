/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2006
-*/

#include "uicustomiodirmod.h"
#include "uiioobjsel.h"
#include "uiodmenumgr.h"
#include "uiodmain.h"
#include "uimenu.h"
#include "uidialog.h"
#include "uimsg.h"
#include "uistrings.h"

#include "ioobjctxt.h"
#include "transl.h"
#include "odplugin.h"
#include "odversion.h"

mDefODPluginInfo(uiCustomIODir)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
      "Custom Data directory usage",
      "OpendTect",
      "dGB",
      "0.0.3",
      "Test of Custom Data directory usage." ) );
    return &retpi;
}


class uiCustomIODirMgr : public uiPluginInitMgr
{ mODTextTranslationClass(uiCustomIODirMgr);
public:

			uiCustomIODirMgr();

private:

    void		dTectMenuChanged() override;

    void		doDlg(CallBacker*);

};


// Has to be > 200000
#define cSelDirNr 207543

// The following lines add a new data type and format
// Note that this is more or less bypassing the Translator system
// but my guess is that that is exactly what you want.
mDeclEmptyTranslatorBundle(uiCustomIODir,MyObj,MyFmt,"myo")
mDefSimpleTranslatorsWithSelKey(MyObj,"My Object",MyFmt,None,cSelDirNr)



uiCustomIODirMgr::uiCustomIODirMgr()
    : uiPluginInitMgr()
{
    init();
}


void uiCustomIODirMgr::dTectMenuChanged()
{
    appl().menuMgr().utilMnu()->insertItem(
			new uiAction( m3Dots(tr("Test Custom data directory")),
				      mCB(this,uiCustomIODirMgr,doDlg) ) );
}


class uiCustomIODirTester : public uiDialog
{
public:

uiCustomIODirTester( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Custom IODir Tester","Specify Own objects",
				 mTODOHelpKey))
{
    selinfld_ = new uiIOObjSel( this, mIOObjContext(MyObj) );

    IOObjContext ctxt( mIOObjContext(MyObj) );
    ctxt.forread = false;
    seloutfld_ = new uiIOObjSel( this, ctxt, "Output thing" );
    seloutfld_->attach( alignedBelow, selinfld_ );
}

bool acceptOK( CallBacker* )
{
    const IOObj* inioobj = selinfld_->ioobj();
    const IOObj* outioobj = seloutfld_->ioobj();
    if ( !inioobj || !outioobj )
	return false;

    // Do your thing here, for example use the filenames:
    BufferString inputname = inioobj->fullUserExpr( true );
    BufferString outputname = outioobj->fullUserExpr( false );
    uiMSG().message( inputname, " is input\noutput is: ", outputname );

    return true;
}

    uiIOObjSel*	selinfld_;
    uiIOObjSel*	seloutfld_;

};


void uiCustomIODirMgr::doDlg( CallBacker* )
{
    uiCustomIODirTester dlg( &appl() );
    dlg.go();
}


mDefODInitPlugin(uiCustomIODir)
{
    mDefineStaticLocalObject( PtrMan<uiCustomIODirMgr>, theinst_,
				= new uiCustomIODirMgr() );
    if ( !theinst_ )
	return "Cannot instantiate CustomDir plugin";

    // These factory adds are necessary since 4.6:
    MyObjTranslatorGroup::initClass();
    MyFmtMyObjTranslator::initClass();

    // The following code assures your data directory will be available
    // in every survey

    IOMan::CustomDirData cdd( cSelDirNr, "CustomDir", "Custom data" );
    DBKey::DirID id = IOMan::addCustomDataDir( cdd );
    if ( id.getI() != cSelDirNr )
	return "The 'Custom data' storage directory could not be created";

    // Tip one: To create an object, press 'Select' for the output, fill in a
    // name and press OK. That will add an entry. Do it again for another
    // entry. Now you can also select an input. In normal practise, users will
    // of course create entries in another way, this is just to get something
    // there.
    // Tip two: It may be instructive to view the .omf in the new data directory
    // 'CustomDir' once you have loaded this plugin for the first time. It
    // will show you where the parameters for the mDef...Translators... macros
    // go.

    return nullptr;
