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


#define mRespondErr(s) { ret_.set( sErrKey, s ); respond( false ); }


static void listObjs( const char* trgrpnm )
{
    BufferStringSet nms, types, trls; DBKeySet ids;
    bool havetype = false;
    auto dbdir = DBM().findDir( trgrpnm );
    if ( dbdir )
    {
	DBDirIter it( *dbdir );
	while ( it.next() )
	{
	    if ( it.ioObj().group() == trgrpnm )
	    {
		nms.add( it.ioObj().name() );
		ids.add( it.ioObj().key() );
		trls.add( it.ioObj().translator() );
		BufferString typ( it.ioObj().pars().find("Type") );
		typ.remove( ' ' );
		if ( !typ.isEmpty() )
		    { havetype = true; typ.replace( '`', '|' ); }
		types.add( typ );
	    }
	}
    }

    ret_.set( "Size", ids.size() );
    if ( !ids.isEmpty() )
    {
	ret_.set( "IDs", ids );
	ret_.set( "Names", nms );
	ret_.set( "Formats", trls );
	if ( havetype )
	    ret_.set( "Types", types );
    }

    respond( true );
}


static void provideInfo( const DBKey& dbky )
{
    PtrMan<IOObj> ioobj = getIOObj( dbky );
    if ( !ioobj )
	mRespondErr( "Input object key not found" )

    ret_.set( "ID", ioobj->key() );
    ret_.set( "Name", ioobj->name() );
    ret_.set( "Format", ioobj->translator() );
    ret_.set( "File name", ioobj->mainFileName() );

    respond( true );
}


static void removeObj( const DBKey& dbkystr )
{
    mRespondErr( "removeObj not impl yet" )
}


static void createObj( const BufferStringSet& args )
{
    mRespondErr( "createObj not impl yet" )
}


int main( int argc, char** argv )
{
    OD::SetRunContext( OD::BatchProgCtxt );
    SetProgramArgs( argc, argv );
    OD::ModDeps().ensureLoaded( "General" );
    CommandLineParser clp;
    ret_.set( "Status", "Fail" ); // make this first entry in IOPar
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
	File::Path fpdr( SI().diskLocation().fullPath() );
	BufferString dataroot( fpdr.pathOnly() );
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
	clp.getVal( sInfoCmd, dbkystr );
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
