/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 1994 / Sep 2016
-*/


#include "dbdir.h"

#include "ascstream.h"
#include "file.h"
#include "filepath.h"
#include "dbman.h"
#include "iosubdir.h"
#include "safefileio.h"
#include "separstr.h"
#include "uistrings.h"


#define mIsBad() (readtime_ < 0)


static BufferString omfFileName( const char* dirnm )
{
    return BufferString( FilePath(dirnm,".omf").fullPath() );
}


#define mInitVarList \
	  curnr_(0) \
	, readtime_(-1) \
	, curtmpnr_(IOObj::tmpObjNrStart())


DBDir::DBDir()
	: mInitVarList
{
    mTriggerInstanceCreatedNotifier();
}


DBDir::DBDir( const DBDir& oth )
	: SharedObject(oth)
    	, dirname_(oth.dirname_)
	, dirid_(oth.dirid_)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


DBDir::DBDir( const char* dirnm )
	: dirname_(dirnm)
	, mInitVarList
{
    readFromFile( false );
    mTriggerInstanceCreatedNotifier();
}


DBDir::DBDir( DirID dirid )
	: mInitVarList
{
    fromDirID( dirid, false );
    mTriggerInstanceCreatedNotifier();
}


DBDir::DBDir( IOObjContext::StdSelType seltyp )
	: dirid_(IOObjContext::getStdDirData(seltyp)->id_)
	, mInitVarList
{
    fromDirID( dirid_, false );
    mTriggerInstanceCreatedNotifier();
}


void DBDir::fromDirID( DirID dirid, bool inc_old_tmps )
{
    if ( dirid.isInvalid() )
	return;
    ConstRefMan<DBDir> rootdbdir = DBM().fetchRootDir();
    if ( !rootdbdir || rootdbdir->isBad() )
	return;
    else if ( dirid.getI() < 1000 )
	{ *this = *rootdbdir; return; }
    const int idx = rootdbdir->gtIdx( ObjID::get(dirid.getI()) );
    if ( idx < 0 )
	return;

    BufferString dirnm( rootdbdir->dirname_ );
    const IOObj& ioobj = *rootdbdir->objs_[idx];
    if ( ioobj.isSubdir() )
	dirnm.set( ioobj.dirName() );

    const_cast<BufferString&>(dirname_).set( dirnm );
    readFromFile( inc_old_tmps );
    const_cast<DirID&>(dirid_) = dirid; // superfluous if .omf is ok ...
}


DBDir::~DBDir()
{
    sendDelNotif();
    deepErase( objs_ );
}


mImplMonitorableAssignment( DBDir, SharedObject )


void DBDir::copyClassData( const DBDir& oth )
{
    const_cast<BufferString&>(dirname_).set( oth.dirname_ );
    const_cast<DirID&>(dirid_) = oth.dirid_;
    readtime_ = oth.readtime_;
    deepCopyClone( objs_, oth.objs_ );
    curnr_ = oth.curnr_;
    curtmpnr_ = oth.curtmpnr_;
    errmsg_ = oth.errmsg_;
}


bool DBDir::readFromFile( bool incl_old_tmp )
{
    mLock4Write();

    SafeFileIO sfio( omfFileName(dirname_), false );
    if ( !sfio.open(true) )
	{ errmsg_ = sfio.errMsg(); return false; }

    const bool ret = readOmf( sfio.istrm(), incl_old_tmp );
    if ( !ret )
	sfio.closeFail();
    else
    {
	readtime_ = Time::getFileTimeInSeconds();
	sfio.closeSuccess();
	mSendEntireObjChgNotif();
    }
    return ret;
}


bool DBDir::readOmf( od_istream& strm, bool inc_old_tmps )
{
    if ( !strm.isOK() )
	return false;

    ascistream astream( strm );
    astream.next();
    if ( !strm.isOK() )
	return false;

    deepErase( objs_ );

    FileMultiString fms( astream.value() );
    DirID dirid = DirID::get( toInt(fms[0]) );
    const_cast<DirID&>(dirid_) = dirid;
    const ObjNrType newnr = fms.getIValue( 1 );
    curnr_ = newnr > 0 && !mIsUdf(newnr) ? newnr : 0;
    if ( IOObj::isTmpObjNr(curnr_) )
	curnr_ = 1;

    astream.next();
    while ( astream.type() != ascistream::EndOfFile )
    {
	IOObj* obj = IOObj::get( astream, dirname_, dirid, !inc_old_tmps );
	if ( !obj || obj->isBad() )
	    { delete obj; continue; }

	DBKey dbky( obj->key() );
	int objnr = dbky.objNr();
	if ( objnr < 0 )
	    objnr = dbky.groupNr();

	if ( objnr == 1 )
	    setName( obj->name() );
	setObjDirName( *obj );

	if ( !setObj(obj,false) )
	    continue;

	if ( IOObj::isTmpObjNr(objnr) )
	{
	    if ( objnr > curtmpnr_ )
		curtmpnr_ = objnr;
	}
	else
	{
	    if ( objnr > curnr_ )
		curnr_ = objnr;
	}
    }

    return true;
}


