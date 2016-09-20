/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 3-8-1994
-*/


#include "dbman.h"
#include "dbdir.h"
#include "oddirs.h"
#include "file.h"
#include "filepath.h"
#include "survinfo.h"
#include "survgeom.h"


static DBMan* theinst_ = 0;
DBMan& DBM()
{
    if ( !theinst_ )
	theinst_ = new DBMan;
    return *theinst_;
}


DBMan::DBMan()
    : rootdir_(GetDataDir())
    , surveyToBeChanged(this)
    , surveyChanged(this)
    , afterSurveyChange(this)
    , applicationClosing(this)
{
    handleNewRootDir();
}


bool DBMan::isBad() const
{
    return !rootdbdir_ || rootdbdir_->isBad();
}


BufferString DBMan::surveyName() const
{
    return GetSurveyName();
}


BufferString DBMan::surveyDirectory() const
{
    return FilePath( rootdir_, surveyName() ).fullPath();
}


DBMan::~DBMan()
{
    if ( rootdbdir_ )
	rootdbdir_->unRef();
    deepUnRef( dirs_ );
}


void DBMan::leaveSurvey()
{
    surveyToBeChanged.trigger();
    rootdbdir_->unRef(); rootdbdir_ = 0;
    deepUnRef( dirs_ );
}


void DBMan::handleNewRootDir()
{
    errmsg_.setEmpty();
    if ( rootdir_.isEmpty() )
	{ errmsg_ = tr( "Directory for data storage is not set" ); return; }

    rootdbdir_ = new DBDir( surveyDirectory() );
    rootdbdir_->ref();
    if ( isBad() )
	errmsg_ = rootdbdir_->errMsg();
    else
	Survey::GMAdmin().fillGeometries(0);
}
