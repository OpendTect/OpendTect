/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2018
-*/


#include "dbman.h"
#include "ascstream.h"
#include "commandlineparser.h"
#include "dbdir.h"
#include "filepath.h"
#include "moddepmgr.h"
#include "od_ostream.h"
#include "odver.h"
#include "prog.h"
#include "survinfo.h"

static const char* sStatusCmd		= "status";
static const char* sDataRootCmd		= "dataroot";
static const char* sSurveyCmd		= "survey";
static const char* sListCmd		= "list";
static const char* sInfoCmd		= "info";
static const char* sCreateCmd		= "edit";
static const char* sRemoveCmd		= "remove";
static const char* sVersionCmd		= "version";

static const char* cmds[] =
{
    sStatusCmd,
    sDataRootCmd,
    sSurveyCmd,
    sListCmd,
    sInfoCmd,
    sCreateCmd,
    sRemoveCmd,
    0
};


static IOPar ret_;
static const char* sErrKey = "ERR";


static od_ostream& strm()
{
    return od_ostream::logStream();
}


static int respond( bool success )
{
    ret_.set( "Status", success ? "OK" : "Fail" );
    ascostream ascstrm( strm() );
    ret_.putTo( ascstrm );
    return ExitProgram( success ? 0 : 1 );
}


static int printUsage()
{
    BufferString errmsg( "Please specify " );
    BufferStringSet nms( cmds );
    for ( auto nm : nms )
	errmsg.add( "--" ).add( *nm ).add( "," );
    errmsg.add( " or --version" );
    ret_.set( sErrKey, errmsg );
    return respond( false );
}


static ConstRefMan<DBDir> getDBDir( const char* trgrpnm, bool okiffail=false )
{
    auto ret = DBM().findDir( trgrpnm );
    if ( !ret )
	respond( okiffail );
    return ret;
}


static void listObjs( const char* trgrpnm )
{
    ret_.set( "Size", 0 ); // in case DBDir not found

    auto dbdir = getDBDir( trgrpnm, true );
    DBDirIter it( *dbdir );
    BufferStringSet nms, types; DBKeySet ids;
    while ( it.next() )
    {
	if ( it.ioObj().group() == trgrpnm )
	{
	    nms.add( it.ioObj().name() );
	    ids.add( it.ioObj().key() );
	    types.add( it.ioObj().pars().find("Type") );
	}
    }

    ret_.set( "Size", ids.size() );
    ret_.set( "Names", nms );
    ret_.set( "IDs", ids );
    ret_.set( "Types", types );

    respond( true );
}


static void provideInfo( const DBKey& dbkystr )
{
    ret_.set( sErrKey, "provideInfo not impl yet" );
    respond( false );
}


static void removeObj( const DBKey& dbkystr )
{
    ret_.set( sErrKey, "removeObj not impl yet" );
    respond( false );
}


static void createObj( const BufferStringSet& args )
{
    ret_.set( sErrKey, "createObj not impl yet" );
    respond( false );
}


int main( int argc, char** argv )
{
    OD::SetRunContext( OD::BatchProgCtxt );
    SetProgramArgs( argc, argv );
    OD::ModDeps().ensureLoaded( "General" );
    CommandLineParser clp;
    if ( clp.nrArgs() < 1 )
	return printUsage();
    else if ( clp.hasKey( sVersionCmd ) )
    {
	strm() << GetFullODVersion() << od_endl;
	return ExitProgram( 0 );
    }

    const bool setdataroot = clp.hasKey( sDataRootCmd );
    const bool setsurvey = clp.hasKey( sSurveyCmd );
    if ( setdataroot || setsurvey )
    {
	BufferString survnm( SI().name() );
	if ( setsurvey )
	{
	    clp.setKeyHasValue( sSurveyCmd, 1 );
	    clp.getVal( sSurveyCmd, survnm );
	}
	BufferString dataroot( SI().diskLocation().fullPath() );
	if ( setdataroot )
	{
	    clp.setKeyHasValue( sDataRootCmd, 1 );
	    clp.getVal( sDataRootCmd, dataroot );
	}
	File::Path fp( dataroot, survnm );
	uiRetVal uirv = DBM().setDataSource( fp.fullPath() );
	if ( !uirv.isOK() )
	{
	    ret_.set( sErrKey, toString(uirv) );
	    return respond( false );
	}
    }

    const bool isbad = DBM().isBad();
    if ( isbad || clp.hasKey( sStatusCmd ) )
    {
	if ( isbad )
	    ret_.set( sErrKey, "Data Store cannot be initialised" );
	return respond( !isbad );
    }

    if ( clp.hasKey( sListCmd ) )
    {
	clp.setKeyHasValue( sListCmd, 1 );
	BufferString trgrpnm;
	clp.getVal( sListCmd, trgrpnm );
	listObjs( trgrpnm );
    }
    else if ( clp.hasKey( sInfoCmd ) )
    {
	clp.setKeyHasValue( sInfoCmd, 1 );
	BufferString dbkystr;
	clp.getVal( sListCmd, dbkystr );
	provideInfo( DBKey(dbkystr) );
    }
    else if ( clp.hasKey( sRemoveCmd ) )
    {
	clp.setKeyHasValue( sRemoveCmd, 1 );
	BufferString dbkystr;
	clp.getVal( sRemoveCmd, dbkystr );
	removeObj( DBKey(dbkystr) );
    }

    const int cridx = clp.indexOf( sCreateCmd );
    if ( cridx < 0 )
	return printUsage();

    BufferStringSet normargs;
    clp.getNormalArguments( normargs );
    if ( normargs.size() < 3 )
    {
	ret_.set( sErrKey,
		  "Create command takes 3 arguments: Name, Group, Translator" );
	return respond( false );
    }
    createObj( normargs );

    return ExitProgram( 0 );
}
