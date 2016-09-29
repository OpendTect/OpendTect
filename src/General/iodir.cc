/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 2-8-1994
-*/


#include "iodir.h"

#include "ascstream.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "iosubdir.h"
#include "safefileio.h"
#include "separstr.h"
#include "uistrings.h"

static Threads::Lock static_read_lock;

#define mIsBad() (readtime_ < 0)


static BufferString omfFileName( const char* dirnm )
{
    return BufferString( FilePath(dirnm,".omf").fullPath() );
}


#define mInitVarList \
	  curnr_(0) \
	, readtime_(-1) \
	, curtmpnr_(IOObj::tmpObjNrStart())


IODir::IODir()
	: mInitVarList
{
    mTriggerInstanceCreatedNotifier();
}


IODir::IODir( const IODir& oth )
	: SharedObject(oth)
    	, dirname_(oth.dirname_)
	, dirid_(oth.dirid_)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


IODir::IODir( const char* dirnm )
	: dirname_(dirnm)
	, mInitVarList
{
    build( false );
    mTriggerInstanceCreatedNotifier();
}


IODir::IODir( DirID dirid )
	: mInitVarList
{
    init( dirid, false );
    mTriggerInstanceCreatedNotifier();
}


IODir::IODir( IOObjContext::StdSelType seltyp )
	: dirid_(IOObjContext::getStdDirData(seltyp)->id_)
	, mInitVarList
{
    init( dirid_, false );
    mTriggerInstanceCreatedNotifier();
}



void IODir::init( DirID dirid, bool inc_old_tmps )
{
    if ( dirid.isInvalid() )
	dirid.setI( 0 );
    IOObj* ioobj = getObj( DBKey(dirid), errmsg_ );
    if ( !ioobj )
	return;

    BufferString dirnm( ioobj->dirName() );
    FilePath fp( dirnm );
    if ( !fp.isAbsolute() )
    {
	fp.set( IOM().rootDir() ).add( dirnm );
	dirnm = fp.fullPath();
    }
    delete ioobj;
    const_cast<BufferString&>(dirname_).set( dirnm );

    build( inc_old_tmps );
    const_cast<DirID&>(dirid_) = dirid; // superfluous if .omf is ok ...
}


IODir::~IODir()
{
    sendDelNotif();
    deepErase( objs_ );
}


mImplMonitorableAssignment( IODir, SharedObject )


void IODir::copyClassData( const IODir& oth )
{
    const_cast<BufferString&>(dirname_).set( oth.dirname_ );
    const_cast<DirID&>(dirid_) = oth.dirid_;
    readtime_ = oth.readtime_;
    deepCopyClone( objs_, oth.objs_ );
    curnr_ = oth.curnr_;
    curtmpnr_ = oth.curtmpnr_;
    errmsg_ = oth.errmsg_;
}


bool IODir::isBad() const
{
    mLock4Read();
    return mIsBad();
}


bool IODir::gtIsOutdated() const
{
    return readtime_ < 0
	|| File::getTimeInSeconds( omfFileName(dirname_) ) > readtime_;
}


bool IODir::isOutdated() const
{
    mLock4Read();
    return gtIsOutdated();
}


IODir::size_type IODir::size() const
{
    mLock4Read();
    return objs_.size();
}


IOObj* IODir::getEntryByIdx( size_type idx ) const
{
    mLock4Read();
    return objs_[idx] ? objs_[idx]->clone() : 0;
}


IOObj* IODir::getEntry( const DBKey& ky ) const
{
    mLock4Read();
    const IOObj* obj = gtObj( ky );
    return obj ? obj->clone() : 0;
}


void IODir::reRead()
{
    mLock4Write();
    doReRead( true );
}


bool IODir::build( bool inc_old_tmps )
{
    Threads::Locker locker( static_read_lock );
    return doRead( dirname_, this, errmsg_, DirID::getInvalid().getI(),
		    inc_old_tmps );
}


/*

IOObj* IODir::getMain( const char* dirnm, uiString& errmsg )
{
    Threads::Locker locker( static_read_lock );
    return doRead( dirnm, 0, errmsg, 1 );
}

*/


bool IODir::writeNow() const
{
    mLock4Read();
    return doWrite();
}


const IOObj* IODir::main() const
{
    for ( size_type idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* ioobj = objs_[idx];
	if ( ioobj->objID().getI() == 1 )
	    return ioobj;
    }
    return 0;
}


IOObj* IODir::doRead( const char* dirnm, IODir* dirptr, uiString& errmsg,
		      ObjNrType reqobjnr, bool incl_old_tmp )
{
    SafeFileIO sfio( omfFileName(dirnm), false );
    if ( !sfio.open(true) )
	{ errmsg = sfio.errMsg(); return 0; }

    IOObj* ret = readOmf( sfio.istrm(), dirnm, dirptr, reqobjnr, incl_old_tmp );
    if ( !ret )
	sfio.closeFail();
    else
    {
	if ( dirptr )
	    dirptr->readtime_ = Time::getFileTimeInSeconds();
	sfio.closeSuccess();
    }
    return ret;
}


