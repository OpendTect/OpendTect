/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 3-8-1994
-*/


#include "applicationdata.h"
#include "commandlineparser.h"
#include "dbman.h"
#include "dbdir.h"
#include "oddirs.h"
#include "file.h"
#include "filepath.h"
#include "timefun.h"
#include "genc.h"
#include "staticstring.h"
#include "survinfo.h"
#include "survgeom.h"
#include "survgeommgr.h"
#include "iosubdir.h"
#include "safefileio.h"
#include "settings.h"
#include "od_iostream.h"
#include "transl.h"
#include "keystrs.h"
#include "taskrunner.h"
#include "uistrings.h"

#define mErrRet(str) { ret.add(str); return ret; }
#define mErrRetDoesntExist(fnm) \
    mErrRet( uiStrings::phrFileDoesNotExist(fnm) )

mGlobal(General) BufferString DBMan_nameOf(const DBKey&);
mGlobal(General) BufferString DBMan_mainFileOf(const DBKey&);
mGlobal(General) bool DBMan_implExist(const DBKey&);
mGlobal(General) IOObj* DBMan_getIOObj(const DBKey&);


class DBMManager : public CallBacker
{
public:

    DBMManager()
	: dbm_(*new DBMan())
    {
	mAttachCB( dbm_.applicationClosing, DBMManager::closedCB );
    }

    ~DBMManager()
    {
	detachAllNotifiers();
	delete &dbm_;
    }

    DBMan& DBM()	    { return dbm_; }

    void applicationClosing()
    {
	if ( !closed_ )
	{
	    mDetachCB( dbm_.applicationClosing, DBMManager::closedCB );
	    dbm_.applClosing();
	}
    }

    static Threads::Lock	lock_;

private:

    void    closedCB( CallBacker* )
    {
	closed_ = true;
    }

    DBMan& dbm_;
    bool    closed_ = false;

};

static PtrMan<DBMManager> theinstmgr = nullptr;
Threads::Lock DBMManager::lock_( true );

void applicationClosing()
{
    Threads::Locker locker( DBMManager::lock_ );
    if ( theinstmgr )
	theinstmgr->applicationClosing();
}


DBMan& DBM()
{
    Threads::Locker locker( DBMManager::lock_ );
    if ( !theinstmgr )
    {
	theinstmgr = new DBMManager;
	NotifyExitProgram( applicationClosing );
    }

    return theinstmgr->DBM();
}


DBMan::DBMan()
    : rootdbdir_(0)
    , surveyToBeChanged(this)
    , surveyChanged(this)
    , afterSurveyChange(this)
    , applicationClosing(this)
    , entryAdded(this)
    , entryRemoved(this)
    , entryToBeRemoved(this)
    , surveyChangeOK(this)
    , surveychangeuserabort_(false)
{
}


void DBMan::initFirst()
{
    if ( isBad() )
    {
	BufferString survnm( GetLastSurveyDirName() );
	if ( !survnm.isEmpty() )
	{
	    const SurveyDiskLocation survloc( survnm );
	    SurveyInfo::setSurveyLocation( survloc, false );
	    survdir_ = survloc.fullPath();
	    handleNewSurvDir();
	}
    }
}


uiRetVal DBMan::setDataSource( const IOPar& iop, bool forcerefresh )
{
    return setDataSource( iop.find(sKey::DataRoot()), iop.find(sKey::Survey()),
			  forcerefresh );
}


uiRetVal DBMan::setDataSource( const CommandLineParser& clp, bool* ischgd )
{
    bool iscur = true;
    const BufferString newpath = clp.getFullSurveyPath( &iscur );
    if ( ischgd )
	*ischgd = !iscur;
    return DBM().setDataSource( newpath );
}


void DBMan::setSurveyChangeUserAbort()
{
    mLock4Write();
    surveychangeuserabort_ = true;
}


void DBMan::setSurveyChangeAbortReason( uiRetVal reason )
{
    mLock4Write();
    surveychangeabortreason_ = reason;
}


uiRetVal DBMan::setDataSource( const char* fullpath, bool forcerefresh )
{
    File::Path fp( fullpath );
    const BufferString pathnm( fp.pathOnly() );
    const BufferString filenm( fp.fileName() );
    return setDataSource( pathnm, filenm, forcerefresh );
}


