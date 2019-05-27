/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : May 2019
-*/

#include "commandlineparser.h"
#include "dbman.h"
#include "filepath.h"
#include "oddirs.h"
#include "oscommand.h"
#include "serverprogtool.h"

static const int cProtocolNr = 1;

static const char* sAddCmd	= "add";
static const char* sRemoveCmd	= "remove";
static const char* sAddOSCmd()	{ return "netsh firewall add allowedprogram "; }
static const char* sDelOSCmd()
			    { return "netsh firewall delete allowedprogram "; }
/*
Command --add p <procnm1> <procnm2> :  In case process is python related
Command --add o <procnm1> <procnm2> :  In case process is opendtect related
*/
class SetUpFireWallServerTool : public ServerProgTool
{
public :
			SetUpFireWallServerTool(int,char**);
    bool		addProcess(const BufferString&);
    bool		removeProcess(const BufferString&);
protected :
    BufferString	getSpecificUsage() const override;
    BufferStringSet	processnmset_;
};


SetUpFireWallServerTool::SetUpFireWallServerTool( int argc, char** argv )
    : ServerProgTool(argc,argv,"General")
{
    initParsing( cProtocolNr );
}

//TODO: add a function to create full path for python and od


bool SetUpFireWallServerTool::addProcess( const BufferString& procnm )
{
    BufferString exenm = procnm;
    exenm.add(".exe");
    BufferString cmd = sAddOSCmd();
    cmd.addSpace().add( exenm ).addSpace().add( procnm ).addSpace()
								.add("ENABLE" );
    OS::MachineCommand mchcmd( cmd );

    return mchcmd.execute( cmd );
}


bool SetUpFireWallServerTool::removeProcess( const BufferString& procnm )
{
    return false;
}

BufferString SetUpFireWallServerTool::getSpecificUsage() const
{
    return BufferString::empty();
}


int main( int argc, char** argv )
{
    SetUpFireWallServerTool progtool( argc, argv );
    auto& clp = progtool.clp();

    BufferStringSet procnms;
    clp.getNormalArguments( procnms ); //idx=0 correspond to OD/Python env
    for ( int procidx=1; procidx<procnms.size(); procidx++ )
    {
	if ( clp.hasKey(sAddCmd) )
	    progtool.addProcess( procnms.get(procidx) );
	else
	    progtool.removeProcess( procnms.get( procidx ) );
    }
}
