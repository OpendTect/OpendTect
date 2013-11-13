/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : September 2013
-*/

static const char* rcsID mUsedVar = ""; 

#include "commandlineparser.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "prestackmutedef.h"
#include "prestackmutedeftransl.h"
#include "survinfo.h"
#include <iostream>

static const char* sKeyTestSurvey()		{ return "F3_Test_Survey"; }
static const char* sKeyTestMute41()		{ return "100070.41"; }
static const char* sKeyTestMute44()		{ return "100070.44"; }
static const char* sKeyTestMute46()		{ return "100070.46"; }
#define mCheckVal(offset,inl,crl,chkval) \
muteval = mutedef.value( offset, BinID(inl,crl) ); \
if ( !mIsEqual(muteval,chkval,1e-5f) ) \
{ \
    BufferString msg( "Value mismatch in '", muteobj->name(), "'" ); \
    msg += " Offset :"; msg += offset; \
    msg += " Inline :"; msg += inl; \
    msg += " Crossline :"; msg += crl; \
    std::cerr<<msg<<std::endl; \
    return false; \
}

//check for Top Mute created in different versions
bool odTestSameMuteInDiffVersion( const MultiID& muteid, bool quiet )
{
    BufferString errmsg;
    IOObj* muteobj = IOM().get( muteid );
    if ( !muteobj )
    {
	errmsg += "Mute object not found";
	std::cerr<<errmsg.buf()<<std::endl;
	return false;
    }

    PreStack::MuteDef mutedef;
    if ( !MuteDefTranslator::retrieve(mutedef,muteobj,errmsg) )
    {
	BufferString msg;
	msg += "Mute definition ";
	msg += muteobj->name();
	msg += " cannot be read. ";
	msg += errmsg;
	std::cerr<<msg.buf()<<std::endl;
	return false;
    }

    float muteval = 0.0f;
    mCheckVal(0,200,700,0.0467186)
    mCheckVal(500,200,700,0.145135)
    mCheckVal(1000,200,700,0.700861)
    mCheckVal(1500,200,700,1.21096)
    mCheckVal(2000,200,700,1.57068)
    mCheckVal(2500,200,700,1.9304)
    mCheckVal(3000,200,700,2.29012)
    mCheckVal(0,200,800,0.0467186)
    mCheckVal(500,200,800,0.145135)
    mCheckVal(1000,200,800,0.700861)
    mCheckVal(1500,200,800,1.21096)
    mCheckVal(2000,200,800,1.57068)
    mCheckVal(2500,200,800,1.9304)
    mCheckVal(3000,200,800,2.29012)
    mCheckVal(0,200,900,0.0467186)
    mCheckVal(0,200,1100,0.0467186)
    mCheckVal(500,200,1100,0.145135)
    mCheckVal(1000,200,1100,0.700861)
    mCheckVal(1500,200,1100,1.21096)
    mCheckVal(2000,200,1100,1.57068)
    mCheckVal(2500,200,1100,1.9304)
    mCheckVal(3000,200,1100,2.29012)
    mCheckVal(0,200,1200,0.0467186)
    mCheckVal(500,200,1200,0.145135)
    mCheckVal(1000,200,1200,0.700861)
    mCheckVal(0,650,800,0.0467186)
    mCheckVal(500,650,800,0.145135)
    mCheckVal(1000,650,800,0.700861)
    mCheckVal(1500,650,800,1.21096)
    mCheckVal(2000,650,800,1.57068)
    mCheckVal(2500,650,800,1.9304)
    mCheckVal(3000,650,800,2.29012)
    mCheckVal(0,650,900,0.0467186)
    mCheckVal(500,650,900,0.145135)
    mCheckVal(1000,650,900,0.700861)
    mCheckVal(1500,650,900,1.21096)
    mCheckVal(2000,650,900,1.57068)
    mCheckVal(2500,650,900,1.9304)
    mCheckVal(3000,650,900,2.29012)
    mCheckVal(0,650,1000,0.0467186)
    mCheckVal(500,650,1000,0.145135)
    mCheckVal(1000,650,1000,0.700861)
    mCheckVal(1500,650,1000,1.21096)
    mCheckVal(2000,650,1000,1.57068)
    mCheckVal(2500,650,1000,1.9304)
    mCheckVal(3000,650,1000,2.29012)
    mCheckVal(0,650,1100,0.0467186)
    mCheckVal(500,650,1100,0.145135)
    mCheckVal(1000,650,1100,0.700861)
    mCheckVal(1500,650,1100,1.21096)
    mCheckVal(2000,650,1100,1.57068)
    mCheckVal(2500,650,1100,1.9304)
    mCheckVal(3000,650,1100,2.29012)
    mCheckVal(0,650,1200,0.0467186)
    mCheckVal(500,650,1200,0.145135)
    mCheckVal(1000,650,1200,0.700861)
    mCheckVal(1500,650,1200,1.21096)
    mCheckVal(2000,650,1200,1.57068)
    mCheckVal(2500,650,1200,1.9304)
    mCheckVal(3000,650,1200,2.29012)
    if ( !quiet )
    {
	BufferString msg( "Test on mute '", muteobj->name() );
	msg += "' is OK";
	std::cout<<msg.buf()<<std::endl;
    }

    return true;
}


int main( int argc, char** argv )
{
    od_init_test_program( argc, argv );
    OD::ModDeps().ensureLoaded( "PreStackProcessing" );

    CommandLineParser parser( argc, argv );
    const bool quiet = parser.hasKey( sKey::Quiet() );

    IOMan::setSurvey( sKeyTestSurvey() );
    if ( !odTestSameMuteInDiffVersion(MultiID(sKeyTestMute41()),quiet) )
	ExitProgram( 1 );
    if ( !odTestSameMuteInDiffVersion(MultiID(sKeyTestMute44()),quiet) )
	ExitProgram( 1 );
    if ( !odTestSameMuteInDiffVersion(MultiID(sKeyTestMute46()),quiet) )
	ExitProgram( 1 );
    ExitProgram( 0 );
};
