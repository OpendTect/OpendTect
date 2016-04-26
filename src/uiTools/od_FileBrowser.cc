/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
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
    strm << "\t --" << OS::MachineCommand::sKeyFG() << "\t\tRun in foreground";
    strm << od_endl;
}


#define mErrRet() \
{ \
    printBatchUsage(); \
    return ExitProgram( 1 ); \
}

int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    CommandLineParser clp;
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
	return ExitProgram( 1 );
    }
    else if ( !File::isReadable(fnm.str()) )
    {
	strm << "File " << fnm << " is not readable." << od_endl;
	return ExitProgram( 1 );
    }
    else if ( !File::isFile(fnm.str()) )
    {
	strm << "File " << fnm << " is not a file." << od_endl;
	return ExitProgram( 1 );
    }

    File::ViewPars vp;
    clp.getVal( File::ViewPars::sKeyMaxLines(), vp.maxnrlines_ );
    vp.editable_ = clp.hasKey( File::ViewPars::sKeyEdit() );

    BufferString stl;
    if ( clp.getVal(File::ViewPars::sKeyStyle(),stl) )
	File::ViewStyleDef().parse( stl.str(), vp.style_ );

    bool dofork = true;
    if ( clp.hasKey(OS::MachineCommand::sKeyFG()) )
	dofork = false;
#ifdef __mac__
    dofork = false;
#endif

    if ( dofork )
	ForkProcess();

    OD::ModDeps().ensureLoaded( "uiTools" );

    uiMain app( argc, argv );

    uiTextFileDlg::Setup fdsetup( toUiString(fnm) );
    fdsetup.allowopen( vp.editable_ ).allowsave( true );
    uiTextFileDlg* dlg = new uiTextFileDlg( 0, vp, fdsetup, fnm );
    dlg->showAlwaysOnTop();
    app.setTopLevel( dlg );
    dlg->show();

    return ExitProgram( app.exec() );
}
