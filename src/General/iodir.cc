/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 2-8-1994
-*/


#include "iodir.h"

#include "ascstream.h"
#include "file.h"
#include "filepath.h"
#include "hiddenparam.h"
#include "ioman.h"
#include "iosubdir.h"
#include "safefileio.h"
#include "separstr.h"
#include "surveydisklocation.h"

IODir::IODir( const char* dirnm )
    : dirname_(dirnm)
{
    if ( build() )
	isok_ = true;
}


IODir::IODir()
{
}


IODir::IODir( const MultiID& ky )
{
    MultiID dirid = ky;
    if ( dirid.groupID()>100000 && dirid.objectID()<0 )
	dirid.setGroupID(-1).setObjectID(ky.groupID());

    IOObj* ioobj = getObj( dirid );
    if ( !ioobj )
	return;

    dirname_ = ioobj->dirName();
    FilePath fp( dirname_ );
    if ( !fp.isAbsolute() )
    {
	fp.set( IOM().rootDir() ).add( dirname_ );
	dirname_ = fp.fullPath();
    }
    delete ioobj;

    if ( build() )
	isok_ = true;
}


IODir::~IODir()
{
    deepErase( objs_ );
}


bool IODir::build()
{
    if ( !doRead(dirname_,this) )
	return false;

    lastmodtime_ =
	File::getTimeInSeconds( FilePath(dirname_,".omf").fullPath() );
    return true;
}


IOObj* IODir::getMain( const char* dirnm )
{
    return doRead( dirnm, 0, 1 );
}


const IOObj* IODir::main() const
{
    for ( int idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* ioobj = objs_[idx];
	if ( ioobj->myKey() == 1 ) return ioobj;
    }
    return 0;
}


