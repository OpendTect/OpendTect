/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "iodir.h"

#include "ascstream.h"
#include "dbdir.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "oddskey.h"
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
    IOObj* ioobj = getObj( ky );
    if ( !ioobj )
	return;

    dirname_ = ioobj->dirName();
    FilePath fp( dirname_ );
    if ( !fp.isAbsolute() )
    {
	fp.set( IOM().rootDir().fullPath() ).add( dirname_ );
	dirname_ = fp.fullPath();
    }

    delete ioobj;
    if ( build() )
	isok_ = true;
}


IODir::~IODir()
{
    objidmap_.clear();
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
    return doRead( dirnm, nullptr, 1 );
}


const IOObj* IODir::main() const
{
    for ( int idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* ioobj = objs_[idx];
	if ( ioobj->myKey() == 1 )
	    return ioobj;
    }

    return nullptr;
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
	const FilePath fp( dirnm );
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
    const MultiID dirky( fms.getIValue(0), 0 );
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
	IOObj* obj = IOObj::get( astream, dirnm, dirky.groupID() );
	if ( !obj || obj->isBad() )
	{
	    delete obj;
	    continue;
	}

	const int id = obj->myKey();
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

    IOObj* subdirobj = doRead( dirnm, nullptr, ky.groupID() );
    if ( !subdirobj || ky.objectID()==0 )
	return subdirobj;

    dirnm = subdirobj->dirName();
    delete subdirobj;
    return doRead( dirnm, nullptr, ky.objectID() );
}


