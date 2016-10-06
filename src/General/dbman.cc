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
#include "genc.h"
#include "survinfo.h"
#include "survgeom.h"
#include "iosubdir.h"
#include "safefileio.h"
#include "od_iostream.h"
#include "transl.h"
#include "uistrings.h"

#define mErrRet(str) { ret.add(str); return ret; }
#define mErrRetDoesntExist(fnm) \
    mErrRet( uiStrings::phrDoesntExist(::toUiString(fnm)) )

static ObjectSet<DBMan> dbmstack_;

DBMan& DBM()
{
    int curdbmidx = dbmstack_.size() - 1;
    if ( curdbmidx < 0 )
    {
	DBMan* newdbm = new DBMan;
	dbmstack_ += newdbm;
	curdbmidx = dbmstack_.size() - 1;
    }

    return *dbmstack_[curdbmidx];
}

void DBMan::pushDBM( DBMan* newdbm )
{
    if ( !newdbm )
	pFreeFnErrMsg("Null survinfo pushed");
    else
	dbmstack_ += newdbm;
}

bool DBMan::popDBM()
{
    const int nrdbms = dbmstack_.size();
    if ( nrdbms < 2 )
	return false;

    DBMan* dbm = dbmstack_.removeSingle( nrdbms-1 );
    delete dbm;
    return true;
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
    const BufferString lastsurvfnm = GetLastSurveyFileName();
    od_istream strm( lastsurvfnm );
    if ( strm.isOK() )
    {
	BufferString survnm;
	strm.getLine( survnm );
	if ( !survnm.isEmpty() )
	{
	    const BufferString basedir = GetBaseDataDir();
	    SurveyInfo::setSurveyLocation( basedir, survnm );
	    rootdir_ = FilePath( basedir, survnm ).fullPath();
	    handleNewRootDir();
	}
    }
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
    return FilePath( rootdir_ ).fileName();
}


DBMan::~DBMan()
{
    if ( rootdbdir_ )
	rootdbdir_->unRef();
    deepUnRef( dirs_ );
}


uiRetVal DBMan::handleNewRootDir()
{
    uiRetVal uirv = uiRetVal::OK();
    if ( rootdir_.isEmpty() )
	uirv = tr( "Directory for data storage is not set" );
    else
    {
	rootdbdir_ = new DBDir( rootdir_ );
	rootdbdir_->ref();
	if ( isBad() )
	    uirv = rootdbdir_->errMsg();
	else
	{
	    readDirs();
	    Survey::GMAdmin().fillGeometries(0);
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
	    dirs_ += dir;
	    mAttachCB( dir->objectChanged(), DBMan::dbdirChgCB );
	}
    }
}


bool DBMan::isPresent( DBKey dbky ) const
{
    mLock4Read();
    const DBDir* dir = gtDir( dbky.dirID() );
    return dir && dir->isPresent( dbky.objID() );
}


bool DBMan::isPresent( const char* objnm, const char* trgrpnm ) const
{
    PtrMan<IOObj> ioobj = getByName( objnm, trgrpnm );
    return ioobj ? true : false;
}


BufferString DBMan::nameOf( DBKey dbky ) const
{
    BufferString ret;
    IOObj* ioobj = get( dbky );
    if ( !ioobj )
	{ ret = "ID=<"; ret += dbky; ret += ">"; }
    else
	{ ret = ioobj->name(); delete ioobj; }

    return ret;
}


