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
#include "iosubdir.h"
#include "od_ostream.h"
#include "genc.h"
#include "uistrings.h"


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
    {
	Survey::GMAdmin().fillGeometries(0);
	readDirs();
    }
}


void DBMan::readDirs()
{
    DBDirIter iter( *rootdbdir_ );
    while ( iter.next() )
    {
	mDynamicCastGet( const IOSubDir*, iosubd, &iter.ioObj() );
	if ( !iosubd )
	    continue;
	DBDir* dir = new DBDir( iosubd->dirName() );
	if ( !dir || dir->isBad() )
	    delete dir;
	else
	    dirs_ += dir;
    }
}


bool DBMan::isPresent( const DBKey& dbky ) const
{
    mLock4Read();
    const DBDir* dir = gtDir( dbky.dirID() );
    return dir && dir->isPresent( dbky.objID() );
}


ConstRefMan<DBDir> DBMan::fetch( DirID dirid ) const
{
    mLock4Read();
    return gtDir( dirid );
}


bool DBMan::isKeyString( const char* kystr ) const
{
    return DBKey::isValidString( kystr );
}


bool DBMan::setRootDir( const char* newdirnm )
{
    errmsg_.setEmpty();
    if ( !newdirnm || !*newdirnm || rootdir_ == newdirnm )
	return false;

    uiRetVal rv = isValidSurveyDir( newdirnm );
    if ( !rv.isOK() )
	return false;

    surveyToBeChanged.trigger();

    rootdbdir_->unRef(); rootdbdir_ = 0;
    deepUnRef( dirs_ );

    handleNewRootDir();
    setupCustomDataDirs( -1 );

    surveyChanged.trigger();
    afterSurveyChange.trigger();
}


DBDir* DBMan::gtDir( DirID dirid )
{
    if ( dirid.isInvalid() )
	return 0;
    else if ( dirid.getI() < 1 )
	return rootdbdir_;

    for ( int idx=0; idx<dirs_.size(); idx++ )
	if ( dirs_[idx]->dirID() == dirid )
	    return dirs_[idx];

    return 0;
}


const DBDir* DBMan::gtDir( DirID dirid ) const
{
    return const_cast<DBMan*>(this)->gtDir( dirid );
}



static TypeSet<DBMan::CustomDirData> cdds_;


uiRetVal DBMan::addCustomDataDir( const DBMan::CustomDirData& dd )
{
    if ( dd.dirnr_ <= 200000 )
	return uiRetVal( tr("Invalid Directory ID key passed for '%1'")
		    .arg( dd.desc_ ) );

    DBMan::CustomDirData cdd( dd );
    cdd.desc_.replace( ':', ';' );
    cdd.dirname_.clean();

    for ( int idx=0; idx<cdds_.size(); idx++ )
    {
	const DBMan::CustomDirData& existcdd = cdds_[idx];
	if ( existcdd.dirnr_ == cdd.dirnr_ )
	    return uiRetVal::OK();
    }

    cdds_ += dd;
    int idx = cdds_.size() - 1;
    if ( !DBM().isBad() )
	return DBM().setupCustomDataDirs( idx );

    return uiRetVal::OK();
}


uiRetVal DBMan::setupCustomDataDirs( int taridx )
{
    uiRetVal ret = uiRetVal::OK();
    mLock4Write();
    for ( int idx=0; idx<cdds_.size(); idx++ )
    {
	if ( taridx < 0 || idx == taridx )
	    setupCustomDataDir( cdds_[idx], ret );
    }
    return ret;
}


