/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2004
-*/

static const char* rcsID = "$Id: repos.cc,v 1.2 2004-11-25 20:45:38 bert Exp $";

#include "repos.h"
#include "filepath.h"
#include <ctype.h>


bool Repos::FileProvider::next( Repos::Source& src )
{
    int newsrc = (int)src + 1;
    if ( newsrc > (int)Appl ) return false;
    src = (Repos::Source)newsrc;
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
	FilePath fp( fn() ); \
	fp.add( ret ); \
	ret = fp.fullPath()

    switch ( src )
    {
    case Repos::Temp: {
	ret = FilePath::getTempName(0);
	FilePath fp; fp.setFileName( basenm_ );
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
    case Repos::Appl: {
	ret = GetDataFileName( basenm_ );
    } break;
    }

    return ret;
}
