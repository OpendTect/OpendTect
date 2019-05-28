/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : May 2019
-*/

#include "commandlineparser.h"
#include "dbman.h"
#include "filepath.h"
#include "keystrs.h"
#include "oddirs.h"
#include "oscommand.h"
#include "pythonaccess.h"
#include "serverprogtool.h"
#include "settings.h"

static const int cProtocolNr = 1;

static const char* sAddCmd	= "add";
static const char* sRemoveCmd	= "remove";
static const char* sAddOSCmd()	{ return "netsh firewall add allowedprogram "; }
static const char* sDelOSCmd()
			    { return "netsh firewall delete allowedprogram "; }
/*
Command --add p <procnm1> <procnm2> :  In case process is python related
Command --add o <procnm1> <procnm2> :  In case process is opendtect related
Do not use .exe at the end of the procnms
*/
class SetUpFirewallServerTool : public ServerProgTool
{
public :
			SetUpFirewallServerTool(int,char**);
    bool		addProcess(const BufferString&);
    bool		removeProcess(const BufferString&);
    bool		ispyproc_;

protected :
    BufferString	getSpecificUsage() const override;
    void		createDirPaths();

    BufferStringSet	processnmset_;
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


bool SetUpFirewallServerTool::addProcess( const BufferString& procnm )
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

    OS::MachineCommand cmd = sAddOSCmd();
    BufferString path = fp.fullPath();
    path.quote( '"' );
    cmd.addArg( path ); cmd.addArg( procnm ); cmd.addArg( "ENABLE" );
    BufferString erroutput;
    BufferString command = cmd.getSingleStringRep();
    OS::CommandExecPars pars;
    pars.launchtype( OS::LaunchType::RunInBG );
    pars.prioritylevel( 1 );
    cmd.execute( pars );

    return true;
}


bool SetUpFirewallServerTool::removeProcess( const BufferString& procnm )
{
    return false;
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

    for ( int procidx=1; procidx<procnms.size(); procidx++ )
    {
	if ( clp.hasKey( sAddCmd ) )
	    progtool.addProcess( procnms.get(procidx) );
	else
	    progtool.removeProcess( procnms.get( procidx ) );
    }
}
