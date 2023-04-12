/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "serverprogtool.h"

#include "applicationdata.h"
#include "ascstream.h"
#include "dbman.h"
#include "dbkey.h"
#include "envvars.h"
#include "genc.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "odjson.h"
#include "odver.h"
#include "od_ostream.h"
#include "plugins.h"
#include "timer.h"

#include <iostream>

static const char* sVersionCmd = "version";
static const char* sDontUseJSONCmd = "nojson";
static const char* sErrKey = "ERR";


ServerProgTool::ServerProgTool( int argc, char** argv, const char* moddep )
    : jsonroot_(*new JSONObject)
    , jsonmode_(true)
    , timer_(*new Timer("Server tool"))
{
    if ( !ApplicationData::hasInstance() )
	DBG::forceCrash( false ); // There should be a QApplication instance

    OD::SetRunContext( OD::BatchProgCtxt );
    SetProgramArgs( argc, argv );
    OD::ModDeps().ensureLoaded( "General" );
    SetEnvVar( "OD_DISABLE_APPLOCKER_TEST", "Yes" );

    clp_ = new CommandLineParser( argc, argv );
    if ( !clp_->hasKey(sListSurvCmd()) )
    {
	const uiRetVal uirv = IOMan::setDataSource_( *clp_ );
	if ( !uirv.isOK() )
	    od_cout() << uirv.getText() << od_endl;
    }

    PIM().loadAuto( false );
    if ( moddep && *moddep )
	OD::ModDeps().ensureLoaded( moddep );

    PIM().loadAuto( true );

    mAttachCB( timer_.tick, ServerProgTool::timerTickCB );
}


ServerProgTool::~ServerProgTool()
{
    detachAllNotifiers();
    delete &timer_;
    delete &jsonroot_;
    delete clp_;
}


void ServerProgTool::initParsing( int protnr, bool setdatasrc )
{
    protocolnr_ = protnr;

    if ( clp().nrArgs() < 1 )
	exitWithUsage();
    else if ( clp().hasKey(sVersionCmd) )
    {
	od_cout() << GetFullODVersion() << " (" << protocolnr_ << ")"
		  << od_endl;
	exitProgram( true );
    }
    else if ( clp().hasKey(sDontUseJSONCmd) )
	jsonmode_ = false;

    setStatus( false ); // making sure this is the first entry

    if ( setdatasrc )
	setDBMDataSource();
}


void ServerProgTool::setDBMDataSource()
{
    const uiRetVal uirv = IOMan::setDataSource_( clp() );
    if ( !uirv.isOK() )
	respondError( toString(uirv) );
}


od_istream& ServerProgTool::inStream() const
{
    return od_cin();
}


od_ostream& ServerProgTool::outStream() const
{
    return od_cout();
}


BufferString ServerProgTool::getKeyedArgStr( const char* ky,
					     bool mandatory ) const
{
    const_cast<ServerProgTool*>(this)->clp().setKeyHasValue( ky );
    BufferString res;
    if ( !clp().getVal(ky,res) && mandatory )
	respondError( BufferString("Please provide a value after '",ky,"'") );
    return res;
}