mGlobal(Basic) void SetBaseDataDir(const char*);

uiRetVal DBMan::setDataSource( const char* dr, const char* sd, bool frcrefresh )
{
    mLock4Read();

    SurveyDiskLocation survloc( sd, dr );
    uiRetVal rv = SurveyInfo::setSurveyLocation( survloc, frcrefresh );
    if ( !rv.isOK() )
	return rv;

    const BufferString newdirnm = SI().getFullDirPath();
    if ( !frcrefresh && survdir_ == newdirnm )
	return uiRetVal::OK();

    mUnlockAllAccess();

    surveyChangeOK.trigger();
    if ( surveychangeuserabort_ || !surveychangeabortreason_.isEmpty() )
    {
	survloc.set( File::Path(survdir_) );
	SurveyInfo::setSurveyLocation( survloc, true );
	if ( surveychangeuserabort_ )
	    rv.set( uiStrings::sCancel() );
	else
	    rv.set( surveychangeabortreason_ );

	surveychangeuserabort_ = false;
	surveychangeabortreason_.setEmpty();
	return rv;
    }

    surveyToBeChanged.trigger();

    mReLock();
    mLock2Write();
    survdir_.set( newdirnm );
    rv = doReRead();
    SetBaseDataDir( SI().basePath() );
    mUnlockAllAccess();
    if ( !rv.isOK() )
	return rv; // disaster ...

    setupCustomDataDirs( -1 );
    surveyChanged.trigger();
    afterSurveyChange.trigger();

    return rv;
}


uiRetVal DBMan::reRead()
{
    mLock4Write();
    return doReRead();
}


uiRetVal DBMan::doReRead()
{
    if ( rootdbdir_ )
	{ rootdbdir_->unRef(); rootdbdir_ = 0; }
    deepUnRef( dbdirs_ );
    return handleNewSurvDir();
}


bool DBMan::isBad() const
{
    return !rootdbdir_ || rootdbdir_->isBad();
}


BufferString DBMan::surveyName() const
{
    return SI().name();
}


BufferString DBMan::surveyDirectoryName() const
{
    return File::Path( survdir_ ).fileName();
}


DBMan::~DBMan()
{
    if ( rootdbdir_ )
	rootdbdir_->unRef();
    deepUnRef( dbdirs_ );
}


uiRetVal DBMan::handleNewSurvDir()
{
    uiRetVal uirv;
    if ( survdir_.isEmpty() )
	uirv = tr("Directory for data storage is not set");
    else
    {
	rootdbdir_ = new DBDir( survdir_ );
	rootdbdir_->ref();
	if ( isBad() )
	    uirv = rootdbdir_->errMsg();
	else
	{
	    readDirs();
	    Survey::GMAdmin().fillGeometries(SilentTaskRunnerProvider());
	}
    }
    return uirv;
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
	{
	    dir->ref();
	    dbdirs_ += dir;
	    mAttachCB( dir->objectChanged(), DBMan::dbdirChgCB );
	}
    }
}


bool DBMan::isPresent( const DBKey& dbky ) const
{
    if ( !dbky.isValid() || !dbky.isInCurrentSurvey() )
	return false;

    mLock4Read();
    const DBDir* dir = gtDir( dbky.dirID() );
    return dir && dir->isPresent( dbky.objID() );
}


bool DBMan::isPresent( const char* objnm, const char* trgrpnm ) const
{
    PtrMan<IOObj> ioobj = getByName( objnm, trgrpnm );
    return ioobj ? true : false;
}


static BufferString getDBKeyStr( const DBKey& dbky )
{
    BufferString ret( "ID=<" );
    ret.add( dbky ).add( ">" );
    return ret;
}


BufferString DBMan::nameFor( const char* kystr ) const
{
    if ( !isKeyString(kystr) )
	return kystr;

    const DBKey id = DBKey( kystr );
    return DBMan_nameOf( id );
}


BufferString DBMan::mainFileOf( const DBKey& dbky ) const
{
    PtrMan<IOObj> ioobj = get( dbky );
    if ( ioobj )
	return ioobj->mainFileName();
    return BufferString();
}


ConstRefMan<DBDir> DBMan::fetchDir( DirID dirid ) const
{
    mLock4Read();
    return gtDir( dirid );
}


