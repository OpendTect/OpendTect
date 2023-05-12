/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odver.h"
#include "oddirs.h"
#include "odinst.h"
#include "odplatform.h"
#include "genc.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "perthreadrepos.h"
#ifndef OD_NO_QT
# include "qglobal.h"
#endif
#include <string.h>


mDefineEnumUtils(OD::Platform,Type,"Platform")
{
	"Linux",
	"Windows",
	"macOS (ARM)",
	"macOS (Intel)",
	nullptr
};


extern "C" const char* GetFullODVersion()
{
    mDeclStaticString( ret );
    if ( !ret.isEmpty() ) return ret.buf();

    GetSpecificODVersion( nullptr, ret );

    if ( ret.isEmpty() )
    {
	const char* pvnm = GetProjectVersionName();
	pvnm = lastOcc( pvnm, 'V' );
	if ( pvnm )
	    ret = pvnm + 1;

	if ( ret.isEmpty() )
	    ret = "0.0.0";
    }

    return ret.buf();
}


void GetSpecificODVersion( const char* typ, BufferString& res )
{
    res = ODInst::getPkgVersion( typ ? typ : "basedata" );
    if ( res.matches( "*Internal*" ) || res.matches( "*error*" ) )
	res.setEmpty();
}


static const char* sCompilerVersionUnkwown = "<unknown>";


const char* GetGCCVersion()
{
#ifdef __GNUC__
    mDeclStaticString( ret );
    if ( !ret.isEmpty() )
	return ret.buf();

    ret.set( __GNUC__ ).add( "." )
       .add( __GNUC_MINOR__ ).add( "." )
       .add( __GNUC_PATCHLEVEL__ );
    return ret.buf();
#else
    return sCompilerVersionUnkwown;
#endif
}


const char* GetMSVCVersion()
{
#ifdef __msvc__
    mDeclStaticString( ret );
    if ( !ret.isEmpty() ) return ret.buf();
    ret.set( _MSC_VER );
    return ret;
#else
    return sKey::EmptyString();
#endif
}


const char* GetMSVCVersionStr()
{
    mDeclStaticString( ret );
    if ( !ret.isEmpty() ) return ret.buf();

    ret = sCompilerVersionUnkwown;
#ifdef __msvc__
# if ( _MSC_VER < 1700 )
    ret = "Visual Studio Pre-2012";
# elif ( _MSC_VER < 1800 )
    ret = "Visual Studio 2012 - MSVC 11.0";
# elif ( _MSC_VER < 1900 )
    ret = "Visual Studio 2013 - MSVC 12.0";
# elif ( _MSC_VER < 1910 )
    ret = "Visual Studio 2015 - MSVC 14.0";
# elif ( _MSC_VER < 1920 )
    ret = "Visual Studio 2017 - MSVC 15.0";
# elif ( _MSC_VER < 1930 )
#    if ( _MSC_VER == 1920 )
        ret = "Visual Studio 2019 - MSVC 16.0";
#    elif ( _MSC_VER == 1921 )
        ret = "Visual Studio 2019 - MSVC 16.1";
#    elif ( _MSC_VER == 1922 )
        ret = "Visual Studio 2019 - MSVC 16.2";
#    elif ( _MSC_VER == 1923 )
        ret = "Visual Studio 2019 - MSVC 16.3";
#    elif ( _MSC_VER == 1924 )
        ret = "Visual Studio 2019 - MSVC 16.4";
#    elif ( _MSC_VER == 1925 )
        ret = "Visual Studio 2019 - MSVC 16.5";
#    elif ( _MSC_VER == 1926 )
        ret = "Visual Studio 2019 - MSVC 16.6";
#    elif ( _MSC_VER == 1927 )
        ret = "Visual Studio 2019 - MSVC 16.7";
#    elif ( _MSC_VER == 1928 )
        ret = "Visual Studio 2019 - MSVC 16.9";
#    else
        ret = "Visual Studio 2019 - MSVC 16.11";
#    endif
# elif ( _MSC_VER < 1945 )
#    if ( _MSC_VER == 1930 )
        ret = "Visual Studio 2022 - MSVC 17.0";
#    elif ( _MSC_VER == 1931 )
        ret = "Visual Studio 2022 - MSVC 17.1";
#    elif ( _MSC_VER == 1932 )
        ret = "Visual Studio 2022 - MSVC 17.2";
#    elif ( _MSC_VER == 1933 )
        ret = "Visual Studio 2022 - MSVC 17.3";
#    elif ( _MSC_VER == 1934 )
        ret = "Visual Studio 2022 - MSVC 17.4";
#    else
        ret = "Visual Studio 2022 - MSVC 17.X";
#    endif
# else
    ret = "CHANGE GetMSVCVersionStr() macro to support newer version";
# endif
#endif

    return ret.buf();
}


const char* GetClangVersion()
{
#ifdef __clang__
    mDeclStaticString( ret );
    if ( !ret.isEmpty() )
	return ret.buf();

    ret.set( __clang_major__ ).add( "." )
       .add( __clang_minor__ ).add( "." )
       .add( __clang_patchlevel__ );
    return ret.buf();
#else
    return sCompilerVersionUnkwown;
#endif
}


const char* GetCompilerVersionStr()
{
    mDeclStaticString( ret );
#ifdef __win__
    ret = GetMSVCVersionStr();

#else
# ifdef __clang__
    ret.set( "Apple clang version " ).add( GetClangVersion() );
# else
    ret.set( "GCC " ).add( GetGCCVersion() );
# endif

#endif
    return ret;
}


const char* GetQtVersion()
{
    mDeclStaticString( ret );
#ifndef OD_NO_QT
    if ( !ret.isEmpty() ) return ret.buf();

    ret.set( QT_VERSION_STR );
#endif
    return ret.buf();
}


const OD::Platform& OD::Platform::local()
{
    mDefineStaticLocalObject( Platform, theplf, );
    return theplf;
}


OD::Platform::Platform()
    : type_(__ismac__ ? (__isarm__ ? MacARM : MacIntel)
		      : (__iswin__ ? Windows : Linux) )
{
}


OD::Platform::Platform( Type typ )
    : type_(typ)
{
}


const char* OD::Platform::shortName() const
{
    return isMac() ? (type_ == MacARM ? "macarm" : "macintel")
		   : (isWindows() ? "win64" : "lux64" );
}


const char* OD::Platform::osName() const
{
    return isLinux() ? "Linux" : (isWindows() ? "Windows" : "macOS");
}


void OD::Platform::set( const char* s, bool isshort )
{
    if ( !s || !*s )
	{ pErrMsg("null or empty platform set"); return; }

    if ( !isshort )
	parseEnumType( s, type_ );
    else
    {
	const bool ismac = *s == 'm';
	if ( ismac )
	{
	    const StringView str( s );
	    if ( str == "macintel" )
		type_ = MacIntel;
	    else
		type_ = MacARM;

	    return;
	}

	const bool islin = *s == 'l';
	type_ = islin ? Linux : Windows;
    }
}


bool OD::Platform::isValidName( const char* s, bool isshort )
{
    if ( !s || !*s )	return false;
    if ( !isshort )	return TypeDef().isValidName( s );

    const StringView cmp( s );
    return cmp == "lux64" || cmp == "win64" ||
	   cmp == "macarm" || cmp == "macintel";
}


static BufferStringSet legalinfo_;

const BufferStringSet&	 GetLegalInformation()
{ return legalinfo_; }


void AddLegalInformation( const char* txt )
{
    legalinfo_.add( txt );
}
