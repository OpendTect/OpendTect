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
#include "oscommand.h"
#include "separstr.h"
#include "threadwork.h"


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


static bool checkGMTAvailability()
{
    BufferString versiontxt;
    BufferString errortxt;
    OS::MachineCommand machcomm( sKeyGMTExec(), "--version" );
    hasgmt5_ = machcomm.execute( versiontxt, &errortxt );
    if ( hasgmt5_ )
	extractVersionString( versiontxt );
    else
    {
#if defined __mac__
	return; // Never supported GMT4 on MAC
#elif defined __lux__
	machcomm.setProgram( sKeyUnixGMT4Wrapper() );
#elif defined __win__
	machcomm.setProgram( sKeyWindowsGMT4TestExec() );
#endif

	versiontxt.setEmpty();
	hasgmt4_ = machcomm.execute( versiontxt, &errortxt );
	if ( hasgmt4_ )
	    extractVersionString( versiontxt );
    }
    if ( versiontxt.isEmpty() || !errortxt.isEmpty() )
    {
	ErrMsg( BufferString("Compatible GMT version not found: ", errortxt) );
	return false;
    }
    UsrMsg( BufferString("Found GMT", versiontxt) );
    return true;
}

};



void GMT::initStdClasses()
{
    ODGMTProcFlowTranslatorGroup::initClass();
    dgbODGMTProcFlowTranslator::initClass();

    Threads::WorkManager::twm().addWork( Threads::Work(&checkGMTAvailability) );

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
