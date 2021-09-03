/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Prajjaval Singh
 Date:          November 2019
________________________________________________________________________

-*/

#include "commandlineparser.h"
#include "file.h"
#include "moddepmgr.h"
#include "oddirs.h"
#include "prog.h"
#include "pythonaccess.h"
#include "procdescdata.h"
#include "uifirewallprocsetterdlg.h"
#include "uimain.h"
#include "uimsg.h"


static const char* odflag()
{
    return ProcDesc::DataEntry::getTypeFlag( ProcDesc::DataEntry::ODv6 );
}


static const char* pyflag()
{
    return ProcDesc::DataEntry::getTypeFlag( ProcDesc::DataEntry::Python );
}


static void printBatchUsage()
{
    od_ostream& strm = od_ostream::logStream();
    strm << "Usage: " << "od_FilewallProcSetter ";
    strm << "[--help] ";
    strm << "[--" << odflag() << " <path till the OpendTect installation directory ([...]/OpendTect/6.6.0)>] ";
    strm << "[--" << pyflag() << " <path till python folder>]";
    strm << "It uses administrative rights to launch FirewallProcSetter Dialog\n";
    strm << "\nThese are common actions that dialog can do\n";
    strm << "\t"
	 << ProcDesc::DataEntry::getCMDActionKey( ProcDesc::DataEntry::Add )
	 << "\t to add rules to firewall\n";
    strm << "\t"
	 << ProcDesc::DataEntry::getCMDActionKey( ProcDesc::DataEntry::Remove )
	 << "\t to remove rules from firewall\n";
    strm << "\t"
	 << ProcDesc::DataEntry::getCMDActionKey( ProcDesc::DataEntry::AddNRemove )
	 << "\t  to add and remove rules to firewall\n";
    strm << "\n Use short and quoted paths";
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
    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiODMain" );
    PIM().loadAuto( true );

    CommandLineParser parser;
    if ( parser.hasKey("help") || parser.hasKey("h") )
    {
	printBatchUsage();
	return 0;
    }

    parser.setKeyHasValue( odflag() );
    parser.setKeyHasValue( pyflag() );
    BufferStringSet proctyp;
    parser.getNormalArguments( proctyp );
    if ( proctyp.isEmpty() )
	mErrRet()

    const BufferString type = parser.getArg( 0 );
    if ( !ProcDesc::DataEntry::isCMDOK(type) )
	mErrRet();

    BufferString path;
    if ( parser.getVal(odflag(),path) && !File::isDirectory(path) )
	ePDD().setPath( path );

    BufferString pythonpath;
    if ( parser.getVal(pyflag(),pythonpath) && !File::isDirectory(pythonpath) )
	mErrRet();

    const ProcDesc::DataEntry::ActionType opertype =
		    ProcDesc::DataEntry::getActionTypeForCMDKey( type );

    PtrMan<uiFirewallProcSetter> dlg = new uiFirewallProcSetter( nullptr,
					    opertype, &path, &pythonpath );
    if ( !dlg->hasWorkToDo() )
	return 0;

    app.setTopLevel( dlg );
    dlg->show();

    return app.exec();
}