IOObj* IODir::doRead( const char* dirnm, IODir* dirptr, int needid )
{
    SafeFileIO sfio( FilePath(dirnm,".omf").fullPath(), false );
    if ( !sfio.open(true) )
    {
	BufferString msg( "\nError during read of Object Management info!" );
	msg += "\n-> Please check directory (read permissions, existence):\n'";
	msg += dirnm; msg += "'";
	ErrMsg( msg );
	return nullptr;
    }

    IOObj* ret = readOmf( sfio.istrm(), dirnm, dirptr, needid );
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


static Threads::Lock lock_;

IOObj* IODir::readOmf( od_istream& strm, const char* dirnm,
			IODir* dirptr, int needid )
{
    Threads::Locker locker( lock_ );

    ascistream astream( strm );
    astream.next();
    FileMultiString fms( astream.value() );
    const MultiID dirky( fms[0].buf() );
    if ( dirptr )
    {
	dirptr->key_ = dirky;
	const int newid = fms.getIValue( 1 );
	dirptr->curid_ = mIsUdf(newid) ? 0 : newid;
	if ( dirptr->curid_ == IOObj::tmpID() )
	    dirptr->curid_ = 1;
    }
    astream.next();

    IOObj* retobj = nullptr;
    while ( astream.type() != ascistream::EndOfFile )
    {
	IOObj* obj = IOObj::get(astream,dirnm,dirky.toString());
	if ( !obj || obj->isBad() ) { delete obj; continue; }

	MultiID ky( obj->key() );
	const int id = ky.objectID();

	if ( dirptr )
	{
	    retobj = obj;
	    if ( id == 1 )
		dirptr->setName( obj->name() );

	    dirptr->addObjNoChecks( obj );
	    if ( id < 99999 && id > dirptr->curid_ )
		dirptr->curid_ = id;
	}
	else
	{
	    if ( id != needid )
		delete obj;
	    else
		{ retobj = obj; break; }
	}
    }

    if ( retobj )
	setDirName( *retobj, dirnm );
    return retobj;
}



IOObj* IODir::getIOObj( const char* _dirnm, const MultiID& ky )
{
    BufferString dirnm( _dirnm );
    if ( ky.isUdf() || dirnm.isEmpty() || !File::isDirectory(dirnm) )
	return nullptr;

    if ( ky.groupID()<0 && ky.objectID()>100000 )
    {
	IOObj* ioobj = doRead( dirnm, nullptr, ky.objectID() );
	return ioobj;
    }

    const int nrkeys = ky.nrIDs();
    for ( int idx=0; idx<nrkeys; idx++ )
    {
	const int id = ky.ID( idx );
	IOObj* ioobj = doRead( dirnm, 0, id );
	if ( !ioobj || !ioobj->isSubdir() )
	    return ioobj;

	dirnm = ioobj->dirName();
	delete ioobj;
    }

    return nullptr;
}


IOObj* IODir::getObj( const DBKey& ky )
{
    if ( !ky.hasSurveyLocation() )
	return getObj( sCast(const MultiID&,ky) );

    return getIOObj( ky.surveyDiskLocation().fullPath(), ky );
}


IOObj* IODir::getObj( const MultiID& ky )
{
    BufferString dirnm( IOM().rootDir() );
    return getIOObj( dirnm.buf(), ky );
}


const IOObj* IODir::get( const char* ky, const char* trgrpnm ) const
{
    for ( int idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* ioobj = objs_[idx];
	if ( ioobj->name() == ky )
	{
	    if ( !trgrpnm || ioobj->group() == trgrpnm )
		return ioobj;
	}
    }
    return 0;
}


int IODir::indexOf( const MultiID& ky ) const
{
    for ( int idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* ioobj = objs_[idx];
	if ( ioobj->key().objectID() == ky.objectID() )
	    return idx;
    }

    return -1;
}


bool IODir::isPresent( const MultiID& ky ) const
{
    return indexOf( ky ) >= 0;
}


IOObj* IODir::get( const MultiID& ky )
{
    const int idxof = indexOf( ky );
    return idxof < 0 ? 0 : objs_[idxof];
}


const IOObj* IODir::get( const MultiID& ky ) const
{
    const int idxof = indexOf( ky );
    return idxof < 0 ? 0 : objs_[idxof];
}


void IODir::update()
{
    const od_int64 curmodtime =
		File::getTimeInSeconds( FilePath(dirname_,".omf").fullPath() );
    if ( curmodtime > lastmodtime_ )
	reRead();
}


void IODir::reRead()
{
    IODir rdiodir( dirname_ );
    if ( rdiodir.isok_ && rdiodir.main() && rdiodir.size() > 1 )
    {
	deepErase( objs_ );
	objs_ = rdiodir.objs_;
	rdiodir.objs_.erase();
	curid_ = rdiodir.curid_;
	isok_ = true;
	lastmodtime_ =
	    File::getTimeInSeconds(FilePath(dirname_,".omf").fullPath() );
    }
}


bool IODir::permRemove( const MultiID& ky )
{
    update();
    if ( isBad() ) return false;

    int sz = objs_.size();
    for ( int idx=0; idx<sz; idx++ )
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
    if ( ioobj->isSubdir() )
    {
	IOObj* obj = get( ioobj->key() );
	if ( obj != ioobj )
	    obj->copyFrom( ioobj );
	return doWrite();
    }

    IOObj* clone = ioobj->clone();
    if ( !clone ) return false;
    update();
    if ( isBad() ) { delete clone; return false; }

    int sz = objs_.size();
    bool found = false;
    for ( int idx=0; idx<sz; idx++ )
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


void IODir::addObjNoChecks( IOObj* ioobj )
{
    objs_ += ioobj;
    setDirName( *ioobj, dirName() );
}


bool IODir::addObj( IOObj* ioobj, bool persist )
{
    if ( persist )
    {
	update();
	if ( isBad() ) return false;
    }

    if ( ioobj->key().isUdf() || objs_[ioobj] || isPresent(ioobj->key()) )
	ioobj->setKey( newKey() );

    ensureUniqueName( *ioobj );
    objs_ += ioobj;
    setDirName( *ioobj, dirName() );

    return persist ? doWrite() : true;
}


bool IODir::ensureUniqueName( IOObj& ioobj )
{
    BufferString nm( ioobj.name() );

    int nr = 1;
    while ( get(nm.buf(),ioobj.group().buf()) )
    {
	nr++;
	nm.set( ioobj.name() ).add( " (" ).add( nr ).add( ")" );
    }
    if ( nr == 1 )
	return false;

    ioobj.setName( nm );
    return true;
}


#define mAddDirWarn(msg) \
    msg += "\n-> Please check write permissions for directory:\n   "; \
    msg += dirname_

#define mErrRet() \
{ \
    BufferString msg( "\nError during write of Object Management info!" ); \
    mAddDirWarn(msg); \
    ErrMsg( msg ); \
    return false; \
}


bool IODir::wrOmf( od_ostream& strm ) const
{
    ascostream astream( strm );
    if ( !astream.putHeader( "Object Management file" ) )
	mErrRet()
    FileMultiString fms( key_.isUdf() ? "0" : key_.toString() );
    for ( int idx=0; idx<objs_.size(); idx++ )
    {
	const MultiID currentkey = objs_[idx]->key();
	int curleafid = currentkey.objectID();
	if ( curleafid != IOObj::tmpID() && curleafid < 99999
	  && curleafid > curid_ )
	    curid_ = curleafid;
    }
    fms += curid_;
    astream.put( "ID", fms );
    astream.newParagraph();

    // First the main obj
    const IOObj* mymain = main();
    if ( mymain && !mymain->put(astream) )
	mErrRet()

    // Then the subdirs
    for ( int idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* obj = objs_[idx];
	if ( obj == mymain ) continue;
	if ( obj->isSubdir() && !obj->put(astream) )
	    mErrRet()
    }
    // Then the normal objs
    for ( int idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* obj = objs_[idx];
	if ( obj == mymain ) continue;
	if ( !obj->isSubdir() && !obj->put(astream) )
	    mErrRet()
    }

    return true;
}


#undef mErrRet
#define mErrRet(addsfiomsg) \
{ \
    BufferString msg( "\nError during write of Object Management info!" ); \
    mAddDirWarn(msg); \
    if ( addsfiomsg ) \
	{ msg += "\n"; msg += sfio.errMsg(); } \
    ErrMsg( msg ); \
    return false; \
}

bool IODir::doWrite() const
{
    const BufferString filenm = FilePath( dirname_, ".omf" ).fullPath();
    SafeFileIO sfio( filenm, false );
    if ( !sfio.open(false) )
	mErrRet(true)

    if ( !wrOmf(sfio.ostrm()) )
    {
	sfio.closeFail();
	mErrRet(false)
    }

    if ( !sfio.closeSuccess() )
	mErrRet(true)

    lastmodtime_ = File::getTimeInSeconds( filenm );
    return true;
}


MultiID IODir::getNewKey() const
{
    return newKey();
}


MultiID IODir::newKey() const
{
    MultiID id = key_;
    const_cast<IODir*>(this)->curid_++;
    id.setObjectID( curid_ );
    return id;
}


bool IODir::hasObjectsWithGroup( const char* trgrpnm ) const
{
    for ( int idx=0; idx<objs_.size(); idx++ )
    {
	if ( objs_[idx]->group() == trgrpnm )
	    return true;
    }

    return false;
}
