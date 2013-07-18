/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: odinstrel.cc 8016 2013-06-21 10:20:07Z kristofer.tingdahl@dgbes.com $";

#include "odinstrel.h"

#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "odinstplf.h"
#include "separstr.h"
#include "bufstringset.h"
#include "iopar.h"

const char* ODInst::sBasePkgName()	{ return "base"; }

const char* ODInst::sTempDataDir()
{
    static BufferString tmpdirpath;
    if ( tmpdirpath.isEmpty() )
    {
	BufferString userdirnm( GetUserNm(), "_od_instmgr" );
	tmpdirpath = FilePath( File::getTempPath(), userdirnm ).fullPath();
    }

	if ( !File::exists(tmpdirpath) && !File::createDir(tmpdirpath) )
	{
	    tmpdirpath.setEmpty();
	    return 0;
	}

    return tmpdirpath.buf();
}

DefineNameSpaceEnumNames(ODInst,LicType,0,"License type")
{
	"Commercial",
	"Academic",
	"GPL",
	0
};


void ODInst::getBSSFromIOP( const char* inpky, const IOPar& iop,
			    BufferStringSet& bss )
{
    bss.erase();
    for ( int idx=0; ; idx++ )
    {
	BufferString ky( inpky );
	if ( idx ) { ky += "."; ky += idx; }
	const char* res = iop.find( ky );
        if ( !res && idx > 1 )
	    return;
	if ( res )
	    bss.add( res );
    }
}


BufferString ODInst::fullURL( const char* inpurl )
{
    BufferString url( inpurl );
    if ( url.isEmpty() )
	url = "about:blank";
    else if ( !url.matches("http") && !url.matches("ftp") )
	{ url = "http://"; url += inpurl; }
    return url;
}


const char* ODInst::Platform::longName() const
{
    return indep_ ? "-" : plf_.longName();
}


const char* ODInst::Platform::shortName() const
{
    return indep_ ? "-" : plf_.shortName();
}


void ODInst::Platform::set( const char* s, bool isshortnm )
{
    indep_ = !OD::Platform::isValidName( s, isshortnm );
    if ( !indep_ )
	plf_.set( s, isshortnm );
}


ODInst::Platform ODInst::Platform::thisPlatform()
{
    return Platform( __iswin__, __is32bits__, __ismac__ );
}


BufferString ODInst::Platform::getPlfSubDir() const
{
    return BufferString( plf_.isWindows() ? plf_.is32Bits() ? "win32" : "win64" :
			 plf_.isLinux() ? plf_.is32Bits() ? "lux32" : "lux64"
			 : "" );
}


BufferString ODInst::RelData::prettyName() const
{
    BufferString ret( ODInst::getRelTypeString(reltype_) );
    ret.add( " " ).add( version_.fullName() );
    return ret;
}


void ODInst::RelDataSet::set( const BufferStringSet& bss )
{
    deepErase( *this );

    for ( int idx=0; idx<bss.size(); idx++ )
    {
	FileMultiString fms( bss.get(idx) );
	const int sz = fms.size();
	if ( sz < 2 ) continue;

	RelData* rd = new RelData;
	rd->name_ = fms[0];
	ODInst::parseEnumRelType( fms[1], rd->reltype_ );
	rd->version_.set( fms[2] );
	*this += rd;
    }
}


ODInst::RelData* ODInst::RelDataSet::get( ODInst::RelType rt )
{
    for ( int idx=0; idx<size(); idx++ )
	if ( (*this)[idx]->reltype_ == rt )
	    return (*this)[idx];
    return 0;
}


ODInst::RelData* ODInst::RelDataSet::get( const Version& version )
{
    for ( int idx=0; idx<size(); idx++ )
	if ( (*this)[idx]->version_ == version )
	    return (*this)[idx];
    return 0;
}


ODInst::RelData* ODInst::RelDataSet::get( const char* name )
{
    for ( int idx=0; idx<size(); idx++ )
	if ( (*this)[idx]->name_ == name )
	    return (*this)[idx];
    return 0;
}


const ODInst::RelData* ODInst::RelDataSet::get( const Version& version ) const
{
    for ( int idx=0; idx<size(); idx++ )
	if ( (*this)[idx]->version_ == version )
	    return (*this)[idx];
    return 0;
}


const ODInst::RelData* ODInst::RelDataSet::get( const char* name ) const
{
    for ( int idx=0; idx<size(); idx++ )
	if ( (*this)[idx]->name_ == name )
	    return (*this)[idx];
    return 0;
}


void ODInst::Version::setEmpty()
{
    super_ = major_ = minor_ = 0; patch_.setEmpty();
}


void ODInst::Version::set( const char* verstr )
{
    setEmpty();
    BufferString ver( verstr );
    if ( ver.isEmpty() || ver == "." || ver == "-" ) return;

    char* minorptr = ver.buf();
    if ( *minorptr == '.' ) minorptr++;
    char* majorptr = 0;
    char* superptr = 0;
    char* ptr = strrchr( ver.buf(), '.' );
    if ( ptr )
    {
	*ptr = '\0';
	minorptr = ptr + 1;
	if ( ptr != ver.buf() )
	{
	    ptr = strrchr( ver.buf(), '.' );
	    if ( !ptr )
		majorptr = ver.buf();
	    else
	    {
		*ptr = '\0';
		majorptr = ptr + 1;
		superptr = ver.buf();
	    }
	}
    }

    ptr = minorptr;
    while ( *ptr && isdigit(*ptr) ) ptr++;
    patch_ = ptr;
    *ptr = '\0';

    if ( superptr ) super_ = toInt( superptr );
    if ( majorptr ) major_ = toInt( majorptr );
    minor_ = toInt( minorptr );
}


bool ODInst::Version::operator ==( const Version& v ) const
{
    return v.isCompatibleWith(*this) && v.patch_ == patch_;
}


bool ODInst::Version::operator >( const Version& v ) const
{
    if ( super_ != v.super_ ) return super_ > v.super_;
    if ( major_ != v.major_ ) return major_ > v.major_;
    if ( minor_ != v.minor_ ) return minor_ > v.minor_;
    return patch_ > v.patch_;
}


BufferString ODInst::Version::dirName( bool local ) const
{
    BufferString ret;
    if ( isEmpty() )
	ret = "-";
    else
    {
	if ( super_ ) ret.add( super_ ).add( ".");
	if ( major_ || super_ ) ret.add( major_ ).add( ".");
	ret += minor_;
#ifdef __mac__
	if ( local )
	{
	    ret.insertAt( 0, "OpendTect" );
	    if ( super_ > 4 || major_ > 5 ) ret += " ";
	    ret += ".app";
	}
#endif
    }
    return ret;
}


BufferString ODInst::Version::fullName() const
{
    if ( isEmpty() )
	return dirName(false);

    BufferString ret( dirName(false) );
    ret += patch_;
    return ret;
}


bool ODInst::Version::isCompatibleWith( const ODInst::Version& v ) const
{
    return super_ == v.super_ && major_ == v.major_ && minor_ == v.minor_;
}