ConstRefMan<DBDir> DBMan::fetchDir( const IOObjContext& ctxt ) const
{
    return fetchDir( ctxt.getSelDirID() );
}


ConstRefMan<DBDir> DBMan::fetchDir( IOObjContext::StdSelType seltyp ) const
{
    const IOObjContext::StdDirData* sdd = IOObjContext::getStdDirData( seltyp );
    if ( sdd )
	return fetchDir( sdd->id_ );
    return 0;
}


ConstRefMan<DBDir> DBMan::findDir( const char* trgroupnm ) const
{
    mLock4Read();
    for ( int idx=0; idx<dbdirs_.size(); idx++ )
    {
	const auto* dbdir = dbdirs_[idx];
	if ( dbdir->hasObjectsWithGroup(trgroupnm) )
	    { dbdir->reRead( false ); return dbdir; }
    }
    return 0;
}


IOObj* DBMan::get( const DBKey& dbky ) const
{
    if ( !dbky.isInCurrentSurvey() )
	return ::getIOObj( dbky );

    if ( isBad() )
	return 0;
    const DirID dirid = dbky.dirID();
    if ( dirid.isInvalid() )
	return 0;
    ConstRefMan<DBDir> dbdir = fetchDir( dirid );
    if ( !dbdir )
	return 0;

    return dbdir->getEntry( dbky.objID() );
}


IOObj* DBMan_getIOObj( const DBKey& dbky )
{
    if ( !dbky.isValid() )
	return 0;

    if ( dbky.isInCurrentSurvey() )
	return DBM().get( dbky );

    if ( !dbky.hasSurveyLocation() )
	return 0;

    const SurveyDiskLocation& survloc = dbky.surveyDiskLocation();
    ConstRefMan<DBDir> survrootdbdir = new DBDir( survloc.fullPath() );
    IOObj* ret = 0;
    if ( !survrootdbdir->isBad() )
    {
	const DBKey::ObjID dirid = DBKey::ObjID( dbky.groupID().getI() );
	const IOObj* iosubdir = survrootdbdir->getEntry( dirid );
	if ( iosubdir )
	{
	    const BufferString datadirnm( iosubdir->fullUserExpr(true) );
	    ConstRefMan<DBDir> objdbdir = new DBDir( datadirnm );
	    const IOObj* ioobj = objdbdir->getEntry( dbky.objID() );
	    if ( ioobj )
	    {
		ret = ioobj->clone();
		ret->setAbsDirectory( datadirnm );
	    }
	}
    }

    return ret;
}


BufferString DBMan_nameOf( const DBKey& dbky )
{
    BufferString ret;
    if ( !dbky.isValid() )
	return ret;

    IOObj* ioobj = DBMan_getIOObj( dbky );
    if ( !ioobj )
	return getDBKeyStr( dbky );
    else
    {
	ret.set( ioobj->name() );
	delete ioobj;
    }
    return ret;
}


BufferString DBMan_mainFileOf( const DBKey& dbky )
{
    BufferString ret;
    if ( !dbky.isValid() )
	return ret;

    IOObj* ioobj = DBMan_getIOObj( dbky );
    if ( ioobj )
    {
	ret.set( ioobj->mainFileName() );
	delete ioobj;
    }
    return ret;
}


bool DBMan_implExist( const DBKey& dbky )
{
    bool ret = false;
    if ( dbky.isValid() )
    {
	IOObj* ioobj = DBMan_getIOObj( dbky );
	if ( ioobj )
	{
	    ret = ioobj->implExists( true );
	    delete ioobj;
	}
    }
    return ret;
}


IOObj* DBMan::getByName( const IOObjContext& ctxt, const char* objname ) const
{
    if ( !objname || !*objname || isBad() )
	return 0;

    ConstRefMan<DBDir> dbdir = fetchDir( ctxt );
    if ( !dbdir )
	return 0;

    IOObj* ret = dbdir->getEntryByName( objname, ctxt.translatorGroupName() );
    if ( !ret )
	return ret;

    if ( ctxt.validIOObj(*ret) )
	return ret;

    delete ret;
    return 0;
}


IOObj* DBMan::getByName( IOObjContext::StdSelType seltyp,
			 const char* objname, const char* tgname ) const
{
    if ( !objname || !*objname || isBad() )
	return 0;

    ConstRefMan<DBDir> dbdir = fetchDir( seltyp );
    return dbdir ? dbdir->getEntryByName( objname, tgname ) : 0;
}


