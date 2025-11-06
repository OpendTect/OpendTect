/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprog.h"
#include "testprog.h"

#include "binid.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "prestackmutedef.h"
#include "prestackmutedeftransl.h"

static const char* sKeyTestMute41()	{ return "Mute for V4.1"; }
static const char* sKeyTestMute44()	{ return "Mute for V4.4"; }
static const char* sKeyTestMute46()	{ return "Mute for V4.6"; }

#define mCheckVal(offset,inl,crl,chkval) \
    muteval = mutedef.value( offset, BinID(inl,crl) ); \
    desc.set( "Mute '" ).add( muteobj->name() ).add( "'" ) \
	.add( " Offset :" ).add( offset ).add( " Inline :" ).add( inl ) \
	.add( " Crossline :" ).add( crl ); \
    errmsg.set( "Expected: " ).add( chkval ) \
	  .add( ", calculated: " ).add( muteval ); \
    mRunStandardTestWithError( mIsEqual(muteval,chkval,1e-5f), \
			       desc.str(), errmsg.str() );

//check for Top Mute created in different versions
bool odTestSameMuteInDiffVersion( const MultiID& muteid )
{
    PtrMan<IOObj> muteobj = IOM().get( muteid );
    mRunStandardTestWithError( muteobj, "Get mute IOObj",
			       IOM().uiMessage().getFullString() );

    PreStack::MuteDef mutedef;
    uiString trerrmsg;
    mRunStandardTestWithError(
	MuteDefTranslator::retrieve( mutedef, muteobj.ptr(), trerrmsg ),
	"Read mute definition object", trerrmsg.getFullString() );

    float muteval = 0.0f;
    BufferString desc, errmsg;
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

    return true;
}


mLoad1Module("PreStackProcessing")

bool BatchProgram::doWork( od_ostream& /*strm*/ )
{
    mInitBatchTestProg();
    od_ostream& mUnusedVar strm = logStream();

    MultiID muteid;
    mRunStandardTest( pars().get( sKeyTestMute41(), muteid ),
		      "Mute for V4.1 in IOPar" );
    if ( !odTestSameMuteInDiffVersion(muteid) )
	return false;

    mRunStandardTest( pars().get( sKeyTestMute44(), muteid ),
		      "Mute for V4.4 in IOPar" );
    if ( !odTestSameMuteInDiffVersion(muteid) )
	return false;

    mRunStandardTest( pars().get( sKeyTestMute46(), muteid ),
		      "Mute for V4.6 in IOPar" );
    if ( !odTestSameMuteInDiffVersion(muteid) )
	return false;

    return true;
};
