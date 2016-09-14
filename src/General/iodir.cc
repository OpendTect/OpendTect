/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 2-8-1994
-*/


#include "iodir.h"

#include "ascstream.h"
#include "filepath.h"
#include "ioman.h"
#include "iosubdir.h"
#include "safefileio.h"
#include "separstr.h"
#include "uistrings.h"


static Threads::Lock static_read_lock;

#define mInitVarList \
	  isok_(false) \
	, curnr_(0) \
	, curtmpnr_(IOObj::tmpObjNrStart())

IODir::IODir( const char* dirnm )
	: dirname_(dirnm)
	, mInitVarList
{
    if ( build(false) )
	isok_ = true;
}


IODir::IODir()
	: dirname_("")
	, mInitVarList
{
}


IODir::IODir( const IODir& oth )
	: dirname_(oth.dirname_)
	, dirid_(oth.dirid_)
{
    *this = oth;
}


IODir::IODir( DirID dirid )
	: dirname_("")
	, mInitVarList
{
    init( dirid, false );
}


IODir::IODir( IOObjContext::StdSelType seltyp )
	: dirname_("")
	, dirid_(IOObjContext::getStdDirData(seltyp)->id_)
	, mInitVarList
{
    init( dirid_, false );
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

    if ( build(inc_old_tmps) )
	isok_ = true;
}


IODir::~IODir()
{
    deepErase( objs_ );
}


IODir& IODir::operator =( const IODir& oth )
{
    if ( this != &oth )
    {
	Threads::Locker mylocker( oth.lock_ );
	Threads::Locker othlocker( oth.lock_ );
	const_cast<BufferString&>(dirname_).set( oth.dirname_ );
	const_cast<DirID&>(dirid_) = oth.dirid_;
	isok_ = oth.isok_;
	deepCopyClone( objs_, oth.objs_ );
	curnr_ = oth.curnr_;
	curtmpnr_ = oth.curtmpnr_;
	errmsg_ = oth.errmsg_;
    }
    return *this;
}


void IODir::reRead()
{
    Threads::Locker locker( lock_ );
    doReRead();
}


bool IODir::isBad() const
{
    Threads::Locker locker( lock_ );
    return !isok_;
}


IODir::size_type IODir::size() const
{
    Threads::Locker locker( lock_ );
    return objs_.size();
}


const IOObj* IODir::getByIdx( size_type idx ) const
{
    Threads::Locker locker( lock_ );
    return objs_[idx];
}


const IOObj* IODir::get( const DBKey& ky ) const
{
    Threads::Locker locker( lock_ );
    return doGet( ky );
}


bool IODir::build( bool inc_old_tmps )
{
    Threads::Locker locker( static_read_lock );
    return doRead( dirname_, this, errmsg_, DirID::getInvalid().getI(),
		    inc_old_tmps );
}


IOObj* IODir::getMain( const char* dirnm, uiString& errmsg )
{
    Threads::Locker locker( static_read_lock );
    return doRead( dirnm, 0, errmsg, 1 );
}

bool IODir::writeNow() const
{
    Threads::Locker locker( lock_ );
    return doWrite();
}


const IOObj* IODir::main() const
{
    Threads::Locker locker( lock_ );
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
    SafeFileIO sfio( FilePath(dirnm,".omf").fullPath(), false );
    if ( !sfio.open(true) )
    {
	errmsg = sfio.errMsg();
	return 0;
    }

    IOObj* ret = readOmf( sfio.istrm(), dirnm, dirptr, reqobjnr, incl_old_tmp );
    if ( ret )
	sfio.closeSuccess();
    else
	sfio.closeFail();
    return ret;
}


void IODir::setDirName( IOObj& ioobj, const char* dirnm )
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
	IOObj* obj = IOObj::get( astream, dirnm, dirid.getI(), !inc_old_tmps );
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
	setDirName( *retobj, dirnm );
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


const IOObj* IODir::getByName( const char* nm, const char* trgrpnm ) const
{
    Threads::Locker locker( lock_ );
    return doGet( nm, trgrpnm );
}


const IOObj* IODir::doGet( const char* nm, const char* trgrpnm ) const
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
    Threads::Locker locker( lock_ );
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


const IOObj* IODir::doGet( const DBKey& ky ) const
{
    const size_type idxof = gtIdxOf( ky );
    return idxof < 0 ? 0 : objs_[idxof];
}


void IODir::doReRead()
{
    IODir rdiodir( dirname_ );
    if ( rdiodir.isok_ && rdiodir.main() && rdiodir.size() > 1 )
    {
	deepErase( objs_ );
	objs_ = rdiodir.objs_;
	rdiodir.objs_.erase();
	curnr_ = rdiodir.curnr_;
	curtmpnr_ = rdiodir.curtmpnr_;
	isok_ = true;
    }
}


bool IODir::permRemove( const DBKey& ky )
{
    Threads::Locker locker( lock_ );
    doReRead();
    if ( !isok_ )
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


bool IODir::commitChanges( const IOObj* ioobj )
{
    if ( !ioobj )
	return true;

    Threads::Locker locker( lock_ );

    if ( ioobj->isSubdir() )
    {
	IOObj* obj = const_cast<IOObj*>( doGet(ioobj->key()) );
	if ( obj != ioobj )
	    obj->copyFrom( ioobj );
	return doWrite();
    }

    IOObj* clone = ioobj->clone();
    if ( !clone )
	return false;
    doReRead();
    if ( !isok_ )
	{ delete clone; return false; }

    const size_type sz = objs_.size();
    bool found = false;
    for ( size_type idx=0; idx<sz; idx++ )
    {
	IOObj* obj = objs_[idx];
	if ( obj->key() == clone->key() )
	{
	    delete objs_.replace( idx, clone );
	    found = true;
	    break;
	}
    }
    if ( !found )
	objs_ += clone;
    return doWrite();
}


bool IODir::addObj( IOObj* ioobj, bool persist )
{
    Threads::Locker locker( lock_ );
    return doAddObj( ioobj, persist );
}


bool IODir::doAddObj( IOObj* ioobj, bool persist )
{
    if ( persist )
    {
	doReRead();
	if ( !isok_ )
	    return false;
    }

    if ( !ioobj->key().isValid() || objs_[ioobj] || gtIdxOf(ioobj->key())>0 )
	ioobj->setKey( gtNewKey(curnr_) );

    doEnsureUniqueName( *ioobj );

    objs_ += ioobj;
    setDirName( *ioobj, dirName() );

    return persist ? doWrite() : true;
}


bool IODir::ensureUniqueName( IOObj& ioobj ) const
{
    Threads::Locker locker( lock_ );
    return doEnsureUniqueName( ioobj );
}


bool IODir::doEnsureUniqueName( IOObj& ioobj ) const
{
    BufferString nm( ioobj.name() );

    size_type nr = 1;
    while ( doGet(nm.buf(),ioobj.translator().buf()) )
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
    SafeFileIO sfio( FilePath(dirname_,".omf").fullPath(), false );
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
    Threads::Locker locker( lock_ );
    return gtNewKey( curnr_ );
}


DBKey IODir::newTmpKey() const
{
    Threads::Locker locker( lock_ );
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
