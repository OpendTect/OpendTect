/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Jube 2008
________________________________________________________________________

-*/

#include "gmtarray2dinterpol.h"
#include "gmtbasemap.h"
#include "gmtclip.h"
#include "gmtcoastline.h"
#include "gmtcontour.h"
#include "gmtfault.h"
#include "gmtlocations.h"
#include "gmt2dlines.h"
#include "gmtprocflowtr.h"
#include "initgmtplugin.h"
#include "msgh.h"
#include "separstr.h"
#include "callback.h"
#include "commandlaunchmgr.h"


namespace GMT {

static bool hasgmt4_ = false;
static bool hasgmt5_ = false;
static BufferString gmtversionstr_;

#if defined __lux__
  static const char* sKeyUnixGMT4Wrapper() { return "GMT"; }
#elif defined __win__
  static const char* sKeyWindowsGMT4TestExec() { return "psbasemap"; }
#endif
static const char* sKeyGMTExec()	{ return "gmt"; }



static void extractVersionString( const BufferString& inp )
{
    if ( inp.isEmpty() )
	return;

    const SeparString retsep( inp, '\n' );
    const int nrlines = retsep.size();
    if ( nrlines < 1 )
	return;

    int linenr = mUdf(int);
    if ( hasgmt5_ )
	linenr = 0;
    else
    {
	for ( int idx=0; idx<nrlines; idx++ )
	{
	    if ( retsep[idx].contains("Version") )
	    {
		linenr = idx;
		break;
	    }
	}
    }

    if ( mIsUdf(linenr) )
	return;

    const SeparString versionstr( retsep[linenr], ' ' );
    const int nrrecords = versionstr.size();
    if ( nrrecords < 1 )
	return;

    int recnr;
    if ( hasgmt5_ )
	recnr = 0;
    else
    {
	BufferString tofind;
#if defined __lux__
	tofind.set( "Version" );
#elif defined __win__
	tofind.set( sKeyWindowsGMT4TestExec() );
#endif
	recnr = nrrecords-1; //Fallback: last record
	for ( int idx=0; idx<nrrecords-1; idx++ )
	{
	    if ( versionstr[idx].contains(tofind) )
	    {
		recnr = idx+1;
		break;
	    }
	}
    }

    gmtversionstr_.set( versionstr[recnr] );
}


static void checkGMTAvailability( bool isgmt5 );

static void gmtVersionCB( CallBacker* cb )
{
    const auto* ct = Threads::CommandLaunchMgr::getMgr().getCommandTask( cb );
    if ( !ct )
	return;

    const auto& mc = ct->getMachineCommand();
    const BufferString versiontxt = ct->getStdOutput();
    const BufferString errortxt = ct->getStdError();
    if ( BufferString(mc.program())==sKeyGMTExec() )
    {
	hasgmt5_ = ct->getResult();
	if ( !hasgmt5_ )
	{
	    checkGMTAvailability( false );
	    return;
	}
    }
    else
	hasgmt4_ = ct->getResult();

    extractVersionString( versiontxt );
    if ( versiontxt.isEmpty() || !errortxt.isEmpty() )
	ErrMsg( BufferString("Compatible GMT version not found: ", errortxt) );
    else
	UsrMsg( BufferString("Found GMT", versiontxt) );
}


static void checkGMTAvailability( bool isgmt5)
{
    BufferString versiontxt;
    BufferString errortxt;
    OS::MachineCommand machcomm( sKeyGMTExec(), "--version" );
    if ( !isgmt5 )
#if defined __mac__
	return; // Never supported GMT4 on MAC
#elif defined __lux__
	machcomm.setProgram( sKeyUnixGMT4Wrapper() );
#elif defined __win__
	machcomm.setProgram( sKeyWindowsGMT4TestExec() );
#endif

    auto& mgr = Threads::CommandLaunchMgr::getMgr();
    CallBack cb( mSCB(gmtVersionCB) );
    mgr.execute( machcomm, true, true, &cb );
}

};

void GMT::initStdClasses()
{
    ODGMTProcFlowTranslatorGroup::initClass();
    dgbODGMTProcFlowTranslator::initClass();

    checkGMTAvailability( true );

    GMTBaseMap::initClass();
    GMTClip::initClass();
    GMTLegend::initClass();
    GMTLocations::initClass();
    GMTPolyline::initClass();
    GMTContour::initClass();
    GMTFault::initClass();
    GMTCoastline::initClass();
    GMTWells::initClass();
    GMT2DLines::initClass();
    GMTRandLines::initClass();
    GMTCommand::initClass();
    GMTSurfaceGrid::initClass();
    GMTNearNeighborGrid::initClass();
}


bool GMT::hasLegacyGMT()
{
    return hasgmt4_;
}


bool GMT::hasModernGMT()
{
    return hasgmt5_;
}


bool GMT::hasGMT()
{
    return hasLegacyGMT() || hasModernGMT();
}


const char* GMT::versionStr()
{
    return gmtversionstr_.str();
}


const char* GMT::sKeyDefaultExec()
{
    if ( hasModernGMT() )
	return sKeyGMTExec();

    if ( hasLegacyGMT() )
    {
#if defined __lux__
	return sKeyUnixGMT4Wrapper();
#elif defined __win__
	return 0;
#endif
    }

    return 0;
}