bool DBDir::isBad() const
{
    mLock4Read();
    return mIsBad();
}


bool DBDir::isOutdated() const
{
    mLock4Read();
    return readtime_ < 0
	|| File::getTimeInSeconds( omfFileName(dirname_) ) > readtime_;
}


#define mPrepUsrAccess(what_to_do_on_fail) \
    if ( reRead(false) && isBad() ) \
	{ what_to_do_on_fail; } \
    mLock4Read()


DBDir::size_type DBDir::size() const
{
    mPrepUsrAccess( return 0 );
    return objs_.size();
}


IOObj* DBDir::getEntry( ObjID objid ) const
{
    mPrepUsrAccess( return 0 );
    const size_type idxof = gtIdx( objid );
    return idxof < 0 ? 0 : objs_[idxof]->clone();
}


IOObj* DBDir::getEntryByName( const char* nm, const char* trgrpnm ) const
{
    mPrepUsrAccess( return 0 );
    const IOObj* obj = gtObjByName( nm, trgrpnm );
    return obj ? obj->clone() : 0;
}


IOObj* DBDir::getEntryByIdx( size_type idx ) const
{
    mPrepUsrAccess( return 0 );
    return objs_[idx] ? objs_[idx]->clone() : 0;
}


bool DBDir::commitChanges( const IOObj& ioobj )
{
    if ( ioobj.isSubdir() )
	{ pErrMsg("Cannot change IOSubDir"); return false; }
    else if ( ioobj.key().dirID() != dirid_ )
	{ pErrMsg("Commit asked for wrong DBDir"); return false; }

    IOObj* newioobj = ioobj.clone();
    if ( !newioobj )
	return false;

    return setObj( newioobj, true );
}


bool DBDir::permRemove( ObjID objid )
{
    if ( objid.isInvalid() )
	return true;

    mPrepUsrAccess( return false );

    size_type idxof = gtIdx( objid );
    if ( idxof < 0 )
	return true;

    if ( !mLock2Write() )
    {
	idxof = gtIdx( objid );
	if ( idxof < 0 )
	    return true;
    }

    mSendChgNotif( cEntryToBeRemoved(), DBKey(dirid_,objid).toInt64() );
    mReLock();
    idxof = gtIdx( objid );
    if ( idxof < 0 )
	return false;

    delete objs_.removeSingle( idxof );
    return writeToFile();
}


DBDir::size_type DBDir::indexOf( ObjID objid ) const
{
    mPrepUsrAccess( return -1 );
    return gtIdx( objid );
}


DBDir::size_type DBDir::gtIdx( ObjID objid ) const
{
    for ( size_type idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* ioobj = objs_[idx];
	if ( ioobj->key().objID() == objid )
	    return idx;
    }

    return -1;
}


bool DBDir::isPresent( const DBKey& dbky ) const
{
    return dbky.dirID() == dirid_ && indexOf( dbky.objID() ) >= 0;
}


bool DBDir::isPresent( ObjID objid ) const
{
    return indexOf( objid ) >= 0;
}


const IOObj* DBDir::gtObjByName( const char* nm, const char* trgrpnm ) const
{
    for ( size_type idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* ioobj = objs_[idx];
	if ( ioobj->name() == nm )
	{
	    if ( !trgrpnm || ioobj->group() == trgrpnm )
		return ioobj;
	}
    }
    return 0;
}


bool DBDir::reRead( bool force ) const
{
    if ( !force && !isOutdated() )
	return false;

    DBDir rddbdir( dirname_ );
    if ( !rddbdir.isBad() && rddbdir.size() > 1 )
	{ const_cast<DBDir&>(*this) = rddbdir; return true; }

    return false;
}


bool DBDir::setObj( IOObj* ioobj, bool writeafter )
{
    mLock4Write();

    DBKey dbky = ioobj->key();
    if ( dbky.dirID() != dirid_ )
    {
	dbky = gtNewKey( curnr_ );
	ioobj->setKey( dbky );
    }
    else
    {
	const IdxType curidxof = gtIdx( dbky.objID() );
	if ( curidxof >= 0 )
	{
	    delete objs_.replace( curidxof, ioobj );
	    ensureUniqueName( *ioobj );
	    mSendChgNotif( cEntryChanged(), dbky.toInt64() );
	    return !writeafter || writeToFile();
	}
    }

    ensureUniqueName( *ioobj );
    setObjDirName( *ioobj );
    objs_ += ioobj;
    mSendChgNotif( cEntryAdded(), dbky.toInt64() );

    return !writeafter || writeToFile();
}


