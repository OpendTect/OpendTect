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

const SurveyDiskLocation& SurveyDiskLocation::currentSurvey()
{
    static SurveyDiskLocation empty;
    return empty;
}


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
    const bool isempty = isEmpty();
    const bool othisempty = oth.isEmpty();
    if ( isempty || othisempty )
	return isempty == othisempty;

    return basePath() == oth.basePath() && dirName() == oth.dirName();
}


BufferString SurveyDiskLocation::basePath() const
{
    return basepath_.isEmpty() ? SI().basePath() : basepath_;
}


BufferString SurveyDiskLocation::dirName() const
{
    return dirname_.isEmpty() ? SI().dirName() : dirname_;
}


void SurveyDiskLocation::set( const char* fullpath )
{
    if ( !fullpath || !*fullpath )
	setEmpty();
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
    return isEmpty() || *this == currentSurvey();
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


void SurveyDiskLocation::setToCurrentSurvey( bool hard )
{
    if ( !hard )
	setEmpty();
    else
    {
	basepath_ = currentSurvey().basePath();
	dirname_ = currentSurvey().dirName();
    }
}


void SurveyDiskLocation::ensureHardPath()
{
    if ( hasSoftPath() )
    {
	if ( basepath_.isEmpty() )
	    basepath_ = currentSurvey().basePath();
	if ( dirname_.isEmpty() )
	    dirname_ = currentSurvey().dirName();
    }
}


void SurveyDiskLocation::softenPath()
{
    if ( !basepath_.isEmpty() || !dirname_.isEmpty() )
    {
	if ( basepath_ == currentSurvey().basePath() )
	    basepath_.setEmpty();
	if ( dirname_ == currentSurvey().dirName() )
	    dirname_.setEmpty();
    }
}


BufferString SurveyDiskLocation::fullPath() const
{
    return File::Path( basePath(), dirName() ).fullPath();
}


BufferString SurveyDiskLocation::fullPathFor( const char* fnm ) const
{
    if ( !fnm || !*fnm )
	return fullPath();

    return File::Path( basePath(), dirName(), fnm ).fullPath();
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
