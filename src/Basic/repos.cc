/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2004
-*/

static const char* rcsID = "$Id: repos.cc,v 1.10 2011/12/14 13:16:41 cvsbert Exp $";

#include "repos.h"
#include "filepath.h"
#include "oddirs.h"
#include <ctype.h>


bool Repos::FileProvider::next( Repos::Source& src, bool rev )
{
    if ( rev )
    {
	if ( src == Rel )
	    return false;
	else if ( src == Temp )
	    src = User;
	else
	    src = (Source)(((int)src)-1);;
    }
    else
    {
	if ( src == User )
	    return false;
	else
	    src = (Source)(((int)src)+1);;
    }
    return true;
}


void Repos::FileProvider::getFname( BufferString& res, bool withdot ) const
{
    res = withdot ? "." : "";
    res += basenm_;
    char* ptr = res.buf();
    while ( *ptr )
    {
	if ( isupper(*ptr) ) *ptr = tolower(*ptr);
	ptr++;
    }
}


BufferString Repos::FileProvider::fileName( Repos::Source src ) const
{
    BufferString ret;

#define mSetRet(fn,yn) \
	getFname( ret, yn ); \
	ret = FilePath( fn(), ret ).fullPath()

    switch ( src )
    {
    case Repos::Temp: {
	FilePath fp( FilePath::getTempName(0) );
	fp.setFileName( basenm_ );
	ret = fp.fullPath();
    } break;
    case Repos::Survey: {
	mSetRet(GetDataDir,true);
    } break;
    case Repos::Data: {
	mSetRet(GetBaseDataDir,true);
    } break;
    case Repos::User: {
	mSetRet(GetSettingsDir,false);
    } break;
    case Repos::ApplSetup: {
	ret = GetSetupDataFileName( ODSetupLoc_ApplSetupOnly, basenm_, 0 );
    } break;
    case Repos::Rel: {
	ret = GetSetupDataFileName( ODSetupLoc_SWDirOnly, basenm_, 0 );
    } break;
    }

    return ret;
}
