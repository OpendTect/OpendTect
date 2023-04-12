/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mmprocmod.h"

#include "commandlineparser.h"
#include "filepath.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "oddirs.h"
#include "oscommand.h"
#include "pythonaccess.h"
#include "settings.h"
#include "winutils.h"

#include "prog.h"

static const char* sAddStr	= "add";
static const char* sRemoveStr	= "remove";
static const char* sODStr	= "od";

/*
Command --add --py <procnm1.exe> <procnm2.exe> : Python related
Command --add --od <procnm1.exe> <procnm2.exe> :  OpendTect related
*/

class SetUpFirewallServerTool
{
public:
			SetUpFirewallServerTool()
			{ createDirPaths(); }
    bool		handleProcess(BufferString&, bool);
    bool		ispyproc_;
    void		updateDirPath(const char*);

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

    if ( !pythonsource ||
	!pythonsetts.get(OD::PythonAccess::sKeyEnviron(), pythonloc) )
	return;

    pypath_ = pythonloc;
    pypath_.add( "envs" );
}


void SetUpFirewallServerTool::updateDirPath( const char* dirnm )
{
    if ( ispyproc_ )
    {
	pypath_ = FilePath::getLongPath( dirnm );
	pypath_.add( "envs" );
    }
    else
	odpath_ = FilePath::getLongPath( dirnm );
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

    OS::MachineCommand mc( "netsh", "advfirewall", "firewall" );
    mc.addArg( toadd ? "add" : "delete" )
       .addArg( "rule" )
       .addArg( BufferString("name=\"",procnm,"\"") );
    const BufferString shortpath = FilePath::getShortPath( fp.fullPath().buf());
    mc.addArg( BufferString("program=\"",shortpath.buf(),"\"") );
    if ( toadd )
    {
	mc.addArg( "enable=yes" );
	mc.addArg( "dir=in" );
	mc.addArg( "action=allow" );
    }

    OS::CommandExecPars pars( OS::RunInBG );
#ifdef __win__
    pars.runasadmin( !WinUtils::IsUserAnAdmin() );
#endif
    return mc.execute( pars );
}


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::BatchProgCtxt )
    SetProgramArgs( argc, argv, false );
    OD::ModDeps().ensureLoaded( "Network" );

    CommandLineParser parser( argc, argv );

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "MMProc" );
    PIM().loadAuto( true );

    SetUpFirewallServerTool progtool;
    BufferStringSet procnms;
    parser.getNormalArguments( procnms );
    const bool ispyproc = parser.hasKey( sODStr ) ? false : true;
    progtool.ispyproc_ = ispyproc;
    const bool toadd = parser.hasKey( sAddStr );
    if ( !toadd && !parser.hasKey(sRemoveStr) )
	return 1;

    for ( int procidx=0; procidx<procnms.size(); procidx++ )
    {
	if ( procidx==0 ) // index 0 stores path
	{
	    progtool.updateDirPath( procnms.get(procidx) );
	    continue;
	}

	progtool.handleProcess( procnms.get(procidx), toadd );
    }

    return 0;
}