void IODir::setDirNameFor( IOObj& ioobj, const char* dirnm )
{
    if ( ioobj.isSubdir() )
	ioobj.dirnm_ = dirnm;
    else
    {
	FilePath fp( dirnm );
	ioobj.setDirName( fp.fileName() );
    }
}


IOObj* IODir::readOmf( od_istream& strm, const char* dirnm,
			IODir* dirptr, int reqobjnr, bool inc_old_tmps )
{
    ascistream astream( strm );
    astream.next();
    FileMultiString fms( astream.value() );
    DirID dirid = DirID::get( toInt(fms[0]) );
    if ( dirptr )
    {
	const_cast<DirID&>(dirptr->dirid_) = dirid;
	const ObjNrType newnr = fms.getIValue( 1 );
	dirptr->curnr_ = newnr > 0 && !mIsUdf(newnr) ? newnr : 0;
	if ( IOObj::isTmpObjNr(dirptr->curnr_) )
	    dirptr->curnr_ = 1;
    }
    astream.next();

    IOObj* retobj = 0;
    while ( astream.type() != ascistream::EndOfFile )
    {
	IOObj* obj = IOObj::get( astream, dirnm, dirid, !inc_old_tmps );
	if ( !obj || obj->isBad() )
	    { delete obj; continue; }

	DBKey ky( obj->key() );
	int objnr = ky.objNr();
	if ( objnr < 0 )
	    objnr = ky.groupNr();

	if ( dirptr )
	{
	    retobj = obj;
	    if ( objnr == 1 )
		dirptr->setName( obj->name() );
	    dirptr->doAddObj( obj, false );
	    if ( IOObj::isTmpObjNr(objnr) )
	    {
		if ( objnr > dirptr->curtmpnr_ )
		    dirptr->curtmpnr_ = objnr;
	    }
	    else
	    {
		if ( objnr > dirptr->curnr_ )
		    dirptr->curnr_ = objnr;
	    }
	}
	else
	{
	    if ( objnr != reqobjnr )
		delete obj;
	    else
		{ retobj = obj; break; }
	}
    }

    if ( retobj )
	setDirNameFor( *retobj, dirnm );
    return retobj;
}


IOObj* IODir::getObj( const DBKey& ky, uiString& errmsg )
{
    Threads::Locker locker( static_read_lock );

    BufferString dirnm( IOM().rootDir() );
    if ( dirnm.isEmpty() || !ky.hasValidGroupID() )
	return 0;

    IOObj* ioobj = doRead( dirnm, 0, errmsg, ky.dirID().getI() );
    if ( !ioobj || !ioobj->isSubdir() || !ky.hasValidObjID() )
	return ioobj;

    dirnm = ioobj->dirName();
    delete ioobj;

    return doRead( dirnm, 0, errmsg, ky.objID().getI() );
}


IOObj* IODir::getEntryByName( const char* nm, const char* trgrpnm ) const
{
    mLock4Read();
    const IOObj* obj = gtObjByName( nm, trgrpnm );
    return obj ? obj->clone() : 0;
}


const IOObj* IODir::gtObjByName( const char* nm, const char* trgrpnm ) const
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


IODir::size_type IODir::indexOf( const DBKey& ky ) const
{
    mLock4Read();
    return gtIdxOf( ky );
}


IODir::size_type IODir::gtIdxOf( const DBKey& ky ) const
{
    for ( size_type idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* ioobj = objs_[idx];
	if ( ioobj->key() == ky )
	    return idx;
    }

    return -1;
}


bool IODir::isPresent( const DBKey& ky ) const
{
    return indexOf( ky ) >= 0;
}


const IOObj* IODir::gtObj( const DBKey& ky ) const
{
    const size_type idxof = gtIdxOf( ky );
    return idxof < 0 ? 0 : objs_[idxof]->clone();
}


bool IODir::doReRead( bool force )
{
    if ( !force && !gtIsOutdated() )
	return false;

    IODir rdiodir( dirname_ );
    if ( !rdiodir.isBad() && rdiodir.size() > 1 )
    {
	copyAll( rdiodir );
	return true;
    }

    return false;
}


bool IODir::permRemove( const DBKey& ky )
{
    mLock4Write();

    if ( doReRead(false) && mIsBad() )
	return false;

    size_type sz = objs_.size();
    for ( size_type idx=0; idx<sz; idx++ )
    {
	IOObj* obj = objs_[idx];
	if ( obj->key() == ky )
	{
	    objs_ -= obj;
	    delete obj;
	    break;
	}
    }

    return doWrite();
}