IOObj* DBMan::getByName( const char* objname, const char* tgname ) const
{
    if ( !objname || !*objname || isBad() )
	return 0;

    const FixedString trlgrpname( tgname );
    for ( int itype=0; itype<TranslatorGroup::groups().size(); itype++ )
    {
	const TranslatorGroup& tgrp = *TranslatorGroup::groups()[itype];
	if ( !trlgrpname.isEmpty() && trlgrpname != tgrp.groupName() )
	    continue;

	ConstRefMan<DBDir> dbdir = fetchDir( tgrp.ioCtxt().getSelDirID() );
	if ( !dbdir )
	    continue;

	IOObj* res = dbdir->getEntryByName( objname, tgname );
	if ( res )
	    return res;
    }

    return 0;
}


IOObj* DBMan::getFromPar( const IOPar& iop, const char* bky,
			  const IOObjContext& ctxt, bool mknew,
			  uiString& errmsg ) const
{
    const BufferString basekey( bky, bky && *bky ? "." : "" );

    BufferString iopkey( basekey );
    iopkey.add( sKey::ID() );
    BufferString res = iop.find( iopkey );
    if ( res.isEmpty() )
    {
	iopkey.set( basekey ).add( sKey::Name() );
	res = iop.find( iopkey );
	if ( res.isEmpty() )
	    { errmsg = tr("Please specify '%1'").arg(iopkey); return 0; }

	if ( !DBKey::isValidString(res) )
	{
	    ConstRefMan<DBDir> dbdir = DBM().fetchDir( ctxt.getSelDirID() );
	    if ( dbdir )
	    {
		PtrMan<IOObj> ioob = dbdir->getEntryByName( res.buf(),
			ctxt.trgroup_ ? ctxt.trgroup_->groupName().str() : 0 );
		if ( ioob )
		    res = ioob->key().toString();
		else if ( mknew )
		{
		    CtxtIOObj ctio( ctxt );
		    ctio.setName( res );
		    const_cast<DBMan*>(this)->getEntry( ctio );
		    if ( ctio.ioobj_ )
		    {
			res = ctio.ioobj_->key().toString();
			ctio.setObj( 0 );
		    }
		}
	    }
	}
    }

    IOObj* ioobj = DBMan_getIOObj( DBKey(res) );
    if ( !ioobj )
	errmsg = tr("Value for %1 is invalid.").arg( iopkey );

    return ioobj;
}


IOObj* DBMan::getFirst( const IOObjContext& ctxt, int* nrpresent ) const
{
    DBDirEntryList del( ctxt );
    if ( del.isEmpty() )
	return 0;
    if ( nrpresent )
	*nrpresent = del.size();

    return del.ioobj( 0 ).clone();
}


void DBMan::getEntry( CtxtIOObj& ctio, bool mktmp, int translidx )
{
    ctio.setObj( 0 );
    if ( ctio.ctxt_.name().isEmpty() )
	return;

    ctio.ctxt_.fillTrGroup();
    ConstRefMan<DBDir> dbdir = fetchDir( ctio.ctxt_.getSelDirID() );
    if ( !dbdir )
	return;

    PtrMan<IOObj> ioobj = dbdir->getEntryByName( ctio.ctxt_.name(),
					ctio.ctxt_.translatorGroupName() );
    if ( ioobj && ctio.ctxt_.translatorGroupName() != ioobj->group() )
	ioobj = 0;

    if ( !ioobj )
    {
	const DBKey newky( mktmp ? dbdir->newTmpKey() : dbdir->newKey() );
	ioobj = crWriteIOObj( ctio, newky, translidx );
	if ( ioobj )
	{
	    ioobj->pars().merge( ctio.ctxt_.toselect_.require_ );
	    if ( !dbdir.getNonConstPtr()->commitChanges(*ioobj) )
		return;
	}
    }

    ctio.setObj( ioobj.release() );
}


BufferString DBMan::getDirectoryNameOf( DirID dirid, bool fullpath ) const
{
    BufferString ret;
    if ( rootdbdir_ )
    {
	DBDirIter iter( *rootdbdir_ );
	while ( iter.next() )
	{
	    mDynamicCastGet( const IOSubDir*, iosubd, &iter.ioObj() );
	    if ( iosubd && iosubd->key().objID().getI() == dirid.getI() )
		{ ret = iosubd->dirName(); break; }
	}
    }

    if ( !fullpath )
	ret = File::Path( ret ).fileName();
    return ret;
}


