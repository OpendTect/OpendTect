/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2019
________________________________________________________________________

-*/

#include "serverprogtool.h"
#include "ascstream.h"
#include "dbman.h"
#include "dbkey.h"
#include "genc.h"
#include "moddepmgr.h"
#include "odjson.h"
#include "odver.h"
#include "od_ostream.h"
#include <iostream>

static const char* sVersionCmd = "version";
static const char* sUseJSONCmd = "json";
static const char* sDontUseJSONCmd = "nojson";
static const char* sErrKey = "ERR";


static od_ostream& strm()
{
    return od_ostream::logStream();
}


ServerProgTool::ServerProgTool( int argc, char** argv,
				const char* moddep, bool jsonmode )
    : jsonroot_(*new JSONObject)
    , jsonisdefault_(jsonmode)
    , jsonmode_(jsonmode)
{
    OD::SetRunContext( OD::BatchProgCtxt );
    SetProgramArgs( argc, argv );
    OD::ModDeps().ensureLoaded( moddep );
    clp_ = new CommandLineParser;
}


void ServerProgTool::initParsing( int protnr )
{
    protocolnr_ = protnr;
    if ( clp().nrArgs() < 1 )
	exitWithUsage();
    else if ( clp().hasKey( sVersionCmd ) )
    {
	od_cout() << GetFullODVersion() << " (" << protocolnr_ << ")"
		  << od_endl;
	exitProgram( true );
    }

    if ( clp().hasKey(sUseJSONCmd) )
	jsonmode_ = true;
    else if ( clp().hasKey(sDontUseJSONCmd) )
	jsonmode_ = false;

    setStatus( false ); // making sure this is the first entry

    uiRetVal uirv = DBM().setDataSource( clp() );
    if ( !uirv.isOK() )
	respondError( toString(uirv) );
}


ServerProgTool::~ServerProgTool()
{
    exitProgram( true );
}


template <class T>
void ServerProgTool::setSingle( const char* keyw, T val, JSONObject* jobj )
{
    if ( !jsonmode_ )
	iop_.set( keyw, val );
    else
    {
	if ( !jobj )
	    jobj = &jsonroot_;

	BufferString cleankeyw( keyw ); cleankeyw.clean();
	jobj->set( cleankeyw, val );
    }
}


void ServerProgTool::set( const char* keyw, const char* val, JSONObject* jobj )
{ setSingle( keyw, val, jobj ); }
void ServerProgTool::set( const char* keyw, const DBKey& val, JSONObject* jobj )
{ setSingle( keyw, toString(val), jobj ); }
void ServerProgTool::set( const char* keyw, int val, JSONObject* jobj )
{ setSingle( keyw, val, jobj ); }
void ServerProgTool::set( const char* keyw, float val, JSONObject* jobj )
{ setSingle( keyw, val, jobj ); }
void ServerProgTool::set( const char* keyw, double val, JSONObject* jobj )
{ setSingle( keyw, val, jobj ); }


void ServerProgTool::set( const char* keyw, const BufferStringSet& vals,
			  JSONObject* jobj )
{
    if ( !jsonmode_ )
	iop_.set( keyw, vals );
    else
    {
	if ( !jobj )
	    jobj = &jsonroot_;

	auto* arr = new JSONArray( false );
	arr->set( vals );

	BufferString cleankeyw( keyw ); cleankeyw.clean();
	jobj->set( cleankeyw, arr );
    }
}


void ServerProgTool::set( const char* keyw, const DBKeySet& ids,
			  JSONObject* jobj )
{
    BufferStringSet vals;
    for ( auto dbky : ids )
	vals.add( dbky->toString() );
    set( keyw, vals, jobj );
}


void ServerProgTool::set( const char* keyw, JSONObject* jobj )
{
    jsonroot_.set( keyw, jobj );
}


void ServerProgTool::set( const char* keyw, JSONArray* jarr )
{
    jsonroot_.set( keyw, jarr );
}


void ServerProgTool::setStatus( bool success )
{
    set( "Status", success ? "OK" : "Fail" );
}


void ServerProgTool::respondInfo( bool success, bool exit )
{
    setStatus( success );
    if ( jsonmode_ )
    {
	od_ostream strm( std::cout );
	jsonroot_.write( strm );
    }
    else
    {
	ascostream ascstrm( strm() );
	iop_.putTo( ascstrm );
    }

    if ( exit )
	exitProgram( success );
}


void ServerProgTool::respondError( const char* msg )
{
    set( sErrKey, msg );
    respondInfo( false );
}


void ServerProgTool::addToUsageStr( BufferString& str,
			const char* flg, const char* args, bool isextra )
{
    const bool isfirst = str.isEmpty();
    str.add( "\n  " );
    if ( isextra )
	str.add( "[--" );
    else
	str.add( isfirst ? "   --" : "|| --" );
    str.add( flg );
    if ( args && *args )
	str.add( " " ).add( args );
    if ( isextra )
	str.add( "]" );
}


void ServerProgTool::exitWithUsage()
{
    BufferString msg( "Usage: ", GetExecutableName() );
    msg.add( getSpecificUsage() );
    addToUsageStr( msg, CommandLineParser::sDataRootArg(), "data_root_dir",
		    true );
    addToUsageStr( msg, CommandLineParser::sSurveyArg(), "survey_dir", true );
    BufferString jsonmsg( sUseJSONCmd, "|", sDontUseJSONCmd );
    jsonmsg.add( " (default=" )
	    .add( jsonisdefault_ ? sUseJSONCmd : sDontUseJSONCmd ).add( ")" );
    addToUsageStr( msg, jsonmsg, "", true );
    od_cout() << msg << od_endl;
    exitProgram( false );
}


void ServerProgTool::exitProgram( bool success )
{
    ExitProgram( success ? 0 : 1 );
}
