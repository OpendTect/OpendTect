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
	"OpendTect command driver",
	"dGB - Bert Bril",
	"=od",
	"Used for testing and general 'scripting'." };
    return &retpii;
}

class uiCmdDriverMgr : public CallBacker
{
public:
    			uiCmdDriverMgr(uiODMain&);

    uiODMain&		appl;
    void		doIt(CallBacker*);

};


uiCmdDriverMgr::uiCmdDriverMgr( uiODMain& a )
    	: appl(a)
{
    uiODMenuMgr& mnumgr = appl.menuMgr();
    uiMenuItem* newitem = new uiMenuItem( "Command &Driver ...",
	    				  mCB(this,uiCmdDriverMgr,doIt) );
    mnumgr.utilMnu()->insertItem( newitem );
}


class uiCmdDriverInps : public uiDialog
{
public:

uiCmdDriverInps( uiParent* p, CmdDriver& d )
        : uiDialog(p,Setup("Command execution","Specify the file with commands"
			    " to execute"))
	, drv(d)
{
    fnmfld = new uiFileInput( this, "Command file", uiFileInput::Setup()
				.filter("*.cmd;;*")
				.forread(true)
				.withexamine(true) );
    FilePath fp( GetDataDir() ); fp.add( "Proc" );
    fnmfld->setDefaultSelectionDir( fp.fullPath() );
    outdirfld = new uiFileInput( this, "Output directory", uiFileInput::Setup()
				.forread(false)
				.directories(true) );
    outdirfld->attach( alignedBelow, fnmfld );
    outdirfld->setDefaultSelectionDir( fp.fullPath() );
}

bool acceptOK( CallBacker* )
{
    FilePath fp( outdirfld->fileName() );
    if ( !File_exists(fp.pathOnly()) )
    {
	uiMSG().error( drv.errMsg() );
	return false;
    }

    BufferString fnm = fnmfld->fileName();
    if ( File_isEmpty(fnm) )
    {
	uiMSG().error( "Invalid command file selected" );
	return false;
    }
    if ( !drv.getActionsFromFile(fnm) )
    {
	uiMSG().error( drv.errMsg() );
	return false;
    }

    fnm = fp.fullPath();
    if ( !File_isDirectory(fnm) )
    {
	if ( File_exists(fnm) )
	    File_remove(fnm,NO);
	File_createDir( fnm, 0 );
    }

    drv.setOutputDir( fnm );
    return true;
}

    uiFileInput*	fnmfld;
    uiFileInput*	outdirfld;
    BufferString	fnm;
    CmdDriver&		drv;

};


void uiCmdDriverMgr::doIt( CallBacker* )
{
    CmdDriver drv;
    uiCmdDriverInps* dlg = new uiCmdDriverInps( &appl, drv );
    bool ret = dlg->go();
    delete dlg;
    if ( !ret ) return;

    drv.execute();
}




extern "C" const char* InitCmdDriverPlugin( int, char** )
{
    (void)new uiCmdDriverMgr( *ODMainWin() );
    return 0;
}