#define mIsValidTransl(transl) \
    IOObjSelConstraints::isAllowedTranslator( \
	    transl->userName(),ctio.ctxt_.toselect_.allowtransls_) \
	    && transl->isUserSelectable(false)

IOObj* DBMan::crWriteIOObj( const CtxtIOObj& ctio, DBKey newkey,
			    int translidx ) const
{
    const ObjectSet<const Translator>& tplts = ctio.ctxt_.trgroup_->templates();

    if ( tplts.isEmpty() )
    {
	BufferString msg( "Translator Group '",ctio.ctxt_.translatorGroupName(),
			  "is empty." );
	msg.add( ".\nCannot create a default write IOObj for " )
	   .add( ctio.ctxt_.name() );
	pErrMsg( msg ); return 0;
    }

    const Translator* templtr = 0;

    if ( tplts.validIdx(translidx) )
	templtr = ctio.ctxt_.trgroup_->templates()[translidx];
    else if ( !ctio.ctxt_.deftransl_.isEmpty() )
	templtr = ctio.ctxt_.trgroup_->getTemplate(ctio.ctxt_.deftransl_,true);
    if ( !templtr )
    {
	translidx = ctio.ctxt_.trgroup_->defTranslIdx();
	if ( mIsValidTransl(tplts[translidx]) )
	    templtr = tplts[translidx];
	else
	{
	    for ( int idx=0; idx<tplts.size(); idx++ )
	    {
		if ( mIsValidTransl(tplts[idx]) )
		    { templtr = tplts[idx]; break; }
	    }
	}
    }

    return templtr ? templtr->createWriteIOObj( ctio.ctxt_, newkey ) : 0;
}


RefMan<DBDir> DBMan::getDBDir( DirID dirid )
{
    if ( dirid.isInvalid() )
	{ pErrMsg("No DirID"); return 0; }
    else if ( isBad() )
	return 0;

    mLock4Read();
    RefMan<DBDir> dbdir = gtDir( dirid );
    if ( !dbdir )
	{ pErrMsg( BufferString("DBDir ",dirid.getI()," not found") ); }

    return dbdir;
}


bool DBMan::setEntry( const IOObj& ioobj )
{
   RefMan<DBDir> dbdir = getDBDir( ioobj.key().dirID() );
   if ( !dbdir )
       return false;

   if ( !ioobj.key().hasValidObjID() )
       const_cast<IOObj&>( ioobj ).setKey( dbdir->newKey() );

   return dbdir->commitChanges( ioobj );
}


bool DBMan::removeEntry( const DBKey& dbky )
{
    const ObjID objid = dbky.objID();
    if ( objid.isInvalid() || !dbky.isInCurrentSurvey() )
	return false;

    RefMan<DBDir> dbdir = getDBDir( dbky.dirID() );
    return dbdir ? dbdir->permRemove( objid ) : false;
}


bool DBMan::isKeyString( const char* kystr ) const
{
    return DBKey::isValidString( kystr );
}


DBDir* DBMan::gtDir( DirID dirid ) const
{
    if ( dirid.isInvalid() )
	return 0;
    else if ( dirid.getI() < 1 )
	return const_cast<DBDir*>(rootdbdir_);

    for ( int idx=0; idx<dbdirs_.size(); idx++ )
    {
	if ( dbdirs_[idx]->dirID() == dirid )
	{
	    DBDir* ret = const_cast<DBDir*>( dbdirs_[idx] );
	    ret->reRead( false );
	    return ret;
	}
    }

    return 0;
}


