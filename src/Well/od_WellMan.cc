/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2018
-*/


#include "serverprogtool.h"
#include "wellmanager.h"
#include "welltrack.h"
#include "welllog.h"
#include "commandlineparser.h"
#include "keystrs.h"
#include "odjson.h"
#include "prog.h"
#include "scaler.h"
#include "unitofmeasure.h"


static const int cProtocolNr = 1;

static const char* sListWellsCmd	= "list";
static const char* sListLogsCmd		= "list-logs";
static const char* sReadLogCmd		= "read-log";
static const char* sNoTVDCmd		= "no-tvd";

class WellServerTool : public ServerProgTool
{
public:

		    WellServerTool(int,char**);

    void	    listWells();
    void	    listLogs(const DBKey&);
    void	    readLog(const DBKey&,const char*,bool notvd);

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
    BufferStringSet nms;
    for ( auto wellid : wellids )
	nms.add( nameOf(*wellid) );
    set( sKey::Name(mPlural), nms );
    respondInfo( true );
}


void WellServerTool::listLogs( const DBKey& wellid )
{
    BufferStringSet lognms;
    Well::MGR().getLogNames( wellid, lognms );
    set( sKey::ID(), wellid );
    set( sKey::Name(mPlural), lognms );
    respondInfo( true );
}


void WellServerTool::readLog( const DBKey& wellid, const char* lognm,
			      bool notvd )
{
    auto wl = Well::MGR().getLog( wellid, lognm );
    if ( !wl )
	respondError( "Log not found" );

    ConstRefMan<Well::Data> wd;
    if ( !notvd )
    {
	Well::LoadReqs loadreqs( Well::Inf, Well::Trck );
	wd = Well::MGR().fetch( wellid );
    }

    const auto sz = wl->size();
    set( sKey::Well(), nameOf(wellid) );
    set( sKey::Name(), wl->name() );
    set( sKey::Size(), wl->size() );
    const auto* uom = wl->unitOfMeasure();
    set( sKey::Unit(), uom ? uom->symbol() : "" );
    set( sKey::Scale(), uom ? uom->scaler().scale(1) : double(1) );

    TypeSet<float> mds, vals, tvds;
    const Well::Track* trck = wd ? &wd->track() : nullptr;
    for ( auto idx=0; idx<sz; idx++ )
    {
	const auto md = wl->dahByIdx( idx );
	mds += md;
	vals += wl->valueByIdx( idx );
	if ( trck )
	    tvds += trck->valueAt( md );
    }
    set( sKey::MD(mPlural), mds );
    set( sKey::Value(mPlural), vals );
    if ( trck )
	set( sKey::TVD(mPlural), tvds );

    respondInfo( true );
}


BufferString WellServerTool::getSpecificUsage() const
{
    BufferString ret;
    addToUsageStr( ret, sListWellsCmd, "" );
    addToUsageStr( ret, sListLogsCmd, "well_id" );
    BufferString argstr( "well_id log_name [", sNoTVDCmd, "]" );
    addToUsageStr( ret, sReadLogCmd, argstr );
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
	const bool notvd = clp.hasKey( sNoTVDCmd );
	st.readLog( wellid, lognm, notvd );
    }

    pFreeFnErrMsg( "Should not reach" );
    return ExitProgram( 0 );
}
