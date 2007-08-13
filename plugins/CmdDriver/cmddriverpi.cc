/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id";

#include "cmddriver.h"
#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uidialog.h"
#include "uifileinput.h"
#include "filegen.h"
#include "filepath.h"
#include "oddirs.h"
#include "plugins.h"

extern "C" int GetCmdDriverPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetCmdDriverPluginInfo()
{
    static PluginInfo retpii = {
	"Command driver",
	"dGB (Bert/Jaap)",
	"=od",
	"Used for testing and general 'scripting'." };
    return &retpii;
}

class uiCmdDriverMgr : public CallBacker
{
public:
    			uiCmdDriverMgr(uiODMain&);

    uiODMain&		appl_;
    CmdDriver*		drv_;
    uiPopupMenu*	cmddrvmnu_;
    uiMenuItem*		runcmddrvitm_;
    uiMenuItem*		nameguideitm_;
    void		doIt(CallBacker*);
    void		toolTipChange(CallBacker*);

};


uiCmdDriverMgr::uiCmdDriverMgr( uiODMain& a )
    	: appl_(a)
    	, drv_(0)
{
    uiODMenuMgr& mnumgr = appl_.menuMgr();
    cmddrvmnu_ = new uiPopupMenu( &appl_, "Command &Driver" );
    mnumgr.utilMnu()->insertItem( cmddrvmnu_ );
    runcmddrvitm_ = new uiMenuItem( "&Run ...",
				    mCB(this,uiCmdDriverMgr,doIt) );
    cmddrvmnu_->insertItem( runcmddrvitm_ );
    nameguideitm_ = new uiMenuItem( "&Tooltip name guide",
				    mCB(this,uiCmdDriverMgr,toolTipChange) );
    cmddrvmnu_->insertItem( nameguideitm_ );
    nameguideitm_->setCheckable( true );
    nameguideitm_->setChecked( false );
}


class uiCmdDriverInps : public uiDialog
{
public:

uiCmdDriverInps( uiParent* p, CmdDriver& d )
        : uiDialog(p,Setup("Command execution","Specify the file with commands"
			    " to execute","0.0.0"))
	, drv_(d)
{
    fnmfld = new uiFileInput( this, "Command file", uiFileInput::Setup()
				.filter("*.cmd")
				.forread(true)
				.withexamine(true) );
    const BufferString procdir = GetProcFileName(0);
    fnmfld->setDefaultSelectionDir( procdir );
    outdirfld = new uiFileInput( this, "Output directory",
	    			uiFileInput::Setup(procdir)
				.forread(false)
				.directories(true) );
    outdirfld->attach( alignedBelow, fnmfld );
    outdirfld->setDefaultSelectionDir( procdir );
    popupfld = new uiGenInput( this, "Show status window",
	    			BoolInpSpec(true) );
    popupfld->attach( alignedBelow, outdirfld );
}

bool acceptOK( CallBacker* )
{
    FilePath fp( outdirfld->fileName() );
    if ( !File_exists(fp.pathOnly()) )
    {
	uiMSG().error( drv_.errMsg() );
	return false;
    }

    BufferString fnm = fnmfld->fileName();
    if ( File_isEmpty(fnm) )
    {
	uiMSG().error( "Invalid command file selected" );
	return false;
    }
    if ( !drv_.getActionsFromFile(fnm) )
    {
	uiMSG().error( drv_.errMsg() );
	return false;
    }

    fnm = fp.fullPath();
    if ( !File_isDirectory(fnm) )
    {
	if ( File_exists(fnm) )
	    File_remove(fnm,NO);
	File_createDir( fnm, 0 );
    }

    drv_.setOutputDir( fnm );
    return true;
}

    uiFileInput*	fnmfld;
    uiFileInput*	outdirfld;
    uiGenInput*		popupfld;
    BufferString	fnm;
    CmdDriver&		drv_;

};


void uiCmdDriverMgr::doIt( CallBacker* )
{
    if ( drv_ ) delete drv_;
    drv_ = new CmdDriver( cmddrvmnu_ );
    uiCmdDriverInps* dlg = new uiCmdDriverInps( &appl_, *drv_ );
    bool ret = dlg->go();
    delete dlg;
    if ( !ret )
    	{ delete drv_; drv_ = 0; return; }
    
    drv_->execute();
}


void uiCmdDriverMgr::toolTipChange( CallBacker* cb )
{
    uiObject::useNameToolTip( nameguideitm_->isChecked() );
}


extern "C" const char* InitCmdDriverPlugin( int, char** )
{
    (void)new uiCmdDriverMgr( *ODMainWin() );
    return 0;
}