void DBMan::dbdirChgCB( CallBacker* cb )
{
    mGetMonitoredChgData( cb, chgdata );
    const DBKey dbky = DBKey::getFromI64( chgdata.ID() );
    if ( chgdata.changeType() == DBDir::cEntryAdded() )
	entryAdded.trigger( dbky );
    else if ( chgdata.changeType() == DBDir::cEntryRemoved() )
	entryRemoved.trigger( dbky );
    else if ( chgdata.changeType() == DBDir::cEntryToBeRemoved() )
	entryToBeRemoved.trigger( dbky );
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
    uiRetVal ret;
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

    File::Path fp( survdir_, cdd.dirname_ );
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
	    strm << Time::getUsrDateTimeString();
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
    iosd->setDirName( survdir_ );
    iosd->setKey( DBKey( DirID::get(cdd.dirnr_) ) );
    iosd->isbad_ = false;

    mLock4Write();
    if ( !rootdbdir_->addAndWrite(iosd) )
	rv.add( tr("Cannot write '.omf' file in '%1' directory")
		    .arg( survdir_ ) );
    else
	dbdirs_ += new DBDir( iosd->dirName() );
}


uiRetVal DBMan::isValidDataRoot( const char* dirnm )
{
    // no extra requirements ... for now ...
    return SurveyInfo::isValidDataRoot( dirnm );
}


uiRetVal DBMan::isValidSurveyDir( const char* dirnm )
{
    // no extra requirements ... for now ...
    return SurveyInfo::isValidSurveyDir( dirnm );
}


static bool validOmf( const char* dir, uiRetVal& uirv )
{
    File::Path fp( dir ); fp.add( ".omf" );
    BufferString fname = fp.fullPath();
    uiString msg;
    if ( !File::checkDirectory(fname,true,msg) )
	{ uirv.add( msg ); return false; }

    SafeFileIO fstrm( fname );
    const bool success = fstrm.open( true, true );
    msg = fstrm.errMsg();
    fstrm.closeSuccess();
    if ( success )
	return true;

    fp.setFileName( ".omb" );
    BufferString fnamebak = fp.fullPath();
    SafeFileIO fstrmback( fnamebak );
    const bool cancopy = fstrmback.open( true, true );
    fstrmback.closeSuccess();
    if ( !cancopy )
	{ uirv.add( msg ); return false; }

    uirv = od_static_tr("IOManvalidOmf",
		       "Restoring .omf file from .omb file");
    if ( !File::copy(fname,fnamebak,&msg) )
	{ uirv.add( msg ); return false; }

    return true;
}



uiRetVal DBMan::checkSurveySetupValid()
{
    uiRetVal uirv;
    const BufferString basedatadir( GetBaseDataDir() );
    if ( basedatadir.isEmpty() )
    {
	uirv = tr("Please set the environment variable DTECT_DATA.");
	return uirv;
    }
    else if ( !File::exists(basedatadir) )
    {
	uirv = tr("$DTECT_DATA=%1\n"
		    "This is not a valid OpendTect data storage directory.")
		   .arg( toUiString(GetBaseDataDir()) );
	return uirv;
    }
    else if ( !validOmf(basedatadir,uirv) )
    {
	uirv.add( tr("$DTECT_DATA=%1\n"
	       "This is not a valid OpendTect data storage directory.\n")
	       .arg( GetBaseDataDir() ) );
	return uirv;
    }

    const BufferString survdir = GetDataDir();
    if ( survdir != basedatadir && File::isDirectory(survdir) )
    {
	const bool noomf = !validOmf( survdir, uirv );
	uiRetVal uirv2 = isValidSurveyDir( survdir );
	uirv.add( uirv2 );
	const bool nosurv = !uirv2.isOK();

	if ( !noomf && !nosurv )
	{
	    if ( !DBM().isBad() )
		return uiRetVal::OK(); // This is normal

	    // But what's wrong here? In any case - survey is not good.
	}
    }

    // Survey name in ~/.od/survey may be invalid or absent. If there, lose it.
    const BufferString lastsurvfname = GetLastSurveyFileName();
    if ( File::exists(lastsurvfname) && !File::remove(lastsurvfname) )
	uirv.add( tr("The file:\n%1\ncontains an invalid survey.\n\n"
		    "Please remove this file")
		   .arg( lastsurvfname ) );

    return uirv;
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
	const DirID dirid( DirID::get(iosubd->key().objID().getI()) );
	DBDir::getTmpIOObjs( dirid, ioobjs, cnstrts );
    }
}


mExternC(General) const char* setDBMDataSource( const char* fullpath,
						bool refresh )
{
    mDeclStaticString(ret);
    const uiRetVal uirv = DBM().setDataSource( fullpath, refresh );
    ret = uirv.getText();
    return ret.buf();
}
