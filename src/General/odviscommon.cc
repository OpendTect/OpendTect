/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2018
-*/

#include "odviscommon.h"
#include "coltabseqmgr.h"
#include "settings.h"
#include "uistrings.h"


OD::SurfaceResolution OD::getDefaultSurfaceResolution()
{
    int ret = (int)SurfaceResolution::Automatic;
    Settings::common().get( sSurfaceResolutionSettingsKey(), ret );
    return (SurfaceResolution)ret;
}


uiWord OD::getSurfaceResolutionDispStr( SurfaceResolution sr )
{
    if ( sr == OD::SurfaceResolution::Automatic )
	return uiStrings::sAutomatic();
    else if ( sr == OD::SurfaceResolution::Full )
	return uiStrings::sFull();
    else if ( sr == OD::SurfaceResolution::Half )
	return uiStrings::sHalf();

    const int denom = Math::IntPowerOf( 2, (int)sr-1 );
    return toUiString( "1 / %1" ).arg( denom );
}


const char* OD::defSurfaceDataColSeqName()
{
    // legacy first
    const char* res = Settings::common().find( "dTect.Horizon.Color table" );
    if ( res && *res && ColTab::SeqMGR().isPresent(res) )
	return res;

    // just use the default for non-seismic data
    return ColTab::defSeqName( false );
}