IOObj* IODir::getIOObj( const char* _dirnm, const OD::DataSetKey& dsky )
{
    BufferString dirnm( _dirnm );
    if ( dsky.isUdf() || dirnm.isEmpty() || !File::isDirectory(dirnm) )
	return nullptr;

    for ( int idx=0; idx<IOObjContext::totalNrStdDirs(); idx++ )
    {
	const IOObjContext::StdSelType seltyp =
				    IOObjContext::StdSelType( idx );
	const IOObjContext::StdDirData* dirdata =
					IOObjContext::getStdDirData( seltyp );
	if ( !dirdata )
	    continue;

	const IODir dir( dirdata->id_ );
	const IOObj* ioobj = dir.get( dsky );
	if ( ioobj )
	    return ioobj->clone();
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
    BufferString dirnm( IOM().rootDir().fullPath() );
    return getIOObj( dirnm.buf(), ky );
}


IOObj* IODir::getObj( const OD::DataSetKey& dsky )
{
    BufferString dirnm( IOM().rootDir().fullPath() );
    return getIOObj( dirnm.buf(), dsky );
}


const IOObj* IODir::get( const OD::DataSetKey& dsky ) const
{
    for ( const auto* ioobj : objs_ )
    {
	if ( ioobj->hasDSKey() && *ioobj->DSKey() == dsky )
	    return ioobj;
    }

    return nullptr;
}


const IOObj* IODir::get( const char* objnm, const char* trgrpnm ) const
{
    for ( const auto* ioobj : objs_ )
    {
	if ( ioobj->name() == objnm )
	{
	    if ( !trgrpnm || ioobj->group() == trgrpnm )
		return ioobj;
	}
    }

    return nullptr;
}


int IODir::indexOf( const MultiID& ky ) const
{
    auto it = objidmap_.find( ky.objectID() );
    if ( it != objidmap_.end() )
	return it->second;

    return -1;
}


bool IODir::isPresent( const MultiID& ky ) const
{
    return indexOf( ky ) >= 0;
}


IOObj* IODir::get( const MultiID& ky )
{
    const int idxof = indexOf( ky );
    return objs_.validIdx(idxof) ? objs_[idxof] : nullptr;
}


const IOObj* IODir::get( const MultiID& ky ) const
{
    return getNonConst(*this).get( ky );
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
    IODir rdiodir( dirname_.buf() );
    if ( rdiodir.isok_ && rdiodir.main() && rdiodir.size() > 1 )
    {
	deepErase( objs_ );
	objidmap_.clear();
	objs_ = rdiodir.objs_;
	objidmap_ = rdiodir.objidmap_;
	rdiodir.objs_.erase();
	curid_ = rdiodir.curid_;
	isok_ = true;
	lastmodtime_ =
	    File::getTimeInSeconds(FilePath(dirname_,".omf").fullPath() );
    }
}


void IODir::rebuild_objidmap()
{
    objidmap_.clear();
    for (int idx=0; idx<objs_.size(); idx++ )
    {
	const IOObj* ioobj = objs_[idx];
	objidmap_[ioobj->key_.objectID()] = idx;
    }
}


bool IODir::permRemove( const MultiID& ky )
{
    update();
    if ( isBad() )
	return false;

    const int idx = indexOf( ky );
    if ( !objs_.validIdx(idx) )
	return false;

    delete objs_.removeSingle( idx );
    rebuild_objidmap();
    return doWrite();
}


bool IODir::permRemove( const TypeSet<MultiID>& keys )
{
    if ( keys.isEmpty() )
	return false;

    update();
    if ( isBad() )
	return false;

    const int sz = objs_.size();
    for ( int idx=sz-1; idx>=0; idx-- )
    {
	const MultiID ky = objs_[idx]->key();
	if ( keys.isPresent(ky) )
	    delete objs_.removeSingle( idx );
    }

    rebuild_objidmap();
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
    if ( !clone )
	return false;

    update();
    if ( isBad() )
    {
	delete clone;
	return false;
    }

    const int idx = indexOf( clone->key() );
    if ( objs_.validIdx(idx) )
	delete objs_.replace( idx, clone );
    else
    {
	objs_ += clone;
	objidmap_[ioobj->key_.objectID()] = objs_.size()-1;
    }

    return doWrite();
}


bool IODir::commitChanges( const ObjectSet<const IOObj>& objs )
{
    if ( objs.isEmpty() )
	return false;

    update();
    if ( isBad() )
	return false;

    for ( const auto* obj : objs )
    {
	PtrMan<IOObj> clone = obj->clone();
	if ( !clone )
	    return false;

	int sz = objs_.size();
	bool found = false;
	for ( int idx=0; idx<sz; idx++ )
	{
	    IOObj* ioobj = objs_[idx];
	    if ( ioobj->key() == clone->key() )
	    {
		delete objs_.replace( idx, clone.release() );
		found = true;
		break;
	    }
	}

	if ( !found )
	    objs_ += clone.release();
    }

    return doWrite();
}


void IODir::addObjNoChecks( IOObj* ioobj )
{
    objs_ += ioobj;
    objidmap_[ioobj->key_.objectID()] = objs_.size()-1;
    setDirName( *ioobj, dirName() );
}


void IODir::addObjectNoWrite( IOObj* ioobj )
{
    if ( ioobj->key().isUdf() || objs_[ioobj] || isPresent(ioobj->key()) )
	ioobj->setKey( newKey() );

    ensureUniqueName( *ioobj );
    objs_ += ioobj;
    objidmap_[ioobj->key_.objectID()] = objs_.size()-1;
    setDirName( *ioobj, dirName() );
}


bool IODir::addObj( IOObj* ioobj, bool persist )
{
    if ( !ioobj )
	return false;

    if ( persist )
    {
	update();
	if ( isBad() )
	    return false;
    }

    addObjectNoWrite( ioobj );
    return persist ? doWrite() : true;
}


bool IODir::addObjects( ObjectSet<IOObj>& objs, bool persist )
{
    if ( objs.isEmpty() )
	return false;

    if ( persist )
    {
	update();
	if ( isBad() )
	    return false;
    }

    for ( auto* obj : objs )
	addObjectNoWrite( obj );

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
    FileMultiString fms( key_.isUdf() ? "0" : toString(key_.groupID()) );
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


// DBDir

DBDir::DBDir( const char* dirnm )
    : IODir(dirnm)
{}


DBDir::DBDir( const DBKey& dbkey )
    : IODir(dbkey)
{}


DBDir::~DBDir()
{}
