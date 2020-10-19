/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 1994 / Sep 2016
-*/


#include "dbdir.h"

#include "ascstream.h"
#include "file.h"
#include "filepath.h"
#include "timefun.h"
#include "dbman.h"
#include "iosubdir.h"
#include "safefileio.h"
#include "separstr.h"
#include "uistrings.h"
#include "globexpr.h"
#include "strmprov.h"
#include "transl.h"
#include "surveydisklocation.h"


#define mIsBad() (readtime_ < 0)


static BufferString omfFileName( const char* dirnm )
{
    return BufferString( File::Path(dirnm,".omf").fullPath() );
}


DBDir::DBDir()
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
{
    readFromFile( false );
    mTriggerInstanceCreatedNotifier();
}


DBDir::DBDir( DirID dirid )
{
    fromDirID( dirid, false );
    mTriggerInstanceCreatedNotifier();
}


DBDir::DBDir( IOObjContext::StdSelType seltyp )
    : DBDir(IOObjContext::getStdDirData(seltyp)->id_)
{}


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
    detachAllNotifiers();
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



Monitorable::ChangeType DBDir::compareClassData( const DBDir& oth ) const
{
    if ( readtime_ != oth.readtime_ || dirid_ != oth.dirid_ )
	return cEntireObjectChange();

    const int mysz = objs_.size();
    const int othsz = oth.objs_.size();
    const int nrdiff = othsz - mysz;
    if ( std::abs(nrdiff) > 1 )
	return cEntireObjectChange();

    ObjectSet<IOObj> myobjs( objs_ );
    ObjectSet<IOObj> othobjs( oth.objs_ );
    int chgdidx = -1;
    if ( nrdiff == 1 )
    {
	chgdidx = mysz;
	othobjs.removeSingle( mysz );
    }
    else if ( nrdiff == -1 )
    {
	for ( int idx=0; idx<mysz; idx++ )
	{
	    const IOObj& myobj = *myobjs[idx];
	    const IOObj& othobj = *othobjs[idx];
	    if ( myobj.key().objID() != othobj.key().objID() )
		{ chgdidx = idx; break; }
	}
	if ( chgdidx < 0 )
	    chgdidx = othsz;
	myobjs.removeSingle( chgdidx );
    }

    for ( int idx=0; idx<myobjs.size(); idx++ )
    {
	const IOObj& ioobj = *myobjs[idx];
	const IOObj& othioobj = *othobjs[idx];
	if ( !ioobj.isEqualTo(othioobj) )
	{
	    if ( chgdidx >= 0 )
		return cEntireObjectChange();
	    chgdidx = idx;
	}
    }

    if ( chgdidx < 0 )
	return cNoChange();

    if ( nrdiff == 0 )
	return cEntryChanged();

    return nrdiff > 0 ? cEntryAdded() : cEntryRemoved();
}


bool DBDir::readFromFile( bool incl_old_tmp )
{
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
    mLock4Read();
    return objs_.size();
}


IOObj* DBDir::getEntry( ObjID objid ) const
{
    mLock4Read();
    const size_type idxof = gtIdx( objid );
    return idxof < 0 ? 0 : objs_[idxof]->clone();
}


IOObj* DBDir::getEntryByName( const char* nm, const char* trgrpnm ) const
{
    mLock4Read();
    const IOObj* obj = gtObjByName( nm, trgrpnm );
    return obj ? obj->clone() : 0;
}


IOObj* DBDir::getEntryByIdx( size_type idx ) const
{
    mLock4Read();
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

    mLock4Read();

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
    if ( !writeToFile() )
	return false;

    mSendChgNotif( cEntryRemoved(), DBKey(dirid_,objid).toInt64() );
    return true;
}


DBDir::size_type DBDir::indexOf( ObjID objid ) const
{
    mLock4Read();
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
	if ( ioobj->hasName(nm) )
	{
	    if ( !trgrpnm || ioobj->group() == trgrpnm )
		return ioobj;
	}
    }
    return 0;
}


bool DBDir::hasObjectsWithGroup( const char* trgrpnm ) const
{
    for ( auto obj : objs_ )
	if ( obj->group() == trgrpnm )
	    return true;
    return false;
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
	const idx_type curidxof = gtIdx( dbky.objID() );
	if ( curidxof >= 0 )
	{
	    delete objs_.replace( curidxof, ioobj );
	    prepObj( *ioobj );
	    mSendChgNotif( cEntryChanged(), dbky.toInt64() );
	    return !writeafter || writeToFile();
	}
    }

    prepObj( *ioobj );
    objs_ += ioobj;
    mSendChgNotif( cEntryAdded(), dbky.toInt64() );

    return !writeafter || writeToFile();
}


bool DBDir::addAndWrite( IOObj* ioobj )
{
    {
	mLock4Write();
	objs_ += ioobj;
    }
    return writeToFile();
}


bool DBDir::prepObj( IOObj& ioobj ) const
{
    if ( ioobj.isSubdir() )
	ioobj.dirnm_ = dirname_;
    else
	ioobj.setDirName( File::Path(dirname_).fileName() );

    // ensure unique name
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
    errmsg_.appendPhrase( uiStrings::phrCheckPermissions() ); \
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
	    bool writenow;
	    if ( ipass == 0 )
		writenow = obj.objID().getI() == 1 ;
	    else if ( ipass == 1 )
		writenow = obj.isSubdir();
	    else
		writenow = obj.objID().getI() > 1 && !obj.isSubdir();

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

    mLock2Write();
    const_cast<DBDir*>(this)->readtime_ = Time::getFileTimeInSeconds();
    return true;
}