DBKey ServerProgTool::getDBKey( const char* ky, bool mandatory ) const
{
    DBKey ret;
    ret.fromString( getKeyedArgStr(ky,mandatory) );
    if ( mandatory && !ret.isValid() )
	respondError( BufferString("Invalid key provided for '",ky,"'") );
    return ret;
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


template <class T>
void ServerProgTool::setArr( const char* keyw, const T* vals, size_type sz,
			     JSONObject* jobj )
{
    const TypeSet<T> valset( vals, sz );
    if ( !jsonmode_ )
    {
	iop_.set( keyw, valset );
    }
    else
    {
	if ( !jobj )
	    jobj = &jsonroot_;

	BufferString cleankeyw( keyw ); cleankeyw.clean();
	OD::JSON::Array* arr = jobj->set( cleankeyw, new JSONArray(false) );
	arr->set( valset );
    }
}


template <class T>
void ServerProgTool::setArr( const char* keyw, const T& valset,
			     JSONObject* jobj )
{
    if ( !jsonmode_ )
	iop_.set( keyw, valset );
    else
    {
	if ( !jobj )
	    jobj = &jsonroot_;

	BufferString cleankeyw( keyw ); cleankeyw.clean();
	OD::JSON::Array* arr = jobj->set( cleankeyw, new JSONArray(false) );
	arr->set( valset );
    }
}


#define mDefServerProgToolSetSingleFn( typ, arg, val ) \
    void ServerProgTool::set( const char* keyw, typ arg, JSONObject* jobj ) \
    { setSingle( keyw, val, jobj ); }

mDefServerProgToolSetSingleFn( const char*, str, str )
mDefServerProgToolSetSingleFn( const DBKey&, dbky, dbky.toString(true) )
mDefServerProgToolSetSingleFn( bool, val, val )
mDefServerProgToolSetSingleFn( od_int16, val, val )
mDefServerProgToolSetSingleFn( od_uint16, val, val )
mDefServerProgToolSetSingleFn( od_int32, val, val )
mDefServerProgToolSetSingleFn( od_uint32, val, val )
mDefServerProgToolSetSingleFn( od_int64, val, val )
mDefServerProgToolSetSingleFn( float, val, val )
mDefServerProgToolSetSingleFn( double, val, val )



#define mDefServerProgToolSetSetFn( typ ) \
    void ServerProgTool::set( const char* keyw, const typ& val, \
			      JSONObject* jobj ) \
    { setArr( keyw, val, jobj ); }

#define mDefServerProgToolSetArrFn( typ ) \
    void ServerProgTool::set( const char* keyw, const typ* val, size_type sz, \
			      JSONObject* jobj ) \
    { setArr( keyw, val, sz, jobj ); }

#define mDefServerProgToolSetFns( typ ) \
    mDefServerProgToolSetSetFn( TypeSet<typ> ) \
    mDefServerProgToolSetArrFn( typ )


mDefServerProgToolSetSetFn( BufferStringSet )
mDefServerProgToolSetSetFn( DBKeySet )
mDefServerProgToolSetSetFn( BoolTypeSet )
mDefServerProgToolSetFns( od_int16 )
mDefServerProgToolSetFns( od_uint16 )
mDefServerProgToolSetFns( od_int32 )
mDefServerProgToolSetFns( od_uint32 )
mDefServerProgToolSetFns( od_int64 )
mDefServerProgToolSetFns( float )
mDefServerProgToolSetFns( double )


void ServerProgTool::set( const char* keyw, const bool* vals, size_type sz,
			  JSONObject* jobj )
{
    BoolTypeSet bset;
    for ( auto idx=0; idx<sz; idx++ )
	bset.add( vals[idx] );

    if ( !jsonmode_ )
    {
	iop_.set( keyw, bset );
    }
    else
    {
	if ( !jobj )
	    jobj = &jsonroot_;

	BufferString cleankeyw( keyw ); cleankeyw.clean();
	OD::JSON::Array* arr = jobj->set( cleankeyw, new JSONArray(false) );
	arr->set( bset );
    }
}


void ServerProgTool::set( const char* keyw, JSONObject* jobj )
{
    jsonroot_.set( keyw, jobj );
}


void ServerProgTool::set( const char* keyw, JSONArray* jarr )
{
    jsonroot_.set( keyw, jarr );
}


void ServerProgTool::setStatus( bool success ) const
{
    mSelf().set( sKey::Status(), success ? "OK" : "Fail" );
}


void ServerProgTool::respondInfo( bool success, bool exit ) const
{
    setStatus( success );

    if ( jsonmode_ )
	jsonroot_.write( outStream() );
    else
    {
	ascostream ascstrm( outStream() );
	iop_.putTo( ascstrm );
    }
    outStream() << od_endl;

    if ( exit )
	exitProgram( success );
}


void ServerProgTool::respondError( const uiRetVal& uirv ) const
{
    mSelf().respondError( toString(uirv) );
}


void ServerProgTool::respondError( const char* msg ) const
{
    mSelf().set( sErrKey, msg );
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


void ServerProgTool::exitWithUsage() const
{
    BufferString msg( "* Usage: ", GetExecutableName() );
    msg.add( getSpecificUsage() );
    addToUsageStr( msg, CommandLineParser::sDataRootArg(), "data_root_dir",
		    true );
    addToUsageStr( msg, CommandLineParser::sSurveyArg(), "survey_dir", true );
    addToUsageStr( msg, sDontUseJSONCmd, "", true );
    msg.add( "\n\n* You can put all command-line arguments in a file:" );
    addToUsageStr( msg, "argsfile", "file_with_cmdline_args", false );
    msg.add( "\n\n* Overrule all arguments using environment variable '" )
	.add( clp().envVarBase() ).add( "_ARGS" ).add( "'" );
    msg.add( "\n  Add arguments using environment variable '" )
	.add( clp().envVarBase() ).add( "_EXTRA_ARGS" ).add( "'" );
    outStream() << msg << od_endl;
    exitProgram( false );
}


void ServerProgTool::exitProgram( bool success ) const
{
    const_cast<ServerProgTool&>(*this).exitProgram( success );
}


void ServerProgTool::exitProgram( bool success )
{
    retval_ = success ? 0 : 1;
    timer_.start( 0, true );
}


void ServerProgTool::timerTickCB( CallBacker* )
{
    ApplicationData::exit( retval_ );
}