bool DBDir::addAndWrite( IOObj* ioobj )
{
    mLock4Write();
    objs_ += ioobj;
    return writeToFile();
}


bool DBDir::ensureUniqueName( IOObj& ioobj ) const
{
    BufferString nm( ioobj.name() );
    size_type nr = 1;
    while ( gtObjByName(nm,ioobj.translator().buf()) )
    {
	nr++;
	nm.set( ioobj.name() ).add( " (" ).add( nr ).add( ")" );
    }
    if ( nr == 1 )
	return false;

    ioobj.setName( nm );
    return true;
}


#define mErrRet() \
{ \
    errmsg_ = uiStrings::phrCannotWriteDBEntry( toUiString(dirname_) ); \
    errmsg_.append( uiStrings::sCheckPermissions(), true ); \
    return false; \
}


bool DBDir::wrOmf( od_ostream& strm ) const
{
    ascostream astream( strm );
    if ( !astream.putHeader( "Object Management file" ) )
	mErrRet()
    FileMultiString fms( "0" );
    if ( dirid_.isValid() )
	fms.set( (int)dirid_.getI() );
    for ( size_type idx=0; idx<objs_.size(); idx++ )
    {
	const ObjNrType objnr = objs_[idx]->key().objID().getI();
	if ( !IOObj::isTmpObjNr(objnr) && objnr > curnr_ )
	    curnr_ = objnr;
    }
    fms += curnr_;
    astream.put( "ID", fms );
    astream.newParagraph();

    // First the main obj, then the subdirs, and lastly the the normal objs
    for ( int ipass=0; ipass<3; ipass++ )
    {
	for ( size_type idx=0; idx<objs_.size(); idx++ )
	{
	    const IOObj& obj = *objs_[idx];
	    bool writenow = false;
	    if ( ipass > 0 )
		writenow = obj.isSubdir() == (ipass==1);
	    else if ( obj.objID().getI() == 1 )
	       writenow = true;

	    if ( writenow && !obj.put(astream) )
		mErrRet()
	}
    }

    return true;
}


#undef mErrRet
#define mErrRet() \
{ \
    errmsg_ = sfio.errMsg(); \
    return false; \
}

bool DBDir::writeToFile() const
{
    mLock4Read();

    SafeFileIO sfio( omfFileName(dirname_), false );
    if ( !sfio.open(false) )
	mErrRet()

    if ( !wrOmf(sfio.ostrm()) )
	return false;

    if ( !sfio.closeSuccess() )
	mErrRet()

    return true;
}


void DBDir::setObjDirName( IOObj& ioobj )
{
    if ( ioobj.isSubdir() )
	ioobj.dirnm_ = dirname_;
    else
	ioobj.setDirName( FilePath(dirname_).fileName() );
}


DBKey DBDir::newTmpKey() const
{
    mLock4Write();
    return gtNewKey( curtmpnr_ );
}


DBKey DBDir::gtNewKey( const ObjNrType& nr ) const
{
    const_cast<ObjNrType&>(nr)++;
    return DBKey( dirid_, ObjID::get(nr) );
}


void DBDir::getTmpIOObjs( DirID dirid, ObjectSet<IOObj>& ioobjs,
			  const IOObjSelConstraints* cnstrnts )
{
    DBDir dbdir;
    dbdir.fromDirID( dirid, true );
    DBDirIter iter( dbdir );
    while ( iter.next() )
    {
	const IOObj& ioobj = iter.ioObj();
	if ( !ioobj.isTmp() )
	    continue;

	if ( !cnstrnts || cnstrnts->isGood(ioobj) )
	    ioobjs += ioobj.clone();
    }
}


DBDirIter::DBDirIter( const DBDir& dbdir )
    : MonitorableIter<DBDir::size_type>(dbdir,-1)
{
}


DBDirIter::DBDirIter( const DBDirIter& oth )
    : MonitorableIter<DBDir::size_type>(oth)
{
}


const DBDir& DBDirIter::dbDir() const
{
    return static_cast<const DBDir&>( monitored() );
}


DBDirIter& DBDirIter::operator =( const DBDirIter& oth )
{
    pErrMsg( "No assignment" );
    return *this;
}


const IOObj& DBDirIter::ioObj() const
{
    return isValid() ? *dbDir().objs_[curidx_] : IOObj::getInvalid();
}


DBKey DBDirIter::key() const
{
    return isValid() ? dbDir().objs_[curidx_]->key() : DBKey::getInvalid();
}


DBDirIter::ObjID DBDirIter::objID() const
{
    return isValid() ? dbDir().objs_[curidx_]->key().objID()
		     : ObjID::getInvalid();
}