void DBDir::setObjDirName( IOObj& ioobj )
{
}


DBKey DBDir::newKey() const
{
    mLock4Write();
    return gtNewKey( curnr_ );
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
    : MonitorableIterBase<DBDir::size_type>(dbdir,0,dbdir.size()-1)
{
}


DBDirIter::DBDirIter( const DBDirIter& oth )
    : MonitorableIterBase<DBDir::size_type>(oth)
{
}


const DBDir& DBDirIter::dbDir() const
{
    return static_cast<const DBDir&>( monitored() );
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


DBDirEntryList::DBDirEntryList( const IOObjContext& ct, bool dofill )
    : ctxt_(*new IOObjContext(ct))
    , survloc_(*new SurveyDiskLocation)
{
    if ( dofill )
	fill( 0 );
}


DBDirEntryList::DBDirEntryList( const IOObjContext& ct,
				const SurveyDiskLocation& survloc, bool dofill )
    : ctxt_(*new IOObjContext(ct))
    , survloc_(*new SurveyDiskLocation(survloc))
{
    if ( dofill )
	fill( 0 );
}


DBDirEntryList::DBDirEntryList( const TranslatorGroup& tr,
				const char* allowedtransls )
    : ctxt_(*new IOObjContext(&tr))
    , survloc_(*new SurveyDiskLocation)
{
    ctxt_.toselect_.allowtransls_ = allowedtransls;
    fill( 0 );
}


DBDirEntryList::~DBDirEntryList()
{
    deepErase( entries_ );
    delete &ctxt_;
    delete &survloc_;
}


void DBDirEntryList::fill( const char* nmfilt )
{
    deepErase( entries_ );
    name_.setEmpty();
    ConstRefMan<DBDir> dbdir;

    const bool iscursurv = survloc_.isCurrentSurvey();
    BufferString datadirnm;
    if ( iscursurv )
	dbdir = DBM().fetchDir( ctxt_.getSelDirID() );
    else
    {
	File::Path fp( survloc_.fullPath() );
	fp.add( IOObjContext::getDataDirName(ctxt_.stdseltype_,true) );
	datadirnm = fp.fullPath();
	dbdir = new DBDir( datadirnm );
    }
    if ( !dbdir || dbdir->isBad() )
	return;

    name_ = dbdir->name();
    GlobExpr* ge = nmfilt && *nmfilt ? new GlobExpr(nmfilt) : 0;

    DBDirIter iter( *dbdir );
    while ( iter.next() )
    {
	const IOObj& obj = iter.ioObj();
	if ( !obj.isTmp() && ctxt_.validIOObj(obj) )
	{
	    if ( !ge || ge->matches(obj.name()) )
	    {
		IOObj* newentry = obj.clone();
		if ( !iscursurv )
		    newentry->setAbsDirectory( datadirnm );
		entries_ += newentry;
	    }
	}
    }

    delete ge;
    sort();
}


DBKey DBDirEntryList::key( idx_type idx ) const
{
    DBKey ret;
    if ( entries_.validIdx(idx) )
    {
	ret = entries_[idx]->key();
	if ( !survloc_.isCurrentSurvey() )
	    ret.setSurveyDiskLocation( survloc_ );
    }
    return ret;
}


BufferString DBDirEntryList::name( idx_type idx ) const
{
    if ( !entries_.validIdx(idx) )
	return BufferString::empty();
    return entries_[idx]->name();
}


BufferString DBDirEntryList::dispName( idx_type idx ) const
{
    if ( !entries_.validIdx(idx) )
	return BufferString::empty();

    const IOObj& obj = *entries_[idx];
    const DBKey dbky = entries_[idx]->key();
    const BufferString nm( obj.name() );
    if ( IOObj::isSurveyDefault(dbky) )
	return BufferString( "> ", nm, " <" );
    else if ( StreamProvider::isPreLoaded(dbky.toString(),true) )
	return BufferString( "/ ", nm, " \\" );
    return nm;
}


BufferString DBDirEntryList::iconName( idx_type idx ) const
{
    if ( entries_.validIdx(idx) )
    {
	const IOObj& obj = *entries_[idx];
	PtrMan<Translator> transl = obj.createTranslator();
	if ( transl )
	    return BufferString( transl->iconName() );
    }

    return BufferString::empty();
}


BufferStringSet DBDirEntryList::getParValuesFor( const char* parkey ) const
{
    BufferStringSet res;
    for ( idx_type idx=0; idx<size(); idx++ )
    {
	const IOObj* ioobj = entries_[idx];
	if ( ioobj )
	{
	    BufferString val;
	    if ( ioobj->group() == parkey )
		val = ioobj->translator();
	    else if ( !ioobj->pars().get( parkey, val ) )
		continue;

	    if ( !val.isEmpty() )
		res.addIfNew( val );
	}
    }
    return res;
}


void DBDirEntryList::sort()
{
    BufferStringSet nms; const size_type sz = size();
    for ( idx_type idx=0; idx<sz; idx++ )
	nms.add( entries_[idx]->name() );

    idx_type* idxs = nms.getSortIndexes();

    ObjectSet<IOObj> tmp( entries_ );
    entries_.erase();
    for ( idx_type idx=0; idx<sz; idx++ )
	entries_ += tmp[ idxs[idx] ];
    delete [] idxs;
}


DBDirEntryList::idx_type DBDirEntryList::indexOf( const char* nm ) const
{
    for ( idx_type idx=0; idx<size(); idx++ )
    {
	const IOObj& entry = *entries_[idx];
	if ( entry.hasName(nm) )
	    return idx;
    }
    return -1;
}
