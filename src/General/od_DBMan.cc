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
#include "iostrm.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "od_ostream.h"
#include "odver.h"
#include "prog.h"
#include "survinfo.h"

static const int protocolnr_ = 1;

static const char* sStatusCmd		= "status";
static const char* sDataRootCmd		= "dataroot";
static const char* sSurveyCmd		= "survey";
static const char* sListCmd		= "list";
static const char* sInfoCmd		= "info";
static const char* sCreateCmd		= "create";
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
	    if ( !it.ioObj().isTmp() && it.ioObj().group() == trgrpnm )
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

    ret_.set( sKey::Size(), ids.size() );
    if ( !ids.isEmpty() )
    {
	ret_.set( sKey::ID(mPlural), ids );
	ret_.set( sKey::Name(mPlural), nms );
	ret_.set( sKey::Format(mPlural), trls );
	if ( havetype )
	    ret_.set( sKey::Type(mPlural), types );
    }

    respond( true );
}


static void provideInfo( const DBKey& dbky )
{
    PtrMan<IOObj> ioobj = getIOObj( dbky );
    if ( !ioobj )
	mRespondErr( "Input object key not found" )

    ret_.set( sKey::ID(), ioobj->key() );
    ret_.set( sKey::Name(), ioobj->name() );
    ret_.set( sKey::Format(), ioobj->translator() );
    ret_.set( sKey::FileName(), ioobj->mainFileName() );
    const char* typstr = ioobj->pars().find( sKey::Type() );
    if ( typstr && *typstr )
	ret_.set( sKey::Type(), typstr );

    respond( true );
}


static void removeObj( const DBKey& dbky )
{
    respond( DBM().removeEntry(dbky) );
}


static void createObj( const BufferStringSet& args )
{
    if ( args.size() < 5 )
	mRespondErr( "Specify at least name, dirid, trgrp, trl, ext. "
		     "Optional, type." )
    auto dbdir = DBM().fetchDir( DBKey::DirID(toInt(args.get(1))) );
    if ( !dbdir )
	mRespondErr( "Invalid Dir ID specified" )

    IOStream iostrm( args.get(0) );
    iostrm.setKey( dbdir->newKey() );
    iostrm.setGroup( args.get(2) );
    iostrm.setTranslator( args.get(3) );
    iostrm.setExt( args.get(4) );
    if ( args.size() > 5 )
	iostrm.pars().set( sKey::Type(), args.get(5) );
    iostrm.genFileName();

    DBDir* dbdirptr = mNonConst( dbdir.ptr() );
    if ( !dbdirptr->commitChanges(iostrm) )
	mRespondErr( "Cannot commit new entry to data store" )

    ret_.set( sKey::ID(), iostrm.key() );
    ret_.set( sKey::FileName(), iostrm.mainFileName() );
    respond( true );
}


int main( int argc, char** argv )
{
    OD::SetRunContext( OD::BatchProgCtxt );
    SetProgramArgs( argc, argv );
    OD::ModDeps().ensureLoaded( "General" );
    CommandLineParser clp;
    ret_.set( "Status", "Fail" ); // make sure it will be the first entry
    if ( clp.nrArgs() < 1 )
	return printUsage();
    else if ( clp.hasKey( sVersionCmd ) )
    {
	strm() << protocolnr_ << "@" << GetFullODVersion() << od_endl;
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
    createObj( normargs );

    return ExitProgram( 0 );
}
