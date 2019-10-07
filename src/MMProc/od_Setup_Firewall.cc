/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : May 2019
-*/

#include "mmprocmod.h"

#include "commandlineparser.h"
#include "filepath.h"
#include "keystrs.h"
#include "oddirs.h"
#include "oscommand.h"
#include "pythonaccess.h"
#include "settings.h"

static const int cProtocolNr = 1;

static const char* sAddStr	= "add";
static const char* sRemoveStr	= "remove";
static const char* sPythonStr	= "py";
static const char* sODStr	= "od";

static uiString sAddOSCmd() {
    return toUiString("netsh advfirewall firewall "
	"add rule name=%1 dir=in action=allow program=%2 enable=yes");
}
static uiString sDelOSCmd() {
    return toUiString("netsh advfirewall firewall "
	"delete rule name=%1 program=%2");
}

/*
Command --add --py <procnm1.exe> <procnm2.exe> :  In case process is python related
Command --add --od <procnm1.exe> <procnm2.exe> :  In case process is opendtect related
*/

class SetUpFirewallServerTool
{
public:
			SetUpFirewallServerTool()
			{ createDirPaths(); }
    bool		handleProcess(BufferString&, bool);
    bool		ispyproc_;

protected:
    void		createDirPaths();

    FilePath		pypath_;
    FilePath		odpath_;

};


void SetUpFirewallServerTool::createDirPaths()
{
    odpath_ = GetExecPlfDir();
    BufferString strcheck = odpath_.fullPath();

    BufferString pythonstr( sKey::Python() ); pythonstr.toLower();
    const IOPar& pythonsetts = Settings::fetch( pythonstr );
    OD::PythonSource source;
    BufferString pythonloc;
    const bool pythonsource = OD::PythonSourceDef().parse( pythonsetts,
	OD::PythonAccess::sKeyPythonSrc(), source );

    if ( !pythonsource || source != OD::PythonSource::Custom ||
	!pythonsetts.get(OD::PythonAccess::sKeyEnviron(), pythonloc) )
	return;

    pypath_ = pythonloc;
    pypath_.add( "envs" );
}


bool SetUpFirewallServerTool::handleProcess( BufferString& procnm, bool toadd )
{
    BufferString exenm = procnm;
    FilePath fp( ispyproc_ ? pypath_ : odpath_ );

    if ( fp.isEmpty() )
	return false;

    if ( ispyproc_ )
	fp.add( procnm ).add( "python.exe" );
    else
    {
	exenm.add( ".exe" );
	fp.add( exenm );
    }

    uiString command = toadd ? sAddOSCmd() : sDelOSCmd();
    command.arg( procnm.quote('"') ).arg( fp.fullPath() );

    OS::MachineCommand cmd( command.getFullString() );

    OS::CommandExecPars pars;
    pars.launchtype( OS::LaunchType::RunInBG );
    pars.prioritylevel( 0 );

    OS::CommandLauncher cl( cmd );

    return cl.execute( pars );
}


int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );

    CommandLineParser parser;
    SetUpFirewallServerTool progtool;
    BufferStringSet procnms;
    parser.getNormalArguments( procnms );

    progtool.ispyproc_ = parser.hasKey( sODStr ) ? false : true;


    const bool toadd = parser.hasKey( sAddStr );

    if ( !toadd && !parser.hasKey(sRemoveStr) )
	return NULL;

    for ( int procidx=0; procidx<procnms.size(); procidx++ )
	progtool.handleProcess( procnms.get(procidx), toadd );

    return true;
}
