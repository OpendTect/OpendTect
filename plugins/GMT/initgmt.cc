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
#include "oscommand.h"
#include "separstr.h"

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



static void extractVersionString( const BufferString& ret )
{
    if ( ret.isEmpty() )
	return;

    const SeparString retsep( ret, '\n' );
    if ( retsep.size() < 1 )
	return;

    int linenr = mUdf(int);
    if ( hasgmt5_ )
	linenr = 0;
    else
    {
	for ( int idx=0; idx<retsep.size(); idx++ )
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
    if ( versionstr.size() < 1 )
	return;

    int recnr;
    if ( hasgmt5_ )
	recnr = 0;
    else
    {
	BufferString needle;
#if defined __lux__
	needle.set( "Version" );
#elif defined __win__
	needle.set( sKeyWindowsGMT4TestExec() );
#endif
	recnr = versionstr.size()-1; //Fallback: last record
	for ( int idx=0; idx<versionstr.size()-1; idx++ )
	{
	    if ( versionstr[idx].contains(needle) )
	    {
		recnr = idx+1;
		break;
	    }
	}
    }

    gmtversionstr_.set( versionstr[recnr] );
}


static void checkGMTAvailability()
{
    BufferString stdoutstr;
    const BufferString comm5( sKeyGMTExec(), " --version" );
    hasgmt5_ = OS::ExecCommand( comm5.str(), OS::Wait4Finish,
				&stdoutstr );
    if ( hasgmt5_ )
    {
	extractVersionString( stdoutstr );
	return; //One is enough to get going
    }

#if defined __mac__
    return;	// Never supported GMT4 on MAC
#elif defined __lux__
    const BufferString comm4( sKeyUnixGMT4Wrapper(), " --version" );
#elif defined __win__
    const BufferString comm4( sKeyWindowsGMT4TestExec() );
#endif

    BufferString stderrstr;
    hasgmt4_ = OS::ExecCommand( comm4.str(), OS::Wait4Finish,
				0, &stderrstr );
    if ( hasgmt4_ )
	extractVersionString( stderrstr );
}

};



void GMT::initStdClasses()
{
    ODGMTProcFlowTranslatorGroup::initClass();
    dgbODGMTProcFlowTranslator::initClass();

    checkGMTAvailability();

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
