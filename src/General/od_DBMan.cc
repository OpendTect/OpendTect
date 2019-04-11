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
#include "odjson.h"
#include "odver.h"
#include "prog.h"
#include "survinfo.h"

#include <iostream>

static const int protocolnr_ = 1;

static const char* sStatusCmd		= "status";
static const char* sListCmd		= "list";
static const char* sListSurvCmd		= "list-surveys";
static const char* sInfoCmd		= "info";
static const char* sCreateCmd		= "create";
static const char* sRemoveCmd		= "remove";
static const char* sVersionCmd		= "version";
static const char* sFileNameCmd		= "filename";
static const char* sJsonOutput		= "json";

static const char* cmds[] =
{
    sStatusCmd,
    sListCmd,
    sListSurvCmd,
    sInfoCmd,
    sCreateCmd,
    sRemoveCmd,
    sJsonOutput,
    0
};


static IOPar ret_;
static OD::JSON::Object jret_;
static bool dojson_ = false;
static const char* sErrKey = "ERR";


static od_ostream& strm()
{
    return od_ostream::logStream();
}

#define mSet( keywd, res ) \
{ \
    if ( dojson_ && jobj ) \
    { \
	BufferString keyword( keywd ); \
	keyword.clean(); \
	jobj->set( keyword, res ); \
    } \
    else \
	ret_.set( keywd, res ); \
}

static int respond( bool success )
{
    auto* jobj = &jret_;
    mSet( "Status", success ? "OK" : "Fail" );
    if ( dojson_ )
    {
	od_ostream strm( std::cout );
	jret_.write( strm );
    }
    else
    {
	ascostream ascstrm( strm() );
	ret_.putTo( ascstrm );
    }
    return ExitProgram( success ? 0 : 1 );
}


static int printUsage()
{
    BufferString errmsg( "Please specify " );
    BufferStringSet nms( cmds );
    for ( auto nm : nms )
	errmsg.add( "--" ).add( *nm ).add( "," );
    errmsg.add( "--" ).add( CommandLineParser::sDataRootArg() ).add( "," );
    errmsg.add( "--" ).add( CommandLineParser::sSurveyArg() ).add( "," );
    errmsg.add( " or --version" );
    auto* jobj = &jret_;
    mSet( sErrKey, errmsg );
    return respond( false );
}


#define mRespondErr(s) { \
    auto* jobj = &jret_; \
    mSet( sErrKey, s ); \
    respond( false ); \
}


static void listSurveys()
{
    BufferStringSet dirnms;
    const File::Path survfp( DBM().survDir() );
    const BufferString dataroot( survfp.pathOnly() );
    SurveyDiskLocation::listSurveys( dirnms, dataroot );
    if ( !dirnms.isEmpty() )
    {
	auto* jobj = &jret_;
	mSet( sKey::DataRoot(), dataroot );
	if ( dojson_ )
	{
	    auto* arr = new OD::JSON::Array( true );
	    for ( int idx=0; idx<dirnms.size(); idx++ )
	    {
		jobj = new OD::JSON::Object;
		jobj->set( sKey::Name(), dirnms.get(idx) );
		arr->add( jobj );
	    }
	    jret_.set( "data", arr );
	}
	else
	{
	    ret_.set( sKey::Size(), dirnms.size() );
	    ret_.set( sKey::Name(mPlural), dirnms );
	}
    }

    respond( true );
}


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

    if ( dojson_ )
    {
	auto* arr = new OD::JSON::Array( true );
	for ( int idx=0; idx<ids.size(); idx++ )
	{
	    auto* jobj = new OD::JSON::Object;
	    jobj->set( sKey::ID(), ids.get(idx) );
	    jobj->set( sKey::Name(), nms.get(idx) );
	    jobj->set( sKey::Format(), trls.get(idx) );
	    if ( havetype )
		jobj->set( sKey::Type(), types.get(idx) );
	    arr->add( jobj );
	}
	jret_.set( "data", arr );
    }
    else
    {
	ret_.set( sKey::Size(), ids.size() );
	if ( !ids.isEmpty() )
	{
	    ret_.set( sKey::ID(mPlural), ids );
	    ret_.set( sKey::Name(mPlural), nms );
	    ret_.set( sKey::Format(mPlural), trls );
	    if ( havetype )
		ret_.set( sKey::Type(mPlural), types );
	}
    }

    respond( true );
}


