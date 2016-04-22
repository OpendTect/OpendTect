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
#include "ctxtioobj.h"
#include "uistrings.h"


static Threads::Lock static_read_lock;

#define mInitVarList \
	  isok_(false) \
	, curid_(0) \
	, curtmpid_(IOObj::tmpLeafIDStart())

IODir::IODir( const char* dirnm )
	: dirname_(dirnm)
	, key_("")
	, mInitVarList
{
    if ( build() )
	isok_ = true;
}


IODir::IODir()
	: dirname_("")
	, key_("")
	, mInitVarList
{
}


IODir::IODir( const IODir& oth )
	: dirname_(oth.dirname_)
	, key_(oth.key_)
{
    *this = oth;
}


IODir::IODir( const MultiID& ky )
	: dirname_("")
	, key_("")
	, mInitVarList
{
    IOObj* ioobj = getObj( ky, errmsg_ );
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

    if ( build() )
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
	const_cast<MultiID&>(key_).set( oth.key_ );
	isok_ = oth.isok_;
	deepCopyClone( objs_, oth.objs_ );
	curid_ = oth.curid_;
	curtmpid_ = oth.curtmpid_;
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


const IOObj* IODir::get( size_type idx ) const
{
    Threads::Locker locker( lock_ );
    return objs_[idx];
}


const IOObj* IODir::get( const MultiID& ky ) const
{
    Threads::Locker locker( lock_ );
    return doGet( ky );
}


bool IODir::build()
{
    Threads::Locker locker( static_read_lock );
    return doRead( dirname_, this, errmsg_ );
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
	if ( ioobj->leafID() == 1 )
	    return ioobj;
    }
    return 0;
}


IOObj* IODir::doRead( const char* dirnm, IODir* dirptr, uiString& errmsg,
		      SubID reqsubid )
{
    SafeFileIO sfio( FilePath(dirnm,".omf").fullPath(), false );
    if ( !sfio.open(true) )
    {
	errmsg = sfio.errMsg();
	return 0;
    }

    IOObj* ret = readOmf( sfio.istrm(), dirnm, dirptr, reqsubid );
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
			IODir* dirptr, SubID reqsubid )
{
    ascistream astream( strm );
    astream.next();
    FileMultiString fms( astream.value() );
    MultiID dirky( fms[0] );
    if ( dirky == "0" )
	dirky = "";
    if ( dirptr )
    {
	const_cast<MultiID&>(dirptr->key_) = dirky;
	const SubID newid = fms.getIValue( 1 );
	dirptr->curid_ = mIsUdf(newid) ? 0 : newid;
	if ( IOObj::isTmpLeafID(dirptr->curid_) )
	    dirptr->curid_ = 1;
    }
    astream.next();

    IOObj* retobj = 0;
    while ( astream.type() != ascistream::EndOfFile )
    {
	IOObj* obj = IOObj::get(astream,dirnm,dirky);
	if ( !obj || obj->isBad() )
	    { delete obj; continue; }

	MultiID ky( obj->key() );
	const SubID subid = ky.leafID();

	if ( dirptr )
	{
	    retobj = obj;
	    if ( subid == 1 )
		dirptr->setName( obj->name() );
	    dirptr->doAddObj( obj, false );
	    if ( IOObj::isTmpLeafID(subid) )
	    {
		if ( subid > dirptr->curtmpid_ )
		    dirptr->curtmpid_ = subid;
	    }
	    else
	    {
		if ( subid > dirptr->curid_ )
		    dirptr->curid_ = subid;
	    }
	}
	else
	{
	    if ( subid != reqsubid )
		delete obj;
	    else
		{ retobj = obj; break; }
	}
    }

    if ( retobj )
	setDirName( *retobj, dirnm );
    return retobj;
}


IOObj* IODir::getObj( const MultiID& ky, uiString& errmsg )
{
    Threads::Locker locker( static_read_lock );

    BufferString dirnm( IOM().rootDir() );
    if ( dirnm.isEmpty() )
	return 0;

    size_type nrkeys = ky.nrKeys();
    for ( size_type idx=0; idx<nrkeys; idx++ )
    {
	const SubID id = ky.subID( idx );
	IOObj* ioobj = doRead( dirnm, 0, errmsg, id );
	if ( !ioobj || idx == nrkeys-1 || !ioobj->isSubdir() )
	    return ioobj;

	dirnm = ioobj->dirName();
	delete ioobj;
    }

    return 0;
}


const IOObj* IODir::get( const char* nm, const char* trgrpnm ) const
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


IODir::size_type IODir::indexOf( const MultiID& ky ) const
{
    Threads::Locker locker( lock_ );
    return gtIdxOf( ky );
}


IODir::size_type IODir::gtIdxOf( const MultiID& ky ) const
{
    for ( size_type idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* ioobj = objs_[idx];
	if ( ioobj->key() == ky )
	    return idx;
    }

    return -1;
}


bool IODir::isPresent( const MultiID& ky ) const
{
    return indexOf( ky ) >= 0;
}


const IOObj* IODir::doGet( const MultiID& ky ) const
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
	curid_ = rdiodir.curid_;
	curtmpid_ = rdiodir.curtmpid_;
	isok_ = true;
    }
}


bool IODir::permRemove( const MultiID& ky )
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

    if ( ioobj->key().isEmpty() || objs_[ioobj] || gtIdxOf(ioobj->key())>0 )
	ioobj->setKey( gtNewKey(curid_) );

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
    FileMultiString fms( key_.isEmpty() ? "0" : key_.str() );
    for ( size_type idx=0; idx<objs_.size(); idx++ )
    {
	const SubID leafid = objs_[idx]->key().leafID();
	if ( !IOObj::isTmpLeafID(leafid) && leafid > curid_ )
	    curid_ = leafid;
    }
    fms += curid_;
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


MultiID IODir::newKey() const
{
    Threads::Locker locker( lock_ );
    return gtNewKey( curid_ );
}


MultiID IODir::newTmpKey() const
{
    Threads::Locker locker( lock_ );
    return gtNewKey( curtmpid_ );
}


MultiID IODir::gtNewKey( const size_type& id ) const
{
    MultiID ret = key_;
    const_cast<int&>(id)++;
    ret.add( id );
    return ret;
}


MultiID IODir::getNewTmpKey( const IOObjContext& ctxt )
{
    const IODir iodir( MultiID(ctxt.getSelKey()) );
    return iodir.newTmpKey();
}
