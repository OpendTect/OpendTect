/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    uiMain app( argc, argv );

    OD::ModDeps().ensureLoaded( "Network" );
    OD::ModDeps().ensureLoaded( "uiBase" );

    const CommandLineParser clp( argc, argv );
    const int nrargs = clp.nrArgs();
    if ( nrargs < 1 )
	mErrRet()

    BufferString fnm;
    if ( clp.hasKey(File::ViewPars::sKeyFile()) )
	clp.getVal( File::ViewPars::sKeyFile(), fnm );
    else if ( nrargs == 1 )
	fnm.set( clp.getArg(0) );

    if ( fnm.isEmpty() )
	mErrRet()

#ifdef __win__
    if ( File::isLink(fnm) )
	fnm = const_cast<char*>(File::linkTarget(fnm));
#endif

    od_ostream& strm = od_ostream::logStream();
    if ( !File::exists(fnm.str()) )
    {
	strm << "File " << fnm << " does not exists." << od_endl;
	return 1;
    }
/*    else if ( !File::isReadable(fnm.str()) )
    {
	strm << "File " << fnm << " is not readable." << od_endl;
	return 1;
    }*/
    else if ( !File::isFile(fnm.str()) )
    {
	strm << "File " << fnm << " is not a file." << od_endl;
	return 1;
    }

    File::ViewPars vp;
    clp.getVal( File::ViewPars::sKeyMaxLines(), vp.maxnrlines_ );
    vp.editable_ = clp.hasKey( File::ViewPars::sKeyEdit() );

    BufferString stl;
    if ( clp.getVal(File::ViewPars::sKeyStyle(),stl) )
	parseEnum( stl.str(), vp.style_ );

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiTools" );
    uiTextFileDlg::Setup fdsetup( toUiString(fnm) );
    fdsetup.allowopen( vp.editable_ ).allowsave( true );
    PtrMan<uiDialog> topdlg = new uiTextFileDlg( nullptr, vp, fdsetup, fnm );
    topdlg->setActivateOnFirstShow();
    app.setTopLevel( topdlg );
    PIM().loadAuto( true );
    topdlg->show();

    return app.exec();
}
