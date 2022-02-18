/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : September 2013
-*/


#include "batchprog.h"
#include "testprog.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "prestackmutedef.h"
#include "prestackmutedeftransl.h"
#include "survinfo.h"

static const char* sKeyTestMute41()	{ return "Mute for V4.1"; }
static const char* sKeyTestMute44()	{ return "Mute for V4.4"; }
static const char* sKeyTestMute46()	{ return "Mute for V4.6"; }
#define mCheckVal(offset,inl,crl,chkval) \
muteval = mutedef.value( offset, BinID(inl,crl) ); \
if ( !mIsEqual(muteval,chkval,1e-5f) ) \
{ \
    BufferString msg( "Value mismatch in '", muteobj->name(), "'" ); \
    msg += " Offset :"; msg += offset; \
    msg += " Inline :"; msg += inl; \
    msg += " Crossline :"; msg += crl; \
    strm<<msg<<od_newline; \
    return false; \
}

//check for Top Mute created in different versions
bool odTestSameMuteInDiffVersion( od_ostream& strm, const MultiID& muteid )
{
    PtrMan<IOObj> muteobj = IOM().get( muteid );
    if ( !muteobj )
    {
	strm << "Mute object with id " << muteid.toString()
	     << " not found" << od_newline;
	return false;
    }

    PreStack::MuteDef mutedef;
    uiString errmsg;
    if ( !MuteDefTranslator::retrieve(mutedef,muteobj,errmsg) )
    {
	BufferString msg;
	msg += "Mute definition ";
	msg += muteobj->name();
	msg += " cannot be read. ";
	msg += errmsg.getOriginalString();
	strm<<msg.buf()<<od_newline;
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
    if ( !quiet_ )
    {
	BufferString msg( "Test on mute '", muteobj->name() );
	msg += "' is OK";
	strm << msg.buf() << od_newline;
    }

    return true;
}


mLoad1Module("PreStackProcessing")

bool BatchProgram::doWork( od_ostream& strm )
{
    mInitBatchTestProg();

    MultiID muteid;
    if ( !pars().get(sKeyTestMute41(),muteid) )
    {
	strm << "Can not find mute for V4.1 in parameter file"<<od_newline;
	return false;
    }
    if ( !odTestSameMuteInDiffVersion(strm,muteid) )
	return false;

    if ( !pars().get(sKeyTestMute44(),muteid) )
    {
	strm << "Can not find mute for V4.4 in parameter file"<<od_newline;
	return false;
    }
    if ( !odTestSameMuteInDiffVersion(strm,muteid) )
	return false;

    if ( !pars().get(sKeyTestMute46(),muteid) )
    {
	strm << "Can not find mute for V4.6 in parameter file"<<od_newline;
	return false;
    }
    if ( !odTestSameMuteInDiffVersion(strm,muteid) )
	return false;

    return true;
};
