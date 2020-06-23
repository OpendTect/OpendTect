/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : May 2019
-*/

#include "commandlineparser.h"
#include "filepath.h"
#include "keystrs.h"
#include "oddirs.h"
#include "oscommand.h"
#include "pythonaccess.h"
#include "serverprogtool.h"
#include "settings.h"

static const int cProtocolNr = 1;

static const char* sAddStr	= "add";
static const char* sRemoveStr	= "remove";
/*
Command --add p <procnm1> <procnm2> :  In case process is python related
Command --add o <procnm1> <procnm2> :  In case process is opendtect related
Do not use .exe at the end of the procnms
*/
class SetUpFirewallServerTool : public ServerProgTool
{
public :
			SetUpFirewallServerTool(int,char**);
    bool		handleProcess(BufferString&,bool);
    bool		ispyproc_;

protected :
    BufferString	getSpecificUsage() const override;
    void		createDirPaths();

    File::Path		pypath_;
    File::Path		odpath_;
};


SetUpFirewallServerTool::SetUpFirewallServerTool( int argc, char** argv )
    : ServerProgTool(argc,argv,"General")
{
    initParsing( cProtocolNr );
    createDirPaths();
}


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
		!pythonsetts.get(OD::PythonAccess::sKeyEnviron(),pythonloc) )
	return;
    pypath_ = pythonloc;
    pypath_.add( "envs" );
}


bool SetUpFirewallServerTool::handleProcess( BufferString& procnm, bool toadd )
{
    BufferString exenm = procnm;
    File::Path fp( ispyproc_ ? pypath_ : odpath_ );

    if ( fp.isEmpty() )
	return false;

    if ( ispyproc_ )
	fp.add( procnm ).add( "python.exe" );
    else
    {
	exenm.add( ".exe" );
	fp.add( exenm );
    }

    OS::MachineCommand mc( "netsh", "advfirewall", "firewall" );
    mc.addArg( toadd ? "add" : "delete" )
       .addArg( "rule" )
       .addArg( BufferString("name=\"",procnm,"\"") )
       .addArg( BufferString("program=\"",fp.fullPath(),"\"") );
    if ( toadd )
	mc.addArg( "enable=yes" );

    return mc.execute( OS::LaunchType::RunInBG );
}


BufferString SetUpFirewallServerTool::getSpecificUsage() const
{
    return BufferString::empty();
}


int main( int argc, char** argv )
{
    SetUpFirewallServerTool progtool( argc, argv );
    auto& clp = progtool.clp();

    BufferStringSet procnms;
    clp.getNormalArguments( procnms ); //idx=0 correspond to OD/Python env
    progtool.ispyproc_ = procnms.get( 0 ) == "o" ? false : true;

    bool toadd = clp.hasKey( sAddStr );

    if ( !toadd && !clp.hasKey(sRemoveStr) )
	return 0;

    for ( int procidx=1; procidx<procnms.size(); procidx++ )
	progtool.handleProcess( procnms.get(procidx), toadd );
}