BufferString DBMan::nameFor( const char* kystr ) const
{
    if ( !isKeyString(kystr) )
	return kystr;

    const DBKey id = DBKey::getFromString( kystr );
    return nameOf( id );
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


IOObj* DBMan::get( DBKey dbky ) const
{
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
    const BufferString basekey( bky, basekey.isEmpty() ? "" : "." );

    BufferString iopkey( basekey );
    iopkey.add( sKey::ID() );
    BufferString res = iop.find( iopkey );
    if ( res.isEmpty() )
    {
	iopkey.set( basekey ).add( sKey::Name() );
	res = iop.find( iopkey );
	if ( res.isEmpty() )
	    { errmsg = tr( "Please specify '%1'", iopkey ); return 0; }

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

    IOObj* ioobj = get( DBKey::getFromString(res) );
    if ( !ioobj )
	errmsg = tr( "Value for %1 is invalid.", iopkey );

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
	const DBKey newky( mktmp ? dbdir->newTmpKey() : DBKey(dbdir->dirID()) );
	ioobj = crWriteIOObj( ctio, newky, translidx );
	if ( ioobj )
	{
	    ioobj->pars().merge( ctio.ctxt_.toselect_.require_ );
	    dbdir.getNonConstPtr()->commitChanges( *ioobj );
	}
    }

    ctio.setObj( ioobj.release() );
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
    return dbdir ? dbdir->commitChanges( ioobj ) : false;
}


bool DBMan::removeEntry( DBKey dbky )
{
    const ObjID objid = dbky.objID();
    if ( objid.isInvalid() )
	return false;

    RefMan<DBDir> dbdir = getDBDir( dbky.dirID() );
    return dbdir ? dbdir->permRemove( objid ) : false;
}


bool DBMan::isKeyString( const char* kystr ) const
{
    return DBKey::isValidString( kystr );
}


uiRetVal DBMan::setDataSource( const IOPar& iop )
{
    return setDataSource( iop.find(sKey::DataRoot()), iop.find(sKey::Survey()));
}


void DBMan::doNotChangeTheSurveyNow()
{
    mLock4Write();
    surveychangeuserabort_ = true;
}


uiRetVal DBMan::setDataSource( const char* dr, const char* sd )
{
    mLock4Read();

    uiRetVal rv = SurveyInfo::setSurveyLocation( dr, sd );
    if ( !rv.isOK() )
	return rv;

    const BufferString newdirnm = SI().getSurvDirFullPath();
    if ( rootdir_ == newdirnm )
	return uiRetVal::OK();

    mUnlockAllAccess();
    surveyChangeOK.trigger();
    if ( surveychangeuserabort_ )
    {
	surveychangeuserabort_ = false;
	FilePath fp( rootdir_ );
	SurveyInfo::setSurveyLocation( fp.pathOnly(), fp.fileName() );
	rv.set( tr("User abort from survey change") );
	return rv;
    }

    surveyToBeChanged.trigger();

    mReLock();
    mLock2Write();
    if ( rootdbdir_ )
	{ rootdbdir_->unRef(); rootdbdir_ = 0; }
    deepUnRef( dirs_ );
    rootdir_.set( newdirnm );
    rv = handleNewRootDir();
    if ( !rv.isOK() )
	return rv; // disaster ...
    setupCustomDataDirs( -1 );
    mUnlockAllAccess();

    surveyChanged.trigger();
    afterSurveyChange.trigger();

    return rv;
}


DBDir* DBMan::gtDir( DirID dirid ) const
{
    if ( dirid.isInvalid() )
	return 0;
    else if ( dirid.getI() < 1 )
	return const_cast<DBDir*>(rootdbdir_);

    for ( int idx=0; idx<dirs_.size(); idx++ )
	if ( dirs_[idx]->dirID() == dirid )
	    return const_cast<DBDir*>(dirs_[idx]);

    return 0;
}


void DBMan::dbdirChgCB( CallBacker* cb )
{
    mGetMonitoredChgData( cb, chgdata );
    const DBKey dbky = DBKey::getFromInt64( chgdata.ID() );
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
    uiRetVal ret = uiRetVal::OK();
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

    FilePath fp( dirnm, ".omf" );
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


static bool validOmf( const char* dir, uiRetVal& uirv )
{
    FilePath fp( dir ); fp.add( ".omf" );
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
    uiRetVal uirv = uiRetVal::OK();
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
	DBDir::getTmpIOObjs( iosubd->key().dirID(), ioobjs, cnstrts );
    }
}