static void provideInfo( const DBKey& dbky )
{
    PtrMan<IOObj> ioobj = getIOObj( dbky );
    if ( !ioobj )
	mRespondErr( "Input object key not found" )

    auto* jobj = dojson_ ? new OD::JSON::Object : 0;
    mSet( sKey::ID(), ioobj->key() );
    mSet( sKey::Name(), ioobj->name() );
    mSet( sKey::Format(), ioobj->translator() );
    mSet( sKey::FileName(), ioobj->mainFileName() );
    const char* typstr = ioobj->pars().find( sKey::Type() );
    if ( typstr && *typstr )
	mSet( sKey::Type(), typstr );

    jret_.set( "data", jobj );
    respond( true );
}


static void removeObj( const DBKey& dbky )
{
    respond( DBM().removeEntry(dbky) );
}


static void createObj( const BufferStringSet& args, const char* filenm )
{
    if ( args.size() < 5 )
	mRespondErr( "Specify at least name, dirid, trgrp, trl, ext. "
		     "Optional, type and/or --filename your_file_name." )
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

    iostrm.setAbsDirectory( dbdir->dirName() );
    if ( filenm && *filenm )
	iostrm.fileSpec().setFileName( filenm );
    else
	iostrm.genFileName();

    iostrm.updateCreationPars();
    DBDir* dbdirptr = mNonConst( dbdir.ptr() );
    if ( !dbdirptr->commitChanges(iostrm) )
	mRespondErr( "Cannot commit new entry to data store" )

    auto* jobj = dojson_ ? new OD::JSON::Object : 0;
    mSet( sKey::ID(), iostrm.key() );
    mSet( sKey::FileName(), iostrm.mainFileName() );
    jret_.set( "data", jobj );
    respond( true );
}


int main( int argc, char** argv )
{
    OD::SetRunContext( OD::BatchProgCtxt );
    SetProgramArgs( argc, argv );
    OD::ModDeps().ensureLoaded( "General" );
    CommandLineParser clp;
    auto* jobj = &jret_;
    mSet( "Status", "Fail" ); // make sure it will be the first entry
    if ( clp.nrArgs() < 1 )
	return printUsage();
    else if ( clp.hasKey( sVersionCmd ) )
    {
	strm() << protocolnr_ << "@" << GetFullODVersion() << od_endl;
	return ExitProgram( 0 );
    }

    dojson_ = clp.hasKey( sJsonOutput );

    uiRetVal uirv = DBM().setDataSource( clp );
    if ( !uirv.isOK() )
	{ mSet( sErrKey, toString(uirv) ); return respond( false ); }

    if ( clp.hasKey( sListSurvCmd ) )
	listSurveys();

    const bool isbad = DBM().isBad();
    if ( isbad || clp.hasKey( sStatusCmd ) )
    {
	if ( isbad )
	{
	    mSet( sErrKey, "Data Store cannot be initialised" );
	}
	else
	{
	    File::Path fp( DBM().survDir() );
	    mSet( sKey::Survey(), fp.fileName() );
	    mSet( sKey::DataRoot(), fp.pathOnly() );
	}
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

    clp.setKeyHasValue( sFileNameCmd, 1 );
    BufferString filenm;
    clp.getVal( sFileNameCmd, filenm );

    BufferStringSet normargs;
    clp.getNormalArguments( normargs );
    createObj( normargs, filenm );

    return ExitProgram( 0 );
}
