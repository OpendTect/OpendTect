/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		01/12/2014
________________________________________________________________________

-*/

#include "settingsaccess.h"
#include "envvars.h"
#include "keystrs.h"
#include "oddirs.h"
#include "oscommand.h"


const char* SettingsAccess::sKeyEnabSharedStor()
{ return "Use DATA for sharing"; }

const char* SettingsAccess::sKeyIcons()
{ return "dTect.Icons"; }

const char* SettingsAccess::sKeyShowInlProgress()
{ return "dTect.Show inl progress"; }

const char* SettingsAccess::sKeyShowCrlProgress()
{ return "dTect.Show crl progress"; }

const char* SettingsAccess::sKeyShowRdlProgress()
{ return "dTect.Show rdl progress"; }

const char* SettingsAccess::sKeyUseSurfShaders()
{ return "dTect.Use surface shaders"; }

const char* SettingsAccess::sKeyUseVolShaders()
{ return "dTect.Use volume shaders"; }

const char* SettingsAccess::sKeyTexResFactor()
{ return "dTect.Default texture resolution factor"; }

const char* SettingsAccess::sKeyEnableMipmapping()
{ return "dTect.Enable mipmapping"; }

const char* SettingsAccess::sKeyAnisotropicPower()
{ return "dTect.Anisotropic power"; }

const char* SettingsAccess::sKeyMouseWheelReversal()
{ return "dTect.Mouse Wheel Reversal"; }

const char* SettingsAccess::sKeyMouseWheelZoomFactor()
{ return "dTect.Mouse Wheel Zoom Factor"; }


SettingsAccess::SettingsAccess()
    : settings_( Settings::common() )
{}


SettingsAccess::SettingsAccess( Settings& settings )
    : settings_( settings )
{}


bool SettingsAccess::doesUserWantShading( bool forvolume ) const
{
    if ( GetEnvVarYN("DTECT_MULTITEXTURE_NO_SHADERS") )
	return false;

    bool userwantsshading = true;
    if ( forvolume )
	settings_.getYN( sKeyUseVolShaders(), userwantsshading );
    else
	settings_.getYN( sKeyUseSurfShaders(), userwantsshading );

    return userwantsshading;
}


static int validResolution( int res, int nrres )
{ return mMAX( 0, mMIN(res, nrres-1) ); }


static int defaultTexResFactorFromEnvVar()
{
    const char* envvar = GetEnvVar( "OD_DEFAULT_TEXTURE_RESOLUTION_FACTOR" );
    return envvar && iswdigit(*envvar) ? toInt(envvar) : -1;
}


bool SettingsAccess::systemHasDefaultTexResFactor()
{ return defaultTexResFactorFromEnvVar() >= 0; }


#define mSystemDefaultTexResSetting -1

int SettingsAccess::getDefaultTexResAsIndex( int nrres ) const
{
    int res = 0;
    if ( settings_.get(sKeyTexResFactor(),res) )
    {
	if ( res == mSystemDefaultTexResSetting )
	    res = systemHasDefaultTexResFactor() ? nrres++ : 0;
    }

    return validResolution( res, nrres );
}


void SettingsAccess::setDefaultTexResAsIndex( int idx, int nrres )
{
    int res = validResolution( idx, nrres );
    if ( idx == nrres )
	res = mSystemDefaultTexResSetting;

    settings_.set( sKeyTexResFactor(), res );
}


int SettingsAccess::getDefaultTexResFactor( int nrres ) const
{
    int res = 0;
    if ( settings_.get(sKeyTexResFactor(),res) )
    {
	if ( res != mSystemDefaultTexResSetting )
	    return validResolution( res, nrres );
    }

    res = defaultTexResFactorFromEnvVar();
    return validResolution( res, nrres );
}


BufferString SettingsAccess::getTerminalEmulator()
{
    const BufferString orgtermcmd = settings_.find( sKey::TermEm() );
    BufferString termcmd = orgtermcmd;
#ifdef __win__
    if ( termcmd.isEmpty() )
	termcmd = "cmd.exe";
#else
    OS::MachineCommand mc( GetShellScript("od_find_term.bash") );
    if ( !termcmd.isEmpty() )
	mc.addArg( termcmd );
    termcmd = mc.runAndCollectOutput();
#endif
    settings_.set( sKey::TermEm(), termcmd );
    if ( termcmd != orgtermcmd )
	settings_.write( false );

    return termcmd;
}

