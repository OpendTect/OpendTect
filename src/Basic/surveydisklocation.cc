/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2018
________________________________________________________________________

-*/

#include "surveydisklocation.h"
#include "filepath.h"
#include "oddirs.h"
#include "ascstream.h"
#include "od_istream.h"
#include "survinfo.h"
#include "keystrs.h"
#include "iopar.h"


SurveyDiskLocation::SurveyDiskLocation( const char* dirnm, const char* bp )
    : dirname_(dirnm)
    , basepath_(bp && *bp ? bp : GetBaseDataDir())
{
}


SurveyDiskLocation::SurveyDiskLocation( const File::Path& fp )
{
    set( fp );
}


bool SurveyDiskLocation::operator ==( const SurveyDiskLocation& oth ) const
{
    const bool iscur = isCurrentSurvey();
    if ( iscur != oth.isCurrentSurvey() )
	return false;
    if ( iscur )
	return true;

    return basepath_ == oth.basepath_ && dirname_ == oth.dirname_;
}


void SurveyDiskLocation::set( const char* fullpath )
{
    if ( !fullpath || !*fullpath )
	setCurrentSurvey();
    else
	set( File::Path(fullpath) );
}


void SurveyDiskLocation::set( const File::Path& fp )
{
    basepath_ = fp.pathOnly();
    dirname_ = fp.fileName();
}


bool SurveyDiskLocation::isCurrentSurvey() const
{
    if ( basepath_.isEmpty() && dirname_.isEmpty() )
	return true;

    SurveyDiskLocation cursdl;
    cursdl.setCurrentSurvey();

    if ( !basepath_.isEmpty() && basepath_ != cursdl.basepath_ )
	return false;

    return dirname_ == cursdl.dirname_;
}


bool SurveyDiskLocation::isEmpty() const
{
    return dirname_.isEmpty() && basepath_.isEmpty();
}


void SurveyDiskLocation::setEmpty()
{
    dirname_.setEmpty();
    basepath_.setEmpty();
}

void SurveyDiskLocation::setCurrentSurvey( bool hard )
{
    if ( hard )
	set( File::Path(SI().getBasePath(),SI().getDirName()) );
    else
	setEmpty();
}


void SurveyDiskLocation::ensureHardPath()
{
    if ( hasSoftPath() )
    {
	SurveyDiskLocation sdl;
	sdl.setCurrentSurvey();
	if ( basepath_.isEmpty() )
	    basepath_ = sdl.basepath_;
	if ( dirname_.isEmpty() )
	    dirname_ = sdl.dirname_;
    }
}


void SurveyDiskLocation::softenPath()
{
    if ( !basepath_.isEmpty() || !dirname_.isEmpty() )
    {
	SurveyDiskLocation sdl;
	sdl.setCurrentSurvey();
	if ( basepath_ == sdl.basepath_ )
	    basepath_.setEmpty();
	if ( dirname_ == sdl.dirname_ )
	    dirname_.setEmpty();
    }
}


BufferString SurveyDiskLocation::fullPath() const
{
    if ( hasSoftPath() )
    {
	SurveyDiskLocation sdl( *this );
	sdl.ensureHardPath();
	return sdl.fullPath();
    }

    return File::Path( basepath_, dirname_ ).fullPath();
}


BufferString SurveyDiskLocation::fullPathFor( const char* fnm ) const
{
    if ( !fnm || !*fnm )
	return fullPath();
    else if ( hasSoftPath() )
	return File::Path( fullPath(), fnm ).fullPath();

    return File::Path( basepath_, dirname_, fnm ).fullPath();
}


BufferString SurveyDiskLocation::surveyName() const
{
    const BufferString survdir( fullPath() );
    File::Path fp( survdir );
    fp.add( ".survey" );
    od_istream strm( fp.fullPath() );
    ascistream astrm( strm );
    IOPar iop( astrm );
    BufferString ret( dirname_ );
    iop.get( sKey::Name(), ret );
    if ( ret.isEmpty() )
	ret = File::Path(survdir).fileName();
    return ret;
}
