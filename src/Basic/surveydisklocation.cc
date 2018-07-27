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
    return basePath() == oth.basePath() && dirName() == oth.dirName();
}


BufferString SurveyDiskLocation::basePath() const
{
    return basepath_.isEmpty() ? SI().diskLocation().basepath_ : basepath_;
}


BufferString SurveyDiskLocation::dirName() const
{
    return dirname_.isEmpty() ? SI().diskLocation().dirname_ : dirname_;
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


void SurveyDiskLocation::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Survey(), fullPath() );
}


bool SurveyDiskLocation::usePar( const IOPar& iop )
{
    BufferString pth;
    if ( !iop.get( sKey::Survey(), pth ) )
	return false;
    set( pth );
    return true;
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
    BufferString ret( surveyInfo().getName() );
    if ( ret.isEmpty() )
	ret = dirname_;
    return ret;
}


const SurveyInfo& SurveyDiskLocation::surveyInfo() const
{
    if ( isCurrentSurvey() )
	return SI();

    static ObjectSet<SurveyInfo> infos_;
    static Threads::Lock infolock;

    Threads::Locker locker( infolock );
    for ( int idx=0; idx<infos_.size(); idx++ )
	if ( infos_[idx]->diskLocation() == *this )
	    return *infos_[idx];

    uiRetVal uirv;
    SurveyInfo* newinfo = SurveyInfo::read( fullPath(), uirv );
    if ( !newinfo )
	{ static const SurveyInfo emptisi; return emptisi; }

    infos_ += newinfo;
    return *newinfo;
}
