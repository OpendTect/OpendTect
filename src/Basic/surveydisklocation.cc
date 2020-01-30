/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2018
________________________________________________________________________

-*/

#include "surveydisklocation.h"
#include "ascstream.h"
#include "dirlist.h"
#include "filepath.h"
#include "iopar.h"
#include "keystrs.h"
#include "oddirs.h"
#include "od_istream.h"
#include "survgeommgr.h"
#include "survinfo.h"

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
    if ( dirName() != oth.dirName() )
	return false;

    if ( basePath() == oth.basePath() )
	return true;

    File::Path fp( basePath() ), ofp( oth.basePath() );
    fp.makeCanonical(); ofp.makeCanonical();
    return fp == ofp;
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


bool SurveyDiskLocation::exists() const
{
    return File::isDirectory( fullPath() );
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


void SurveyDiskLocation::listSurveys( BufferStringSet& dirnms, const char* bp )
{
    const BufferString basepath( bp && *bp
				 ? bp : currentSurvey().basePath().str() );
    Survey::getDirectoryNames( dirnms, false, basepath );
}


void SurveyDiskLocation::fillPar( IOPar& iop, bool force ) const
{
    if ( isCurrentSurvey() )
    {
	if ( force )
	    iop.set( sKey::Survey(), fullPath() );
	else
	    iop.set( sKey::Survey(), BufferString::empty() );
	return;
    }

    const SurveyDiskLocation& cursurvsdl = currentSurvey();
    if ( basePath() == cursurvsdl.basePath() )
	iop.set( sKey::Survey(), dirName() );
    else
	iop.set( sKey::Survey(), fullPath() );
}


bool SurveyDiskLocation::usePar( const IOPar& iop )
{
    BufferString survdir;
    if ( !iop.get(sKey::Survey(),survdir) )
	return false;

    if ( survdir.isEmpty() )
    {
	setToCurrentSurvey();
	return true;
    }

    File::Path fp( survdir );
    const SurveyDiskLocation sdl( fp );
    if ( sdl.exists() )
	{ *this = sdl; return true; }
    else
    {
	const SurveyDiskLocation& cursdl = currentSurvey();
	fp.set( cursdl.basePath() ).add( survdir );
	const SurveyDiskLocation relsdl( fp );
	if ( relsdl.exists() )
	    { *this = relsdl; return true; }
	else
	{
	    fp.set( cursdl.basePath() ).add( sdl.dirName() );
	    const SurveyDiskLocation rebasedsdl( fp );
	    if ( rebasedsdl.exists() )
		{ *this = rebasedsdl; return true; }
	}
    }

    return false;
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
    static TypeSet<od_int64> timestamps_;
    static Threads::Lock infolock;

    Threads::Locker locker( infolock );
    const BufferString fullpath = fullPath();
    const File::Path fp( fullpath, SurveyInfo::sSetupFileName() );
    const BufferString survinfofnm = fp.fullPath();

    SurveyInfo* newinfo = 0;
    od_int64 timestamp = mUdf( od_int64 );
    if ( File::exists(survinfofnm) )
    {
	timestamp = File::getTimeInSeconds( survinfofnm );
	for ( int idx=0; idx<infos_.size(); idx++ )
	{
	    if ( infos_[idx]->diskLocation() == *this )
	    {
		if ( timestamps_[idx] <= timestamp )
		    return *infos_[idx];

		delete infos_.removeSingle( idx );
		break;
	    }
	}

	uiRetVal uirv;
	newinfo = SurveyInfo::read( fullpath, uirv );
    }

    if ( !newinfo )
	{ static const SurveyInfo emptisi; return emptisi; }

    infos_ += newinfo;
    timestamps_ += timestamp;
    return *newinfo;
}


const SurvGM& SurveyDiskLocation::geometryManager() const
{
    if ( isCurrentSurvey() )
	return Survey::GM();

    //TODO implement this cache
    static SurvGM ret;
    return ret;
}