void DBMan::setupCustomDataDir( const CustomDirData& cdd, uiRetVal& rv )
{
    if ( gtDir( DirID::get(cdd.dirnr_) ) )
	return;

    FilePath fp( rootdir_, cdd.dirname_ );
    const BufferString dirnm = fp.fullPath();
    if ( !File::exists(dirnm) )
    {
	if ( !File::createDir(dirnm) )
	{
	    rv.add( tr("Cannot create '%1' directory in survey")
			.arg( cdd.dirname_ ) );
	    return;
	}
    }

    fp.add( ".omf" );
    const BufferString omffnm( fp.fullPath() );
    if ( !File::exists(omffnm) )
    {
	od_ostream strm( fp );
	if ( strm.isOK() )
	{
	    strm << GetProjectVersionName();
	    strm << "\nObject Management file\n";
	    strm << Time::getDateTimeString();
	    strm << "\n!\nID: " << cdd.dirnr_ << "\n!\n"
		      << cdd.desc_ << ": 1\n"
		      << cdd.desc_ << " directory: Gen`Stream\n"
			"$Name: Main\n!"
		      << od_endl;
	}
	if ( !strm.isOK() )
	{
	    rv.add( tr("Cannot write proper '.omf' file in '%1' directory")
			.arg( cdd.dirname_ ) );
	    return;
	}
    }

    IOSubDir* iosd = new IOSubDir( cdd.dirname_ );
    iosd->setDirName( rootdir_ );
    iosd->setKey( DBKey( DirID::get(cdd.dirnr_) ) );
    iosd->isbad_ = false;

    mLock4Write();
    if ( !rootdbdir_->addAndWrite(iosd) )
	rv.add( tr("Cannot write '.omf' file in '%1' directory")
		    .arg( rootdir_ ) );
    else
	dirs_ += new DBDir( iosd->dirName() );
}


#define mErrRet(str) { ret.add(str); return ret; }
#define mErrRetDoesntExist(fnm) \
    mErrRet( uiStrings::phrDoesntExist(toUiString(fnm)) )

uiRetVal DBMan::isValidDataRoot( const char* inpdirnm )
{
    uiRetVal ret = uiRetVal::OK();

    FilePath fp( inpdirnm ? inpdirnm : GetBaseDataDir() );
    const BufferString dirnm( fp.fullPath() );
    const uiString datarootstr = tr("OpendTect Data Directory '%1'")
					.arg( dirnm );
    if ( !File::isDirectory(dirnm) || !File::isWritable(dirnm) )
	mErrRetDoesntExist( datarootstr );

    fp.add( ".omf" );
    const BufferString omffnm( fp.fullPath() );
    if ( !File::exists(omffnm) )
	mErrRetDoesntExist( omffnm );

    fp.setFileName( ".survey" );
    if ( File::exists(fp.fullPath()) )
    {
	// probably we're in a survey. So let's check:
	fp.setFileName( "Seismics" );
	if ( File::isDirectory(fp.fullPath()) )
	    mErrRet( tr("%1 has '.survey' file").arg( datarootstr ) )
    }

    return ret;
}


uiRetVal DBMan::isValidSurveyDir( const char* dirnm )
{
    uiRetVal ret = uiRetVal::OK();

    FilePath fp( dirnm );
    const BufferString omffnm( fp.fullPath() );
    if ( !File::exists(omffnm) )
	mErrRetDoesntExist( omffnm );

    fp.setFileName( ".survey" );
    const BufferString survfnm( fp.fullPath() );
    if ( !File::exists(survfnm) )
	mErrRetDoesntExist( survfnm );

    fp.setFileName( "Seismics" );
    const BufferString seisdirnm( fp.fullPath() );
    if ( !File::isDirectory(seisdirnm) )
	mErrRetDoesntExist( seisdirnm );

    return ret;
}


void DBMan::findTempObjs( ObjectSet<IOObj>& ioobjs,
			 const IOObjSelConstraints* cnstrts ) const
{
    DBDirIter iter( *rootdbdir_ );
    while ( iter.next() )
    {
	mDynamicCastGet( const IOSubDir*, iosubd, &iter.ioObj() );
	if ( !iosubd )
	    continue;
	DBDir::getTmpIOObjs( iosubd->key().dirID(), ioobjs, cnstrts );
    }
}
