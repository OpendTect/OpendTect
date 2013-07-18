/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jul 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: odinstpkgkey.cc 7574 2013-03-26 13:56:34Z kristofer.tingdahl@dgbes.com $";

#include "odinstpkgkey.h"


ODInst::PkgKey::PkgKey( const char* filestr )
{
    BufferString bs( filestr );
    char* verptr = strchr( bs.buf(), ':' );
    if ( verptr )
    {
	*verptr++ = '\0';
	mTrimBlanks(verptr);
	ver_.set( verptr );
    }

    setFromFileNameBase( bs.buf() );
}


ODInst::PkgKey::PkgKey( const char* filenm, const char* ver )
    : ver_(ver)
{
    BufferString inpfnm( filenm );
    const char* startptr = strchr( inpfnm.buf(), '.' );
    if ( !startptr )
	startptr = "_no_name_";
    else
    {
	startptr++;
	char* ptr = strchr( (char*)startptr, '.' );
	if ( ptr ) *ptr = '\0';
    }
    setFromFileNameBase( startptr );
}


void ODInst::PkgKey::setFromFileNameBase( const char* inpstr )
{
    BufferString bs( inpstr );
    char* nmptr = bs.buf(); mTrimBlanks(nmptr);
    char* plfptr = strchr( nmptr, '_' );
    if ( plfptr )
    {
	*plfptr++ = '\0';
	plf_.set( plfptr, true );
    }

    nm_ = nmptr;
}


BufferString ODInst::PkgKey::fileNameBase() const
{
    BufferString ret = nm_;
    if ( !plf_.isIndep() )
	ret.add( "_" ).add( plf_.shortName() );
    return ret;
}

BufferString ODInst::PkgKey::zipFileName() const
{
    BufferString ret( fileNameBase(), ".zip" );
    return ret;
}

BufferString ODInst::PkgKey::listFileName() const
{
    BufferString ret( fileNameBase(), ".zip.list" );
    return ret;
}
