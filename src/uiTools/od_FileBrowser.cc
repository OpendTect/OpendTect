/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2003
________________________________________________________________________

-*/

#include "uitextfile.h"
#include "uimain.h"

#include "commandlineparser.h"
#include "file.h"
#include "moddepmgr.h"
#include "oscommand.h"
#include "prog.h"


static void printBatchUsage()
{
    od_ostream& strm = od_ostream::logStream();
    strm << "Usage: " << "od_FileBrowser";
    strm << " [OPTION]... [FILE]...\n";
    strm << "Opens an examine window for a text file argument, ";
    strm << "optionaly editable.\n";
    strm << "Mandatory argument:\n";
    strm << "\t --" << File::ViewPars::sKeyFile();
    strm << "\tfilename (must be the full path)\n";
    strm << "Optional arguments:\n";
    strm << "\t --" << File::ViewPars::sKeyMaxLines() << "\tnrlines\n";
    strm << "\t --" << File::ViewPars::sKeyStyle() << "\ttext|table|log|bin\n";
    strm << "\t --" << File::ViewPars::sKeyEdit() << "\t\tAllow file edition\n";
    strm << od_endl;
}


#define mErrRet() \
{ \
    printBatchUsage(); \
    return 1; \
}

int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv, false );

    uiMain app;
    auto& clp = app.commandLineParser();
    const int nrargs = clp.nrArgs();
    if ( nrargs < 1 )
	mErrRet()

    OD::ModDeps().ensureLoaded( "Network" );

    BufferString fnm = clp.keyedString( File::ViewPars::sKeyFile() );
    if ( fnm.isEmpty() )
	fnm.set( clp.getArg(0) );
    if ( fnm.isEmpty() )
	mErrRet()

#ifdef __win__
    fnm = File::linkEnd( fnm );
#endif

    od_ostream& strm = od_ostream::logStream();
    if ( !File::exists(fnm.str()) )
    {
	strm << "File " << fnm << " does not exists." << od_endl;
	return 1;
    }
    else if ( !File::isReadable(fnm.str()) )
    {
	strm << "File " << fnm << " is not readable." << od_endl;
	return 1;
    }
    else if ( !File::isFile(fnm.str()) )
    {
	strm << "File " << fnm << " is not a file." << od_endl;
	return 1;
    }

    File::ViewPars vp;
    if ( clp.hasKey(File::ViewPars::sKeyMaxLines()) )
	vp.maxnrlines_ = clp.keyedValue<int>( File::ViewPars::sKeyMaxLines() );
    vp.editable_ = clp.hasKey( File::ViewPars::sKeyEdit() );

    BufferString stl = clp.keyedString( File::ViewPars::sKeyStyle() );
    if ( !stl.isEmpty() )
	File::ViewStyleDef().parse( stl.str(), vp.style_ );

    OD::ModDeps().ensureLoaded( "uiTools" );

    uiTextFileDlg::Setup fdsetup( toUiString(fnm) );
    fdsetup.allowopen( vp.editable_ ).allowsave( true );
    PtrMan<uiDialog> dlg = new uiTextFileDlg( 0, vp, fdsetup, fnm );
    dlg->setActivateOnFirstShow();
    app.setTopLevel( dlg );
    dlg->show();

    return app.exec();
}