bool IODir::commitChanges( const IOObj* inpioobj )
{
    if ( !inpioobj )
	return true;

    mLock4Write();

    if ( inpioobj->isSubdir() )
    {
	IOObj* obj = const_cast<IOObj*>( gtObj(inpioobj->key()) );
	if ( obj != inpioobj )
	    obj->copyFrom( *inpioobj );
	return doWrite();
    }

    IOObj* ioobj = inpioobj->clone();
    if ( !ioobj )
	return false;
    if ( !doReRead(false) && mIsBad() )
	{ delete ioobj; return false; }

    const size_type sz = objs_.size();
    bool found = false;
    for ( size_type idx=0; idx<sz; idx++ )
    {
	IOObj* obj = objs_[idx];
	if ( obj->key() == ioobj->key() )
	{
	    delete objs_.replace( idx, ioobj );
	    found = true;
	    break;
	}
    }

    if ( !found )
	objs_ += ioobj;

    return doWrite();
}


bool IODir::addObj( IOObj* ioobj, bool persist )
{
    mLock4Write();
    return doAddObj( ioobj, persist );
}


bool IODir::doAddObj( IOObj* ioobj, bool persist )
{
    if ( persist )
    {
	if ( !doReRead() && mIsBad() )
	    return false;
    }

    if ( !ioobj->key().isValid() || objs_[ioobj] || gtIdxOf(ioobj->key())>0 )
	ioobj->setKey( gtNewKey(curnr_) );

    doEnsureUniqueName( *ioobj );

    objs_ += ioobj;
    setDirNameFor( *ioobj, dirName() );

    return persist ? doWrite() : true;
}


bool IODir::ensureUniqueName( IOObj& ioobj ) const
{
    mLock4Read();
    return doEnsureUniqueName( ioobj );
}


bool IODir::doEnsureUniqueName( IOObj& ioobj ) const
{
    BufferString nm( ioobj.name() );

    size_type nr = 1;
    while ( gtObjByName(nm.buf(),ioobj.translator().buf()) )
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


bool IODir::wrOmf( od_ostream& strm ) const
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

    // First the main obj
    const IOObj* mymain = main();
    if ( mymain && !mymain->put(astream) )
	mErrRet()

    // Then the subdirs
    for ( size_type idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* obj = objs_[idx];
	if ( obj == mymain ) continue;
	if ( obj->isSubdir() && !obj->put(astream) )
	    mErrRet()
    }
    // Then the normal objs
    for ( size_type idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* obj = objs_[idx];
	if ( obj == mymain ) continue;
	if ( !obj->isSubdir() && !obj->put(astream) )
	    mErrRet()
    }

    return true;
}


#undef mErrRet
#define mErrRet() \
{ \
    errmsg_ = sfio.errMsg(); \
    return false; \
}

bool IODir::doWrite() const
{
    SafeFileIO sfio( omfFileName(dirname_), false );
    if ( !sfio.open(false) )
	mErrRet()

    if ( !wrOmf(sfio.ostrm()) )
	return false;

    if ( !sfio.closeSuccess() )
	mErrRet()

    return true;
}


DBKey IODir::newKey() const
{
    mLock4Write();
    return gtNewKey( curnr_ );
}


DBKey IODir::newTmpKey() const
{
    mLock4Write();
    return gtNewKey( curtmpnr_ );
}


DBKey IODir::gtNewKey( const ObjNrType& id ) const
{
    const_cast<ObjNrType&>(id)++;
    return DBKey( dirid_, DBKey::ObjID::get(id) );
}


DBKey IODir::getNewTmpKey( const IOObjContext& ctxt )
{
    const IODir iodir( ctxt.getSelDirID() );
    return iodir.newTmpKey();
}


void IODir::getTmpIOObjs( DirID dirid, ObjectSet<IOObj>& ioobjs,
			  const IOObjSelConstraints* cnstrnts )
{
    IODir iodir;
    iodir.init( dirid, true );
    for ( int idx=0; idx<iodir.objs_.size(); idx++ )
    {
	const IOObj& ioobj = *iodir.objs_[idx];
	if ( !ioobj.isTmp() )
	    continue;

	if ( !cnstrnts || cnstrnts->isGood(ioobj) )
	    ioobjs += ioobj.clone();
    }
}


IODirIter::IODirIter( const IODir& iodir )
    : MonitorableIter<IODir::size_type>(iodir,-1)
{
}


IODirIter::IODirIter( const IODirIter& oth )
    : MonitorableIter<IODir::size_type>(oth)
{
}


const IODir& IODirIter::ioDir() const
{
    return static_cast<const IODir&>( monitored() );
}


IODirIter& IODirIter::operator =( const IODirIter& oth )
{
    pErrMsg( "No assignment" );
    return *this;
}


const IOObj& IODirIter::ioObj() const
{
    return isValid() ? *ioDir().objs_[curidx_] : IOObj::getInvalid();
}


DBKey IODirIter::key() const
{
    return isValid() ? ioDir().objs_[curidx_]->key() : DBKey::getInvalid();
}
