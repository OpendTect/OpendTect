/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2018
-*/


#include "serverprogtool.h"
#include "wellmanager.h"
#include "commandlineparser.h"
#include "keystrs.h"
#include "odjson.h"
#include "prog.h"


static const int cProtocolNr = 1;

static const char* sListWellsCmd	= "list";
static const char* sListLogsCmd		= "list-logs";
static const char* sReadLogCmd		= "read-log";

class WellServerTool : public ServerProgTool
{
public:

		    WellServerTool(int,char**);

    void	    listWells();
    void	    listLogs(const DBKey&);
    void	    readLog(const DBKey&,const char*);

protected:

    BufferString    getSpecificUsage() const override;

};


WellServerTool::WellServerTool( int argc, char** argv )
    : ServerProgTool(argc,argv,"Well")
{
    initParsing( cProtocolNr );
}


void WellServerTool::listWells()
{
    DBKeySet wellids;
    Well::MGR().getAll( wellids, false );
    set( sKey::ID(mPlural), wellids );
    respondInfo( true );
}


void WellServerTool::listLogs( const DBKey& wellid )
{
    BufferStringSet lognms;
    Well::MGR().getLogNames( wellid, lognms );
    set( sKey::Name(mPlural), lognms );
    respondInfo( true );
}


void WellServerTool::readLog( const DBKey& wellid, const char* lognm )
{
    auto wl = Well::MGR().getLog( wellid, lognm );
    if ( !wl )
	respondError( "Log not found" );

    respondInfo( true );
}


BufferString WellServerTool::getSpecificUsage() const
{
    BufferString ret;
    addToUsageStr( ret, sListWellsCmd, "" );
    addToUsageStr( ret, sListLogsCmd, "well_id" );
    addToUsageStr( ret, sReadLogCmd, "well_id log_name" );
    return ret;
}


int main( int argc, char** argv )
{
    WellServerTool st( argc, argv );
    auto& clp = st.clp();

    if ( clp.hasKey(sListWellsCmd) )
	st.listWells();

    DBKey wellid;
    if ( clp.hasKey(sListLogsCmd) )
    {
	clp.setKeyHasValue( sListLogsCmd, 1 );
	clp.getDBKey( sListLogsCmd, wellid );
	st.listLogs( wellid );
    }
    else if ( clp.hasKey(sReadLogCmd) )
    {
	clp.setKeyHasValue( sReadLogCmd, 2 );
	clp.getDBKey( sReadLogCmd, wellid );
	BufferString lognm;
	clp.getVal( sReadLogCmd, lognm, false, 2 );
	st.readLog( wellid, lognm );
    }

    pFreeFnErrMsg( "Should not reach" );
    return ExitProgram( 0 );
}
