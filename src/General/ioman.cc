/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ioman.h"

#include "commandlineparser.h"
#include "compoundkey.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "iodir.h"
#include "iostrm.h"
#include "iosubdir.h"
#include "keystrs.h"
#include "msgh.h"
#include "oddirs.h"
#include "perthreadrepos.h"
#include "safefileio.h"
#include "settings.h"
#include "strmprov.h"
#include "surveyfile.h"
#include "survinfo.h"
#include "timefun.h"
#include "transl.h"
#include "uistrings.h"

extern "C" { mGlobal(Basic) void SetCurBaseDataDir(const char*); }

using voidFromVoidFn = void(*)(void);
static voidFromVoidFn geom2dinitfn_ = nullptr;

mGlobal(General) void setGlobal_General_Fns(voidFromVoidFn);
void setGlobal_General_Fns( voidFromVoidFn geom2dinitfn )
{
    geom2dinitfn_ = geom2dinitfn;
}


class IOMManager : public CallBacker
{
public:

    IOMManager( const FilePath& rootdir )
	: iom_(*new IOMan(rootdir))
    {
	mAttachCB( iom_.applicationClosing, IOMManager::closedCB );
    }

    ~IOMManager()
    {
	detachAllNotifiers();
	delete& iom_;
    }

    void init( uiRetVal& uirv )
    {
	SurveyInfo* si = SurveyInfo::readDirectory( iom_.rootDir().fullPath() );
	uirv = iom_.init( si );
    }

    IOMan& IOM()	    { return iom_; }

    void applicationClosing()
    {
	if ( !closed_ )
	{
	    mDetachCB( iom_.applicationClosing, IOMManager::closedCB );
	    iom_.applClosing();
	}
    }

    static Threads::Lock	lock_;

private:

    void    closedCB( CallBacker* )
    {
	closed_ = true;
    }

    IOMan& iom_;
    bool    closed_ = false;

};

static PtrMan<IOMManager> theinstmgr = nullptr;
Threads::Lock IOMManager::lock_( true );

void applicationClosing()
{
    Threads::Locker locker( IOMManager::lock_ );
    if ( theinstmgr )
	theinstmgr->applicationClosing();
}


IOMan& IOM( const FilePath& rootdir, uiRetVal& uirv )
{
    Threads::Locker locker( IOMManager::lock_ );
    if ( !theinstmgr )
    {
	theinstmgr = new IOMManager( rootdir );
	theinstmgr->init( uirv );
	NotifyExitProgram( applicationClosing );
	IOMan::iomReady().trigger();
    }

    return theinstmgr->IOM();
}


IOMan& IOM()
{
    Threads::Locker locker( IOMManager::lock_ );
    if ( !theinstmgr )
    {
	FilePath dummyfp;
	theinstmgr = new IOMManager( dummyfp );
	pFreeFnErrMsg( "Should not be reached before "
		       "IOMan::setDataSource is called" );
	NotifyExitProgram( applicationClosing );
    }

    return theinstmgr->IOM();
}


// CustomDirData

IOMan::CustomDirData::CustomDirData( int dirkey, const char* dirnm,
				     const char* desc )
    : selkey_(dirkey,0)
    , dirname_(dirnm)
    , desc_(desc)
{
}


IOMan::CustomDirData::~CustomDirData()
{
}


// IOMan

IOMan::IOMan( const FilePath& rootdir )
    : NamedCallBacker("IO Manager")
    , entryRemoved(this)
    , entriesRemoved(this)
    , entryAdded(this)
    , entriesAdded(this)
    , entryChanged(this)
    , implUpdated(this)
    , newIODir(this)
    , prepareSurveyChange(this)
    , surveyToBeChanged(this)
    , surveyChanged(this)
    , afterSurveyChange(this)
    , applicationClosing(this)
    , rootdir_(rootdir)
    , curlvl_(-1)
    , lock_(false)
{
    SetCurBaseDataDir( rootdir.pathOnly().buf() );
    SurveyInfo::deleteInstance();
    SurveyInfo::setSurveyName( rootdir.baseName().buf() );
}


uiRetVal IOMan::init( SurveyInfo* nwsi )
{
    PtrMan<SurveyInfo> newsi = nwsi;
    state_ = Bad;
    if ( rootdir_.basePath().isEmpty() || rootdir_.dirName().isEmpty() )
	return tr("Survey Data Root is not set");

    const BufferString rootdirnm( rootdir_.fullPath() );
    if ( !File::isDirectory(rootdirnm.str()) )
	return tr("Survey Data Root does not exist or is not a folder:\n%1")
		.arg(rootdirnm.str());

    if ( !to(MultiID::udf(),true) )
    {
	FilePath surveyfp( rootdirnm.str(), ".omf" );
	if ( surveyfp.exists() )
	    return tr("Warning: Invalid '.omf' found in:\n%1.\n"
		      "This survey is corrupt.").arg( rootdirnm.str() );

	FilePath basicfp(
		GetSWSetupShareFileName(SurveyInfo::sKeyBasicSurveyName()),
			  ".omf" );
        File::copy( basicfp.fullPath(),surveyfp.fullPath() );
	if ( !to(MultiID::udf(),true) )
	    return tr("Warning: Invalid or no '.omf' found in:\n%1.\n"
		      "This survey is corrupt.").arg( rootdirnm.str() );
    }

    state_ = Good;
    curlvl_ = 0;
    SetCurBaseDataDir( rootdir_.basePath().str() );
    SurveyInfo::setSurveyName( rootdir_.dirName().str() );
    if ( newsi )
    {
	SurveyInfo::deleteInstance();
	SurveyInfo::pushSI( newsi.release() );
    }

    const int nrstddirdds = IOObjContext::totalNrStdDirs();
    const IOObjContext::StdDirData* prevdd = nullptr;
    bool needwrite = false;
    FilePath rootfp( rootdirnm.str(), "X" );
    uiRetVal uirv;
    for ( int idx=0; idx<nrstddirdds; idx++ )
    {
	auto stdseltyp = sCast(IOObjContext::StdSelType,idx);
	const IOObjContext::StdDirData* dd =
			IOObjContext::getStdDirData( stdseltyp );
	const IOObj* dirioobj = dirptr_->get( dd->id_ );
	if ( dirioobj )
	{
	    prevdd = dd;
	    continue;
	}

	// Oops, a data directory required is missing
	// We'll try to recover by using the 'Basic Survey' in the app
	FilePath basicfp(
		    GetSWSetupShareFileName(SurveyInfo::sKeyBasicSurveyName()),
			  "X" );
	basicfp.setFileName( dd->dirnm_ );
	if ( !basicfp.exists() )
	    // Oh? So this is removed from the Basic Survey
	    // Let's hope they know what they're doing
	    { prevdd = dd; continue; }

	rootfp.setFileName( dd->dirnm_ );
	BufferString dirnm = rootfp.fullPath();

#define mErrMsgRet(s) uirv = s; ErrMsg(s); state_ = Bad; return uirv
	if ( !File::exists(dirnm) )
	{
	    // This directory should have been in the survey.
	    // It is not. If it is the seismic directory, we do not want to
	    // continue. Otherwise, we want to copy the Basic Survey directory.
	    if ( stdseltyp == IOObjContext::Seis )
	    {
		mErrMsgRet( tr("Corrupt survey: missing directory: %1")
			    .arg(dirnm) );
	    }
	    else if ( !File::copy(basicfp.fullPath().buf(),dirnm) )
	    {
		mErrMsgRet( tr("Cannot create directory: %1.\n"
			    "You probably do not have write permissions in %2.")
			    .arg(dirnm).arg(rootfp.pathOnly()) );
	    }
	}

	// So, we have copied the directory.
	// Now create an entry in the root omf
	auto* iosd = new IOSubDir( dd->dirnm_ );
	iosd->key_ = dd->id_;
	iosd->dirnm_ = rootdirnm.str();
	const IOObj* previoobj = prevdd ? dirptr_->get( MultiID(prevdd->id_) )
					: dirptr_->main();
	int idxof = dirptr_->objs_.indexOf( (IOObj*)previoobj );
	dirptr_->objs_.insertAfter( iosd, idxof );

	prevdd = dd;
	needwrite = true;
    }

    if ( needwrite )
    {
	dirptr_->doWrite();
	to( MultiID::udf(), true );
    }

    if ( geom2dinitfn_ )
	(*geom2dinitfn_)();
    else
    {
	pErrMsg("The Geometry module is not loaded, "
		"did you forgot to call ensureLoaded(Geometry)?");
    }

    return uirv;
}


bool IOMan::isOK()
{
    Threads::Locker locker( IOMManager::lock_ );
    return theinstmgr.ptr() && !IOM().isBad();
}


bool IOMan::close( bool dotrigger )
{
    if ( dotrigger && !isBad() )
	prepareSurveyChange.trigger();

    if ( changeSurveyBlocked() )
    {
	setChangeSurveyBlocked( false );
	return false;
    }

    if ( dotrigger && !isBad() )
	surveyToBeChanged.trigger();

    TranslatorGroup::clearSelHists();

    deleteAndNullPtr( dirptr_ );
    survchgblocked_ = false;
    state_ = IOMan::NeedInit;

    return true;
}


uiRetVal IOMan::reInit( SurveyInfo* nwsi )
{
    PtrMan<SurveyInfo> newsi = nwsi;
    const bool dotrigger = newsi;
    if ( !close(dotrigger) ) //Still notifying using previous survey
	return uiRetVal::OK();

    rootdir_.setDirName( newsi->getDirName() );
    const uiRetVal uirv = init( newsi.release() );
    if ( uirv.isOK() )
    {
	setupCustomDataDirs(-1);
	if ( dotrigger )
	{
	    surveyChanged.trigger();
	    afterSurveyChange.trigger();
	}

	ResetDefaultDirs();
    }

    return uirv;
}


IOMan::~IOMan()
{
    delete dirptr_;
    delete prevrootdir_;
}


bool IOMan::isReady() const
{
    return isBad() || !dirptr_ ? false : !dirptr_->key().isUdf();
}


Notifier<IOMan>& IOMan::iomReady()
{
    Threads::Locker locker( IOMManager::lock_ );
    static PtrMan<Notifier<IOMan> > thenotif_;
    if ( !thenotif_ )
	thenotif_ = new Notifier<IOMan>( nullptr );

    return *thenotif_.ptr();
}


uiRetVal IOMan::newSurvey( SurveyInfo* nwsi )
{
    PtrMan<SurveyInfo> newsi = nwsi;
    if ( !newsi )
    {
	SurveyInfo::deleteInstance();
	SurveyInfo::setSurveyName( "" );
	IOM().close( true );
	return uiRetVal::OK();
    }

    const SurveyDiskLocation& rootdir = IOM().rootDir();
    if ( IOMan::isOK() && newsi->getDataDirName() != rootdir.basePath())
	pFreeFnErrMsg("Incorrect switching to another data root");

    return IOM().reInit( newsi.release() );
}


uiRetVal IOMan::setSurvey( const char* survname )
{
    Threads::Locker locker( IOMManager::lock_ );
    if ( !theinstmgr )
	{ pFreeFnErrMsg("Data root has not been set"); }

    locker.unlockNow();

    uiRetVal uirv;
    const SurveyDiskLocation& rootdir = IOM().rootDir();
    if ( !IOM().isBad() && rootdir.dirName() == survname )
	return uirv;

    const FilePath fp( IOM().rootDir().basePath(), survname );
    uirv = isValidSurveyDir( fp.fullPath() );
    if ( !uirv.isOK() )
	return uirv;

    return IOM().setRootDir( fp, true );
}


void IOMan::surveyParsChanged()
{
    if ( !IOM().isPreparedForSurveyChange() )
	return;

    IOM().surveyToBeChanged.trigger();
    IOM().surveyChanged.trigger();
    IOM().afterSurveyChange.trigger();
}


BufferString IOMan::surveyName() const
{
    return rootdir_.dirName();
}


static bool validOmf( const char* dir )
{
    FilePath fp( dir ); fp.add( ".omf" );
    BufferString fname = fp.fullPath();
    if ( File::isEmpty(fname) )
    {
	fp.setFileName( ".omb" );
	if ( File::isEmpty(fp.fullPath()) )
	    return false;
	else
	    File::copy( fname, fp.fullPath() );
    }
    return true;
}


#define mErrRet(str) \
    { errmsg = str; return false; }

#define mErrRetNotODDir(fname) \
    { \
	errmsg = "Survey Data Root: "; errmsg += GetBaseDataDir(); \
        errmsg += "\nThis is not a valid OpendTect data storage directory."; \
	if ( fname ) \
	    { errmsg += "\n[Cannot find: "; errmsg += fname; errmsg += "]"; } \
        return false; \
    }

bool IOMan::validSurveySetup( BufferString& errmsg )
{
    const BufferString basedatadir( GetBaseDataDir() );
    if ( basedatadir.isEmpty() )
	mErrRet("No Survey Data Root found in the user settings, "
		"or provided the command line (--dataroot dir), "
		"or provided using the environment variable DTECT_DATA")
    else if ( !File::exists(basedatadir) )
	mErrRetNotODDir("")
    else if ( !validOmf(basedatadir) )
	mErrRetNotODDir(".omf")

    const BufferString surveynm( SurveyInfo::curSurveyName() );
    if ( surveynm.isEmpty() )
    {
	// Survey name in ~/.od/survey is invalid or absent. If there, remove it
	const BufferString survfname = SurveyInfo::surveyFileName();
	if ( File::exists(survfname) && !File::remove(survfname) )
	{
	    errmsg.set( "The file: '" ).add( survfname )
		  .add( "' contains an invalid survey.\n\n"
			"Please remove this file" );
	}

	return false;
    }

    const BufferString projdir = GetDataDir();
    if ( projdir != basedatadir && File::isDirectory(projdir) )
    {
	const bool noomf = !validOmf( projdir );
	const bool nosurv = File::isEmpty( FilePath(projdir).
					   add(SurveyInfo::sKeySetupFileName()).
					   fullPath() );
	if ( !noomf && !nosurv )
	{
	    if ( IOMan::isOK() )
		return true; // This is normal

	    // But what's wrong here? In any case - survey is not good.
	}
	else
	{
	    BufferString msg;
	    if ( nosurv && noomf )
		msg = "Warning: Essential data files not found in ";
	    else if ( nosurv )
	    {
		msg = BufferString( "Warning: Invalid or no '",
				    SurveyInfo::sKeySetupFileName(),
				    "' found in " );
	    }
	    else if ( noomf )
		msg = "Warning: Invalid or no '.omf' found in ";
	    msg += projdir; msg += ".\nThis survey is corrupt.";
	    UsrMsg( msg );
	}
    }

    SurveyInfo::setSurveyName( "" ); // force user-set of survey
    if ( !IOMan::isOK() )
	return false;

    const uiRetVal uirv = IOM().reInit( nullptr );
    if ( !uirv.isOK() )
	errmsg = uirv.getText();

    return uirv.isOK();
}


uiRetVal IOMan::setRootDir( const FilePath& dirfp, bool ischecked )
{
    Threads::Locker lock( lock_ );
    const BufferString dirnm = dirfp.fullPath();
    if ( dirnm.isEmpty() )
	return tr( "Cannot set IOM Root Dir with empty path" );

    uiRetVal uirv;
    if ( dirnm == rootdir_.fullPath() )
	return uirv;

    if ( !ischecked )
    {
	uirv = isValidSurveyDir( dirnm.str() );
	if ( !uirv.isOK() )
	    return uirv;
    }

    rootdir_.set( dirnm.str() );
    SurveyInfo* newsi = SurveyInfo::readDirectory( dirnm.str() );
    uirv = reInit( newsi );
    if ( !uirv.isOK() )
	return uirv;

    return setDir( nullptr );
}


bool IOMan::to( const IOSubDir* sd, bool forcereread )
{
    if ( isBad() )
    {
	if ( !to(MultiID(0,MultiID::udf().objectID()),true) || isBad() )
	    return false;

	return to( sd, true );
    }
    else if ( !forcereread )
    {
	if ( !sd && curlvl_ == 0 )
	    return true;
	else if ( dirptr_ && sd && sd->key() == dirptr_->key() )
	    return true;
    }

    const BufferString dirnm( sd ? sd->dirName() : rootdir_.fullPath().buf() );
    if ( !File::isDirectory(dirnm.buf()) )
	return false;

    const uiRetVal uirv = setDir( dirnm.buf() );
    return uirv.isOK();
}


MultiID IOMan::createNewKey( const MultiID& dirkey )
{
    Threads::Locker lock( lock_ );
    if ( !to(dirkey,true) || !dirptr_ )
	return MultiID::udf();

    return dirptr_->newKey();
}


bool IOMan::to( IOObjContext::StdSelType type, bool force_reread )
{
    return to( IOObjContext::getStdDirData(type)->id_, force_reread );
}


bool IOMan::to( const MultiID& ky, bool forcereread )
{
    Threads::Locker lock( lock_ );
    if ( !File::isDirectory(rootdir_.fullPath()) || (!forcereread && isBad()) )
	return false;

    MultiID key = ky;
    if ( key.groupID()>MultiID::cFirstDatabaseGrpID() && key.objectID()<0 )
	key.setGroupID( MultiID::udf().groupID()).setObjectID(ky.groupID());

    const bool issamedir = dirptr_ && key.groupID() == dirptr_->key().groupID();
    if ( !forcereread && issamedir )
    {
	dirptr_->update();
	return true;
    }

    MultiID dirkey;
    IOObj* refioobj = IODir::getObj( key );
    if ( refioobj )
	dirkey = refioobj->isSubdir() ? key : key.mainID();
    else
    {
	dirkey = key.mainID();
	refioobj = IODir::getObj( dirkey );
	if ( !refioobj )
	    dirkey.setUdf();
    }

    delete refioobj;
    auto* newdir = dirkey.isUdf() ? new IODir( rootdir_.fullPath().buf() )
				  : new IODir( dirkey );
    if ( !newdir || newdir->isBad() )
    {
	delete newdir;
	return false;
    }

    bool needtrigger = dirptr_;
    if ( dirptr_ )
	delete dirptr_;

    dirptr_ = newdir;
    curlvl_ = levelOf( curDirName() );
    lock.unlockNow();
    if ( needtrigger )
	newIODir.trigger();

    return true;
}


IOObj* IOMan::get( const DBKey& ky ) const
{
    Threads::Locker lock( lock_ );
    if ( !IOObj::isKey(ky.MultiID::toString()) )
	return nullptr;

    return IODir::getObj( ky );
}


IOObj* IOMan::get( const MultiID& k ) const
{
    Threads::Locker lock( lock_ );
    if ( !IOObj::isKey(k.toString()) )
	return nullptr;

    MultiID ky = k.mainID();
    if ( dirptr_ && dirptr_->key().groupID() == ky.groupID() )
    {
	const IOObj* ioobj = dirptr_->get( ky );
	if ( ioobj )
	    return ioobj->clone();
    }

    return IODir::getObj( ky );
}


IOObj* IOMan::get( const OD::DataSetKey& dsky, const char* tgname ) const
{
    Threads::Locker lock( lock_ );
    if ( dsky.isUdf() || isBad() )
	return nullptr;

    /*TODO: use tgname if set to narrow down the search, although this
	    will only work if the corresponding translator group is loaded */

    /*TODO: not efficient, using the still empty dirptrs_ set
	    should be better	*/
    if ( dirptr_ )
    {
	const IOObj* ioobj = dirptr_->get( dsky );
	if ( ioobj )
	    return ioobj->clone();
    }

    return IODir::getObj( dsky );
}


IOObj* IOMan::getOfGroup( const char* tgname, bool first,
			  bool onlyifsingle ) const
{
    Threads::Locker lock( lock_ );
    if ( isBad() || !tgname || !dirptr_ )
	return nullptr;

    const IOObj* ioobj = nullptr;
    for ( int idx=0; idx<dirptr_->size(); idx++ )
    {
	if ( dirptr_->get(idx)->group()==tgname )
	{
	    if ( onlyifsingle && ioobj ) return nullptr;

	    ioobj = dirptr_->get( idx );
	    if ( first && !onlyifsingle ) break;
	}
    }

    return ioobj ? ioobj->clone() : nullptr;
}


IOObj* IOMan::getLocal( const char* objname, const char* trgrpnm ) const
{
    const StringView fsobjnm( objname );
    if ( fsobjnm.isEmpty() )
	return nullptr;

    if ( fsobjnm.startsWith("ID=<") )
    {
	BufferString oky( objname+4 );
	char* ptr = oky.find( '>' );
	if ( ptr )
	    *ptr = '\0';

	MultiID key;
	key.fromString( oky.buf() );
	return get( key );
    }

    if ( dirptr_ )
    {
	const IOObj* ioobj = dirptr_->get( objname, trgrpnm );
	if ( ioobj ) return ioobj->clone();
    }

    if ( IOObj::isKey(objname) )
    {
	MultiID key;
	return key.fromString(objname) ? get(key) : nullptr;
    }

    return nullptr;
}


IOObj* IOMan::getFirst( const IOObjContext& ctxt, int* nrfound ) const
{
    if ( nrfound )
	*nrfound = 0;

    Threads::Locker lock( lock_ );
    if ( !ctxt.trgroup_ || isBad() )
	return nullptr;

    if ( !IOM().to(ctxt.getSelKey()) || !dirptr_ )
	return nullptr;

    const ObjectSet<IOObj>& ioobjs = dirptr_->getObjs();
    IOObj* ret = nullptr;
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj* ioobj = ioobjs[idx];
	if ( ctxt.validIOObj(*ioobj) )
	{
	    if ( !ret )
		ret = ioobj->clone();
	    if ( nrfound )
		(*nrfound)++;
	    else
		break;
	}
    }

    return ret;
}


IOObj* IOMan::getFromPar( const IOPar& par, const char* bky,
			  const IOObjContext& ctxt,
			  bool mknew, BufferString& errmsg ) const
{
    Threads::Locker lock( lock_ );
    BufferString basekey( bky );
    if ( !basekey.isEmpty() )
	basekey.add( "." );

    BufferString iopkey( basekey );
    iopkey.add( sKey::ID() );
    BufferString res = par.find( iopkey );
    if ( res.isEmpty() )
    {
	iopkey = basekey; iopkey.add( sKey::Name() );
	res = par.find( iopkey );
	if ( res.isEmpty() )
	{
	    errmsg = BufferString( "Please specify '", iopkey, "'" );
	    return nullptr;
	}

	if ( !IOObj::isKey(res.buf()) )
	{
	    CtxtIOObj ctio( ctxt );
	    if ( !IOM().to(ctio.ctxt_.getSelKey()) || !dirptr_ )
		return nullptr;
	    const IOObj* ioob = dirptr_->get( res.buf(), nullptr );
	    if ( ioob )
		res = ioob->key();
	    else if ( mknew )
	    {
		ctio.setName( res );
		IOM().getEntry( ctio );
		if ( ctio.ioobj_ )
		{
		    IOM().commitChanges( *ctio.ioobj_ );
		    return ctio.ioobj_;
		}
	    }
	}
    }

    MultiID key;
    key.fromString( res.buf() );
    IOObj* ioobj = get( key );
    if ( !ioobj )
	errmsg = BufferString( "Value for ", iopkey, " is invalid." );

    return ioobj;
}


bool IOMan::isKey( const char* ky ) const
{
    if ( !ky || !*ky || !iswdigit(*ky) ) return false;

    bool digitseen = false;
    while ( *ky )
    {
	if ( iswdigit(*ky) )
	    digitseen = true;
	else if ( *ky == '|' )
	    return digitseen;
	else if ( *ky != '.' )
	    return false;
	ky++;
    }

    return true;
}


const char* IOMan::nameOf(const MultiID& key ) const
{
    const DBKey dbkey( key );
    return objectName( dbkey );
}


const char* IOMan::nameOf( const char* id ) const
{
    mDeclStaticString( ret );
    if ( !id || !*id || !IOObj::isKey(id) )
	return id;

    MultiID ky;
    IOObj* ioobj = ky.fromString(id) ? get( ky ) : nullptr;
    ret.setEmpty();
    if ( !ioobj )
	ret.set( "ID=<" ).add( id ).add( ">" );
    else
    {
	ret = ioobj->name();
	delete ioobj;
    }

    return ret.buf();
}


const char* IOMan::objectName( const DBKey& key ) const
{
    mDeclStaticString( ret );
    PtrMan<IOObj> ioobj = IODir::getObj( key );
    ret = ioobj ? ioobj->name().buf()
		: BufferString("ID=<",key.toString(false),">").buf();
    return ret.buf();
}


void IOMan::getObjectNames( const DBKeySet& keys, BufferStringSet& nms )
{
    nms.erase();
    for ( const auto* key : keys )
	nms.add( IOM().objectName(*key) );
}


BufferString IOMan::curDirName() const
{
    return dirptr_ ? dirptr_->dirName() : rootdir_.fullPath().buf();
}


const MultiID& IOMan::key() const
{
    return dirptr_ ? dirptr_->key() : MultiID::udf();
}


uiRetVal IOMan::setDir( const char* dirname )
{
    Threads::Locker lock( lock_ );
    BufferString dirnm( dirname );
    if ( dirnm.isEmpty() )
	dirnm.set( rootdir_.fullPath() );

    if ( dirnm.isEmpty() )
	return tr("Cannot set directory from empty string");

    PtrMan<IODir> newdirptr = new IODir( dirnm.str() );
    if ( !newdirptr )
	return tr("Cannot switch to database directory '%1'").arg( dirname );

    if ( newdirptr->isBad() )
	return tr("Cannot switch to database directory '%1'").arg( dirname );

    bool needtrigger = dirptr_;
    delete dirptr_;
    dirptr_ = newdirptr.release();
    curlvl_ = levelOf( curDirName() );

    lock.unlockNow();

    if ( needtrigger )
	newIODir.trigger();

    return uiRetVal::OK();
}


void IOMan::getEntry( CtxtIOObj& ctio, bool mktmp, int translidx )
{
    getObjEntry( ctio, false, mktmp, translidx );
}


void IOMan::getNewEntry( CtxtIOObj& ctio, bool mktmp, int translidx )
{
    getObjEntry( ctio, true, mktmp, translidx );
}


void IOMan::getNewEntries( ObjectSet<CtxtIOObj>& ctios, bool mktmp, int tridx )
{
    if ( ctios.isEmpty() )
	return;

    ManagedObjectSet<ObjectSet<CtxtIOObj>> groupctios;
    getCtxtObjsByGroup( ctios, groupctios );
    if ( groupctios.isEmpty() )
	return;

    for ( auto* ctxtios : groupctios )
    {
	bool dirptrset = false;
	for ( const auto* ctio : *ctxtios )
	{
	    Threads::Locker lock( lock_ );
	    if ( to(ctio->ctxt_.getSelKey()) )
	    {
		dirptrset = true;
		break;
	    }
	}

	if ( !dirptrset )
	    continue;

	getObjEntries( *ctxtios, true, mktmp, tridx );
    }
}


const IOObj* IOMan::getIOObjFromCtxt( const CtxtIOObj& ctio, bool isnew,
				      bool mktmp, int translidx )
{
    const IOObj* ioobj = dirptr_->get( ctio.ctxt_.name(),
				       ctio.ctxt_.trgroup_->groupName() );
    ctio.ctxt_.fillTrGroup();
    if ( ioobj && ctio.ctxt_.trgroup_->groupName() != ioobj->group() )
	ioobj = nullptr;

    if ( !ioobj || (isnew && !mktmp) )
    {
	MultiID newkey( mktmp ? ctio.ctxt_.getSelKey() : dirptr_->newKey() );
	if ( mktmp )
	    newkey.setObjectID( IOObj::tmpID() );

	ioobj = crWriteIOObj( ctio, newkey, translidx );
	if ( !ioobj )
	    return nullptr;
    }

    ioobj->pars().merge( ctio.ctxt_.toselect_.require_ );
    return ioobj;
}


void IOMan::getObjEntry( CtxtIOObj& ctio, bool isnew, bool mktmp,
								int translidx )
{
    ctio.setObj( nullptr );
    if ( ctio.ctxt_.name().isEmpty() )
	return;

    Threads::Locker lock( lock_ );
    if ( !to(ctio.ctxt_.getSelKey()) || !dirptr_ )
	return;

    const IOObj* ioobj = getIOObjFromCtxt( ctio, isnew, mktmp, translidx );
    bool needstrigger = false;
    if ( ioobj && !dirptr_->isPresent(ioobj->key()) )
    {
	if ( !dirptr_->addObj((IOObj*)ioobj) )
	    return;

	needstrigger = true;
    }

    ctio.setObj( ioobj ? ioobj->clone() : nullptr );
    lock.unlockNow();

    if ( needstrigger )
    {
	const MultiID mid = ioobj->key();
	entryAdded.trigger( mid );
    }
}


void IOMan::getObjEntries( const ObjectSet<CtxtIOObj>& ctios, bool isnew,
			   bool mktmp, int translidx )
{
    if ( ctios.isEmpty() )
	return;

    ObjectSet<IOObj> objs;
    ObjectSet<std::pair<CtxtIOObj&,IOObj*>> successctioobjpairs;
    for ( auto* ctio : ctios )
    {
	ctio->setObj( nullptr );
	if ( ctio->ctxt_.name().isEmpty() )
	    continue;

	Threads::Locker lock( lock_ );
	const IOObj* ioobj = getIOObjFromCtxt( *ctio, isnew,
						mktmp, translidx );
	lock.unlockNow();
	if ( !ioobj )
	    continue;

	objs.add( (IOObj*)ioobj );
	auto* pair = new std::pair<CtxtIOObj&,IOObj*>( *ctio, (IOObj*)ioobj );
	successctioobjpairs.add( pair );
    }

    if ( !dirptr_ || objs.isEmpty() )
	return;

    Threads::Locker lock( lock_ );
    if ( !dirptr_->addObjects(objs) )
	return;

    lock.unlockNow();
    TypeSet<MultiID> addedids;
    for ( const auto* pair : successctioobjpairs )
    {
	IOObj* ioobj = pair->second;
	if ( !ioobj )
	    continue;

	CtxtIOObj& ctxtio = pair->first;
	ctxtio.setObj( ioobj->clone() );
	addedids.add( ioobj->key() );
    }

    entriesAdded.trigger( addedids );
}


#define mIsValidTransl(transl) \
    IOObjSelConstraints::isAllowedTranslator( \
	    transl->userName(),ctio.ctxt_.toselect_.allowtransls_) \
	    && transl->isUserSelectable(false)

IOObj* IOMan::crWriteIOObj( const CtxtIOObj& ctio, const MultiID& newkey,
			    int translidx ) const
{
    const ObjectSet<const Translator>& templs =
	ctio.ctxt_.trgroup_->templates();

    if ( templs.isEmpty() )
    {
	BufferString msg( "Translator Group '",ctio.ctxt_.trgroup_->groupName(),
			  "' is empty." );
	msg.add( ".\nCannot create a default write IOObj for " )
	   .add( ctio.ctxt_.name() );
	pErrMsg( msg );
	return nullptr;
    }

    const Translator* templtr = nullptr;

    if ( templs.validIdx(translidx) )
	templtr = ctio.ctxt_.trgroup_->templates()[translidx];
    else if ( !ctio.ctxt_.deftransl_.isEmpty() )
	templtr = ctio.ctxt_.trgroup_->getTemplate(ctio.ctxt_.deftransl_,true);

    if ( !templtr )
    {
	translidx = ctio.ctxt_.trgroup_->defTranslIdx();
	if ( mIsValidTransl(templs[translidx]) )
	    templtr = templs[translidx];
	else
	{
	    for ( int idx=0; idx<templs.size(); idx++ )
	    {
		if ( mIsValidTransl(templs[idx]) )
		{
		    templtr = templs[idx];
		    break;
		}
	    }
	}
    }

    return templtr ? templtr->createWriteIOObj( ctio.ctxt_, newkey ) : nullptr;
}


bool IOMan::ensureUniqueName( IOObj& ioobj )
{
    if ( !dirptr_ || ioobj.key().groupID() != dirptr_->key().groupID() )
    {
	if ( !to(ioobj.key().mainID()) || !dirptr_ )
	    return false;
    }

    return dirptr_->ensureUniqueName( ioobj );
}


void IOMan::removeUnusable( DBKeySet& keys )
{
    for ( int idx=keys.size()-1; idx>=0; idx-- )
    {
	if ( !isUsable(keys.get(idx)) )
	    keys.removeSingle( idx );
    }
}


bool IOMan::isUsable( const DBKey& key ) const
{
    ConstPtrMan<IOObj> ioobj = IOM().get( key );
    return ioobj;
}


bool IOMan::isUsable( const MultiID& key ) const
{
    ConstPtrMan<IOObj> ioobj = IOM().get( key );
    return ioobj;
}


bool IOMan::implIsLink( const MultiID& key ) const
{
    ConstPtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj )
	return false;

    ConstPtrMan<Translator> trans = ioobj->createTranslator();
    return trans && trans->implIsLink( ioobj.ptr() );
}


bool IOMan::implExists( const MultiID& key ) const
{
    ConstPtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj )
	return false;

    ConstPtrMan<Translator> trans = ioobj->createTranslator();
    return trans ? trans->implExists( ioobj.ptr(), true )
		 : ioobj->implExists( true );
}


bool IOMan::isReadOnly( const MultiID& key ) const
{
    ConstPtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj )
	return false;

    ConstPtrMan<Translator> trans = ioobj->createTranslator();
    return trans ? trans->implReadOnly( ioobj.ptr() )
		 : ioobj->implReadOnly();
}


bool IOMan::implRename( const MultiID& key, const char* newname )
{
    BufferString newnm( newname );
    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj )
    {
	msg_ = uiStrings::phrCannotFindDBEntry( key );
	return false;
    }

    if ( newnm == ioobj->name() )
	return false;

    ConstPtrMan<IOObj> othioobj = IOM().getLocal( newnm, ioobj->group() );
    if ( othioobj )
    {
	msg_ = tr("Name '%1' is already in use.").arg( newnm );
	return false;
    }

    PtrMan<Translator> trans = ioobj->createTranslator();
    if ( !trans )
	return false;

    ioobj->setName( newnm );
    mDynamicCastGet(IOStream*,iostrm,ioobj.ptr())
    if ( !iostrm || trans->implIsLink(ioobj.ptr()) )
	return IOM().commitChanges( *ioobj );

    if ( !iostrm->implExists(true) )
    {
	iostrm->genFileName();
	return IOM().commitChanges( *ioobj );
    }

    IOStream chiostrm( *iostrm );
    FilePath fp( iostrm->fileSpec().fileName() );
    if ( trans )
	chiostrm.setExt( trans->defExtension() );

    BufferString cleannm( chiostrm.name() );
    cleannm.clean( BufferString::NoFileSeps );
    chiostrm.setName( cleannm );
    chiostrm.genFileName();
    chiostrm.setName( newnm );

    FilePath deffp( chiostrm.fileSpec().fileName() );
    fp.setFileName( deffp.fileName() );
    chiostrm.fileSpec().setFileName( fp.fullPath() );

    const bool newfnm = StringView(chiostrm.fileSpec().fileName())
				    != iostrm->fileSpec().fileName();
    if ( newfnm && !doReloc(key,trans.ptr(),*iostrm,chiostrm) )
    {
	if ( !newnm.contains('/') && !newnm.contains('\\') )
	    return false;

	newnm.clean( BufferString::AllowDots );
	chiostrm.setName( newnm );
	chiostrm.genFileName();
	deffp.set( chiostrm.fileSpec().fileName() );
	fp.setFileName( deffp.fileName() );
	chiostrm.fileSpec().setFileName( fp.fullPath() );
	chiostrm.setName( iostrm->name() );
	if ( !doReloc(key,trans.ptr(),*iostrm,chiostrm) )
	    return false;
    }

    iostrm->copyFrom( &chiostrm );
    return IOM().commitChanges( *ioobj );
}


bool IOMan::implReloc( const MultiID& key, const char* newdir )
{
    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj )
    {
	msg_ = tr("Cannot create an object for %1").arg( key );
	return false;
    }

    PtrMan<Translator> trans = ioobj->createTranslator();
    if ( trans->implIsLink(ioobj.ptr()) )
    {
	bool allsuccess = true;
	BufferStringSet fnms;
	trans->implFileNames( ioobj.ptr(), fnms);
	BufferStringSet oldfnms, newfnms;
	for ( const auto* fnm : fnms )
	{
	    const FilePath oldfp( *fnm );
	    const FilePath objfp( ioobj->fullUserExpr() );
	    const BufferString objext = objfp.extension();
	    const bool isobjdeffile = objext.endsWith( "def" );
	    if ( isobjdeffile && oldfp == FilePath(ioobj->fullUserExpr()) )
	    {
		pErrMsg("Internal error: implFileNames should not include "
			"the *.*def file.");
		continue;
	    }

	    oldfnms.add( *fnm );
	    FilePath newfp( *fnm );
	    newfp.setPath( newdir );
	    if ( !trans->implRelocate(ioobj.ptr(), newfp.fullPath(),
					     oldfp.fullPath()) )
	    {
		allsuccess = false;
		oldfnms.removeSingle( oldfnms.size()-1 );
		break;
	    }

	    newfnms.add( newfp.fullPath() );
	}

	if ( !allsuccess )
	{
	    msg_ = tr("Some files could not be relocated to the new location");
	    for ( const auto* newfnm : newfnms )
	    {
		const int idx = newfnms.indexOf( newfnm );
		trans->implRelocate( ioobj.ptr(), oldfnms.get(idx), *newfnm );
	    }
	}

	return allsuccess;
    }

    mDynamicCastGet(IOStream*,iostrm,ioobj.ptr())
    BufferString oldfnm( iostrm->fullUserExpr() );
    IOStream chiostrm( *iostrm );
    if ( !File::isDirectory(newdir) )
	return false;

    FilePath fp( oldfnm );
    fp.setPath( newdir );
    const FilePath defdirfp( curDirName().buf() );
    fp.makeRelativeTo( defdirfp );
    chiostrm.fileSpec().setFileName( fp.fullPath() );
    if ( !doReloc(key,trans.ptr(),*iostrm,chiostrm) )
	return false;

    IOM().commitChanges( *ioobj );
    return true;
}


bool IOMan::implRemove( const MultiID& key, bool rmentry, uiRetVal* uirv )
{
    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj )
	return false;

    if ( !implRemove(*ioobj) && !rmentry )
    {
	if ( uirv )
	{
	    const StringView objname = ioobj->name().buf();
	    uiString errmsg;
	    if ( !objname.isEmpty() )
		errmsg.append( tr("%1: ").arg(objname) );

	    errmsg.append( tr("Could not delete data file(s).") );
	    uirv->add( errmsg );
	}

	return false;
    }

    if ( rmentry && !IOM().permRemove(key) )
	return false;

    return true;
}


void IOMan::getIdsByGroup( const TypeSet<MultiID>& keys,
			   ObjectSet<TypeSet<MultiID>>& grpids )
{
    if ( keys.isEmpty() )
	return;

    std::unordered_map<int,TypeSet<MultiID>> grpidsmap;
    for ( const auto& key : keys )
    {
	const int grpid = key.groupID();
	grpidsmap[grpid] += key;
    }

    for ( const auto& pair : grpidsmap )
    {
	auto* ids = new TypeSet<MultiID>( pair.second );
	grpids.add( ids );
    }
}


void IOMan::getObjsByGroup( const ObjectSet<const IOObj>& objs,
			    ObjectSet<ObjectSet<const IOObj>>& groupedobjs )
{
    if ( objs.isEmpty() )
	return;

    std::unordered_map<int,ObjectSet<const IOObj>> grpobjsmap;
    for ( const auto* obj : objs )
    {
	const int group = obj->key().groupID();
	grpobjsmap[group].add( obj );
    }

    for ( auto& pair : grpobjsmap )
    {
	auto* objects = new ManagedObjectSet<const IOObj>( pair.second );
	groupedobjs.add( objects );
    }
}


void IOMan::getCtxtObjsByGroup( const ObjectSet<CtxtIOObj>& ctxts,
				ObjectSet<ObjectSet<CtxtIOObj>>& groupedctxts )
{
    if ( ctxts.isEmpty() )
	return;

    std::unordered_map<int,ObjectSet<CtxtIOObj>> grpctxtsmap;
    for ( auto* ctio : ctxts )
    {
	const int group = ctio->ctxt_.getSelKey().groupID();
	grpctxtsmap[group].add( ctio );
    }

    for ( auto& pair : grpctxtsmap )
    {
	auto* objects = new ObjectSet<CtxtIOObj>( pair.second );
	groupedctxts.add( objects );
    }
}


bool IOMan::implRemove( const TypeSet<MultiID>& ids,
			bool rmentry, uiRetVal* uirv )
{
    if ( ids.isEmpty() )
	return false;

    ManagedObjectSet<TypeSet<MultiID>> groupkeys;
    getIdsByGroup( ids, groupkeys );
    bool allok = true;
    for ( const auto* keys : groupkeys )
    {
	bool dirptrset = false;
	for ( const auto& key : *keys )
	{
	    if ( !dirptrset )
	    {
		if ( !to(key.mainID()) )
		{
		    allok = false;
		    continue;
		}

		dirptrset = true;
	    }

	    uiRetVal suirv;
	    if ( !implRemove(key,false,&suirv) )
	    {
		if ( uirv )
		    uirv->add( suirv );
		else
		    msg_.appendPhrase( suirv );

		continue;
	    }
	}

	if ( rmentry && !permRemove(*keys) )
	    allok = false;
    }

    return allok;
}


bool IOMan::implRemove( const IOObj& obj, bool deep ) const
{
    PtrMan<Translator> trans = obj.createTranslator();
    return trans && trans->implRemove( &obj, deep );
}


bool IOMan::doReloc( const MultiID& key, Translator* trans,
		     IOStream& iostrm, IOStream& chiostrm )
{
    const bool oldimplexist = trans ? trans->implExists( &iostrm, true )
				    : iostrm.implExists( true );
    const BufferString newfname( chiostrm.fullUserExpr() );

    bool succeeded = true;
    if ( oldimplexist )
    {
	const bool newimplexist = trans ? trans->implExists(&chiostrm, true)
					: chiostrm.implExists(true);
	if ( newimplexist && !implRemove(key) )
	    return false;

	succeeded = trans ? trans->implRename( &iostrm, newfname )
			  : iostrm.implRename( newfname );
    }

    if ( succeeded )
	iostrm.fileSpec().setFileName( newfname );

    return succeeded;
}


IOObj* IOMan::get( const char* objname, const char* tgname ) const
{
    for ( int itype=0; itype<TranslatorGroup::groups().size(); itype++ )
    {
	const TranslatorGroup& tgrp = *TranslatorGroup::groups()[itype];
	if ( tgname && StringView(tgname) != tgrp.groupName() )
	    continue;

	IODir iodir( tgrp.ioCtxt().getSelKey() );
	if ( iodir.isBad() )
	    continue;

	const IOObj* res = iodir.get( objname, tgname );
	if ( !res )
	    continue;

	return res->clone();
    }

    return nullptr;
}


IOObj* IOMan::get( const IOObjContext& ctxt, const char* objnm ) const
{
    return get( objnm, ctxt.trgroup_->groupName() );
}


bool IOMan::isPresent( const MultiID& key ) const
{
    PtrMan<IOObj> obj = get( key );
    return obj;
}


bool IOMan::isPresent( const char* objname, const char* tgname ) const
{
    PtrMan<IOObj> obj = get( objname, tgname );
    return obj;
}


int IOMan::levelOf( const char* dirnm ) const
{
    Threads::Locker lock( lock_ );
    if ( !dirnm )
	return 0;

    int lendir = StringView( dirnm ).size();
    int lenrootdir = rootdir_.fullPath().size();
    if ( lendir <= lenrootdir )
	return 0;

    int lvl = 0;
    const char* ptr = ((const char*)dirnm) + lenrootdir;
    while ( ptr )
    {
	ptr++; lvl++;
	ptr = firstOcc( ptr, *FilePath::dirSep(FilePath::Local) );
    }
    return lvl;
}


bool IOMan::commitChanges( const IOObj& ioobj )
{
    Threads::Locker lock( lock_ );
    PtrMan<IOObj> clone = ioobj.clone();
    to( clone->key() );

    return dirptr_ ? dirptr_->commitChanges( clone.ptr() ) : false;
}


bool IOMan::commitChanges( const ObjectSet<const IOObj>& objs )
{
    if ( objs.isEmpty() )
	return false;

    ManagedObjectSet<ObjectSet<const IOObj>> grpobjs;
    getObjsByGroup( objs, grpobjs );
    bool allok = true;
    for ( const auto* objects : grpobjs )
    {
	if ( objects->isEmpty() )
	    continue;

	bool dirptrset = false;
	ObjectSet<const IOObj> objstocommit;
	for ( const auto* object : *objects )
	{
	    if ( !object || object->isBad() )
		continue;

	    if ( !dirptrset )
	    {
		Threads::Locker lock( lock_ );
		dirptrset = to( objects->first()->key() );
	    }

	    objstocommit.add( object->clone() );
	}

	if ( objstocommit.isEmpty() )
	{
	    allok = false;
	    continue;
	}

	Threads::Locker lock( lock_ );
	if ( !dirptr_ || !dirptr_->commitChanges(objstocommit) )
	    allok = false;
    }

    return allok;
}


bool IOMan::permRemove( const MultiID& ky )
{
    const bool issurvdefault = IOObj::isSurveyDefault( ky );
    PtrMan<IOObj> ioobj = IOM().get( ky );
    if ( !ioobj )
    {
	msg_ = uiStrings::phrCannotFindDBEntry( ky );
	return false;
    }

    PtrMan<Translator> trl = ioobj->createTranslator();
    if ( !trl )
    {
	msg_ = tr("Could not retrieve translator of object '%1'")
	       .arg( ioobj->name() );
	return false;
    }

    const CompoundKey defaultkey(
		trl->group()->getSurveyDefaultKey(ioobj.ptr()) );
    Threads::Locker lock( lock_ );
    if ( !dirptr_ || !dirptr_->permRemove(ky) )
	return false;

    lock.unlockNow();

    entryRemoved.trigger( ky );
    if ( issurvdefault )
    {
	SI().getPars().removeWithKey( defaultkey.buf() );
	SI().savePars();
    }

    return true;
}


bool IOMan::permRemove( const TypeSet<MultiID>& keys )
{
    if ( keys.isEmpty() )
	return false;

    TypeSet<MultiID> goodkeys;
    BoolTypeSet issurvdefset;
    issurvdefset.setSize( keys.size() );
    ManagedObjectSet<CompoundKey> defaultkeys;
    defaultkeys.setNullAllowed( true );
    for ( const auto& key : keys )
    {
	const bool issurvdefault = IOObj::isSurveyDefault( key );
	PtrMan<IOObj> ioobj = IOM().get( key );
	if ( !ioobj )
	{
	    msg_ = uiStrings::phrCannotFindDBEntry( key );
	    issurvdefset.add( false );
	    defaultkeys.add( nullptr );
	    continue;
	}

	PtrMan<Translator> trl = ioobj->createTranslator();
	if ( !trl )
	{
	    msg_ = tr("Could not retrieve translator of object '%1'")
							  .arg( ioobj->name() );
	    issurvdefset.add( false );
	    defaultkeys.add( nullptr );
	    continue;
	}

	goodkeys.addIfNew( key );
	auto* defaultkey = new CompoundKey(
			      trl->group()->getSurveyDefaultKey(ioobj.ptr()) );
	issurvdefset.add( issurvdefault );
	defaultkeys.add( defaultkey );
    }

    if ( goodkeys.isEmpty() )
	return false;

    Threads::Locker lock( lock_ );
    if ( !dirptr_ || !dirptr_->permRemove(goodkeys) )
	return false;

    lock.unlockNow();
    entriesRemoved.trigger( goodkeys );
    if ( issurvdefset.isEmpty() )
    {
	bool parschanged = false;
	for ( int idx=0; idx<issurvdefset.size(); idx++ )
	{
	    const bool issurvdef = issurvdefset[idx];
	    const CompoundKey* defaultkey = defaultkeys.get( idx );
	    if ( issurvdef && defaultkey )
	    {
		SI().getPars().removeWithKey( defaultkey->buf() );
		parschanged = true;
	    }
	}

	if ( parschanged )
	    SI().savePars();
    }

    return true;
}


class SurveyDataTreePreparer
{
public:
			SurveyDataTreePreparer( const IOMan::CustomDirData& dd )
			    : dirdata_(dd)		{}

    bool		prepDirData();
    bool		prepSurv();
    bool		createDataTree();

    const IOMan::CustomDirData&	dirdata_;
    BufferString	errmsg_;
};


#undef mErrRet
#define mErrRet(s1,s2,s3) \
	{ errmsg_ = s1; errmsg_ += s2; errmsg_ += s3; return false; }


bool SurveyDataTreePreparer::prepDirData()
{
    IOMan::CustomDirData* dd = const_cast<IOMan::CustomDirData*>( &dirdata_ );

    dd->desc_.replace( ':', ';' );
    dd->dirname_.clean();

    const int nr = dd->selkey_.groupID();
    if ( nr <= 200000 )
	mErrRet("Invalid selection key passed for '",dd->desc_,"'")

    dd->selkey_.setObjectID( 0 );
    return true;
}


bool SurveyDataTreePreparer::prepSurv()
{
    if ( IOM().isBad() ) { errmsg_ = "Bad directory"; return false; }

    PtrMan<IOObj> ioobj = IOM().get( dirdata_.selkey_ );
    if ( ioobj ) return true;

    IOM().toRoot();
    if ( IOM().isBad() )
	{ errmsg_ = "Can't go to root of survey"; return false; }
    IODir* topdir = IOM().dirptr_;
    if ( !topdir || !topdir->main() || topdir->main()->name() == "Appl dir" )
	return true;

    if ( !createDataTree() )
	return false;

    // Maybe the parent entry is already there, then this would succeeed now:
    ioobj = IOM().get( dirdata_.selkey_ );
    if ( ioobj ) return true;

    if ( !IOM().dirptr_->addObj(IOMan::getIOSubDir(dirdata_),true) )
	mErrRet( "Couldn't add ", dirdata_.dirname_, " directory to root .omf" )
    return true;
}


bool SurveyDataTreePreparer::createDataTree()
{
    if ( !IOM().dirptr_ ) { errmsg_ = "Invalid current survey"; return false; }

    FilePath fp( IOM().dirptr_->dirName() );
    fp.add( dirdata_.dirname_ );
    const BufferString thedirnm( fp.fullPath() );
    bool dircreated = false;
    if ( !File::exists(thedirnm) )
    {
	if ( !File::createDir(thedirnm) )
	    mErrRet( "Cannot create '", dirdata_.dirname_,
		     "' directory in survey");
	dircreated = true;
    }

    fp.add( ".omf" );
    const BufferString omffnm( fp.fullPath() );
    if ( File::exists(omffnm) )
	return true;

    od_ostream strm( fp );
    if ( !strm.isOK() )
    {
	if ( dircreated )
	    File::remove( thedirnm );
	mErrRet( "Could not create '.omf' file in ", dirdata_.dirname_,
		 " directory" );
    }

    strm << GetProjectVersionName();
    strm << "\nObject Management file\n";
    strm << Time::getISODateTimeString();
    strm << "\n!\nID: " << dirdata_.selkey_.groupID() << "\n!\n"
	      << dirdata_.desc_ << ": 1\n"
	      << dirdata_.desc_ << " directory: Gen`Stream\n"
		"$Name: Main\n!"
	      << od_endl;
    return true;
}


static TypeSet<IOMan::CustomDirData>& getCDDs()
{
    mDefineStaticLocalObject( PtrMan<TypeSet<IOMan::CustomDirData> >, cdds,
			      = new TypeSet<IOMan::CustomDirData> );
    return *cdds;
}


const MultiID& IOMan::addCustomDataDir( const IOMan::CustomDirData& dd )
{
    SurveyDataTreePreparer sdtp( dd );
    if ( !sdtp.prepDirData() )
    {
	ErrMsg( sdtp.errmsg_ );
	static MultiID noid;
	return noid;
    }

    TypeSet<IOMan::CustomDirData>& cdds = getCDDs();
    for ( int idx=0; idx<cdds.size(); idx++ )
    {
	const IOMan::CustomDirData& cdd = cdds[idx];
	if ( cdd.selkey_ == dd.selkey_ )
	    return cdd.selkey_;
    }

    cdds += dd;
    int idx = cdds.size() - 1;
    const char* survnm = IOM().surveyName();
    if ( survnm && *survnm )
	setupCustomDataDirs( idx );
    return cdds[idx].selkey_;
}


void IOMan::setupCustomDataDirs( int taridx )
{
    const TypeSet<IOMan::CustomDirData>& cdds = getCDDs();
    for ( int idx=0; idx<cdds.size(); idx++ )
    {
	if ( taridx >= 0 && idx != taridx )
	    continue;

	SurveyDataTreePreparer sdtp( cdds[idx] );
	if ( !sdtp.prepSurv() )
	    ErrMsg( sdtp.errmsg_ );
    }
}


IOSubDir* IOMan::getIOSubDir( const IOMan::CustomDirData& cdd )
{
    auto* sd = new IOSubDir( cdd.dirname_ );
    sd->setDirName( IOM().rootDir().fullPath() );
    sd->setKey( cdd.selkey_ );
    sd->isbad_ = false;
    return sd;
}


uiRetVal IOMan::isValidDataRoot( const char* basedatadir )
{
    uiRetVal uirv = SurveyInfo::isValidDataRoot( basedatadir );
    if ( !uirv.isOK() )
	return uirv;

    const IODir datarootdir( basedatadir );
    if ( datarootdir.isEmpty() )
	uirv = tr("Data root folder '%1' is empty").arg( basedatadir );
    else if ( datarootdir.isBad() || !datarootdir.get("Appl dir","Appl") )
	uirv = tr("Data root folder '%1' seems corrupted").arg( basedatadir );

    return uirv;
}


bool IOMan::prepareDataRoot( const char* dirnm )
{
    const BufferString stdomf( GetSWSetupShareFileName("omf") );
    const BufferString datarootomf = FilePath( dirnm ).add( ".omf" ).fullPath();
    if ( !File::copy(stdomf,datarootomf) )
       return false;

    const uiRetVal uirv = isValidDataRoot( dirnm );
    return uirv.isOK();
}


uiRetVal IOMan::isValidSurveyDir( const char* surveydir )
{
    return SurveyInfo::isValidSurveyDir( surveydir );
}


uiRetVal IOMan::setDataSource( const char* dataroot, const char* survdir,
			       bool /* refresh */ )
{
    uiRetVal uirv = isValidDataRoot( dataroot );
    if ( uirv.isOK() )
	SetCurBaseDataDir( dataroot );
    else
	return uirv;

    const SurveyDiskLocation sdl( survdir, dataroot );
    const BufferString iomrootdir = sdl.fullPath();
    uirv = isValidSurveyDir( iomrootdir.buf() );
    if ( !uirv.isOK() )
	return uirv;

    const FilePath rootdirfp( iomrootdir.buf() );
    Threads::Locker locker( IOMManager::lock_ );
    if ( !theinstmgr )
    {
	IOM( rootdirfp, uirv );
	locker.unlockNow();
	return uirv;
    }

    return IOM().setRootDir( rootdirfp, true );
}


uiRetVal IOMan::setDataSource( const char* fullpath, bool refresh )
{
    const FilePath fp( fullpath );
    return setDataSource( fp.pathOnly().buf(), fp.fileName().buf(), refresh );
}


uiRetVal IOMan::setDataSource( const IOPar& iop, bool refresh )
{
    BufferString dataroot, survdir;
    if ( !iop.get(sKey::DataRoot(),dataroot) )
	dataroot.set( GetBaseDataDir() );

    if ( !iop.get(sKey::Survey(),survdir) )
	survdir.set( SurveyInfo::curSurveyName() );

    return setDataSource( dataroot.buf(), survdir.buf(), refresh );
}


EmptyTempSurvey& StartupSurvey( const char* surveydir, const char* dataroot )
{
    static PtrMan<EmptyTempSurvey> theinst =
				   new EmptyTempSurvey( surveydir, dataroot );
    return *theinst.ptr();
}


static void UnmountStartupSurvey( CallBacker* )
{
    StartupSurvey( nullptr, nullptr ).unmount( false );
}


uiRetVal IOMan::setDataSource( const CommandLineParser& clp, bool refresh )
{
    BufferString dataroot, survdir;
    const bool hasdataroot = clp.getVal( CommandLineParser::sDataRootArg(),
					 dataroot );
    const bool hassurveynm = clp.getVal( CommandLineParser::sSurveyArg(),
					 survdir );
    const bool needtempsurvey = clp.hasKey( CommandLineParser::sNeedTempSurv());
    if ( needtempsurvey )
    {
	EmptyTempSurvey& tempsurvey =
			 StartupSurvey( survdir.buf(), dataroot.buf() );
	uiRetVal uirv = tempsurvey.mount();
	if ( !uirv.isOK() )
	    return uirv;

	uirv = tempsurvey.activate();
	if ( uirv.isOK() && isOK() )
	    IOM().applicationClosing.notify( mSCB(UnmountStartupSurvey) );
    }

    if ( !hasdataroot )
	dataroot.set( GetBaseDataDir() );

    if ( !hassurveynm )
	survdir.set( SurveyInfo::curSurveyName() );

    return setDataSource( dataroot.buf(), survdir.buf(), refresh );
}


bool IOMan::recordDataSource( const SurveyDiskLocation& sdl, uiRetVal& uirv )
{
    const bool survok = writeSettingsSurveyFile( sdl.dirName(), uirv );
    const bool datadirok = SetSettingsDataDir( sdl.basePath(), uirv );
    return survok && datadirok;
}


bool IOMan::writeSettingsSurveyFile( const char* surveydirnm, uiRetVal& uirv )
{
    const BufferString survfnm( SurveyInfo::surveyFileName() );
    if ( survfnm.isEmpty() )
    {
	uirv.add( uiStrings::phrInternalErr(
			    "Cannot construct last-survey-filename" ) );
	return false;
    }

    SafeFileIO sfio( survfnm );
    if ( !sfio.open(false) )
    {
	uirv.add( mToUiStringTodo(sfio.errMsg() ) );
	return false;
    }

    sfio.ostrm() << surveydirnm;
    if ( !sfio.closeSuccess() )
    {
	uirv.add( mToUiStringTodo(sfio.errMsg() ) );
	return false;
    }

    return true;
}


IODir* IOMan::getDir( const char* trlgrpnm ) const
{
    const int nrstddirdds = IOObjContext::totalNrStdDirs();
    for ( int idx=0; idx<nrstddirdds; idx++ )
    {
	IOObjContext::StdSelType stdseltyp = (IOObjContext::StdSelType)idx;
	const IOObjContext::StdDirData* dd =
			IOObjContext::getStdDirData( stdseltyp );
	const FilePath fp( rootDir().fullPath(), dd->dirnm_ );
	if ( !fp.exists() )
	    continue;

	IODir* iodir = new IODir( fp.fullPath().buf() );
	if ( iodir->hasObjectsWithGroup(trlgrpnm) )
	    return iodir;

	delete iodir;
    }

    return nullptr;
}


IODir* IOMan::getDir( IOObjContext::StdSelType seltype ) const
{
    const IOObjContext::StdDirData* dd =
		IOObjContext::getStdDirData( seltype );
    if ( !dd )
	return nullptr;

    const FilePath fp( rootDir().fullPath(), dd->dirnm_ );
    return fp.exists() ? new IODir( fp.fullPath().buf() ) : nullptr;
}


IODir* IOMan::getDir( const MultiID& mid ) const
{
    const int nrstddirdds = IOObjContext::totalNrStdDirs();
    for ( int idx=0; idx<nrstddirdds; idx++ )
    {
	IOObjContext::StdSelType stdseltyp = (IOObjContext::StdSelType)idx;
	const IOObjContext::StdDirData* dd =
			IOObjContext::getStdDirData( stdseltyp );
	if ( mid.groupID() == dd->groupID() )
	{
	    const FilePath fp( rootDir().fullPath(), dd->dirnm_ );
	    return fp.exists() ? new IODir( fp.fullPath().buf() ) : nullptr;
	}
    }

    return nullptr;
}


BufferString IOMan::getNewTempDataRootDir()
{
    const BufferString tmpdataroot =
		       FilePath::getTempFullPath( "dataroot", nullptr );
    if ( !File::createDir(tmpdataroot) )
	return BufferString::empty();

    if ( !prepareDataRoot(tmpdataroot) )
    {
	File::removeDir( tmpdataroot );
	return BufferString::empty();
    }

    return tmpdataroot;
}


mExternC(General) const char* setDBMDataSource( const char* fullpath,
						bool refresh )
{
    mDeclStaticString(ret);
    const uiRetVal uirv = IOMan::setDataSource( fullpath, refresh );
    ret = uirv.getText();
    return ret.buf();
}


uiRetVal IOMan::setTempSurvey( const SurveyDiskLocation& sdl )
{
    const BufferString dataroot( sdl.basePath() );
    uiRetVal uirv = isValidDataRoot( dataroot );
    if ( !uirv.isOK() )
	return uirv;

    const BufferString temprootdir( sdl.fullPath() );
    uirv = isValidSurveyDir( temprootdir.buf() );
    if ( !uirv.isOK() )
	return uirv;

    delete prevrootdir_;
    const FilePath origrootdirfp( rootdir_.fullPath() );
    SurveyInfo::setSurveyName( "" );
    uirv = setRootDir( FilePath(temprootdir.buf()), true );
    if ( uirv.isOK() )
	prevrootdir_ = new SurveyDiskLocation( origrootdirfp );
    else
	rootdir_.set( origrootdirfp.fullPath() );

    return uirv;
}


uiRetVal IOMan::cancelTempSurvey()
{
    uiRetVal uirv;
    if ( !prevrootdir_ )
    {
	uirv = tr("Cannot cancel temp survey: No valid project before");
	return uirv;
    }

    SurveyInfo::setSurveyName( "" );
    uirv = setRootDir( FilePath(prevrootdir_->fullPath()), true );
    if ( !uirv.isOK() )
    {
	uirv.add( tr("\nFailed to switch back to the current survey\n%1\n\n."
		     "Please restart the application to continue")
		  .arg(rootdir_.fullPath()) );
    }

    deleteAndNullPtr( prevrootdir_ );
    return uirv;
}


bool IOMan::isPreparedForSurveyChange()
{
    IOM().prepareSurveyChange.trigger();
    if ( IOM().changeSurveyBlocked() )
    {
	IOM().setChangeSurveyBlocked( false );
	return false;
    }

    return true;
}


// SurveyChanger
SurveyChanger::SurveyChanger( const SurveyDiskLocation& sdl )
{
    if ( sdl.isCurrentSurvey() )
	return;

    const SurveyDiskLocation cursdl = IOM().rootDir();
    if ( cursdl != sdl )
	msg_ = IOM().setTempSurvey( sdl );
}


SurveyChanger::~SurveyChanger()
{
    if ( hasChanged() )
	IOM().cancelTempSurvey();
}


bool SurveyChanger::hasChanged()
{
    return IOM().isUsingTempSurvey();
}


SurveyDiskLocation SurveyChanger::changedToSurvey()
{
    return IOM().rootDir();
}


static int noConv2DSeisStatusFn( void ) { return 0; }
static void noConv2DSeisFn(uiString&,TaskRunner*)	{}
using intFromVoidFn = int(*)();
using voidFromuiStringFn = void(*)(uiString&,TaskRunner*);
static intFromVoidFn localconv2dseisstatusfn_ = noConv2DSeisStatusFn;
static voidFromuiStringFn localconv2dseisfn_ = noConv2DSeisFn;

mGlobal(General) void setConv2DSeis_General_Fns(intFromVoidFn,
						voidFromuiStringFn);
void setConv2DSeis_General_Fns( intFromVoidFn convstatusfn,
				voidFromuiStringFn convfn )
{
    localconv2dseisstatusfn_ = convstatusfn;
    localconv2dseisfn_ = convfn;
}

mGlobal(General) int OD_Get_2D_Data_Conversion_Status()
{
    return (*localconv2dseisstatusfn_)();
}

mGlobal(General) void OD_Convert_2DLineSets_To_2DDataSets( uiString& errmsg,
							   TaskRunner* taskrnr )
{
    (*localconv2dseisfn_)( errmsg, taskrnr );
}

static bool noBodyStatusFn( void )	{ return true; }
static bool noBodyConvFn(uiString&)	{ return true; }
using boolFromVoidFn = bool(*)();
using boolFromStringFn = bool(*)(uiString&);
static boolFromVoidFn localbodyconvstatusfn_ = noBodyStatusFn;
static boolFromStringFn localbodyconvfn_ = noBodyConvFn;

mGlobal(General) void setConvBody_General_Fns(boolFromVoidFn,boolFromStringFn);

void setConvBody_General_Fns( boolFromVoidFn convstatusfn,
			      boolFromStringFn convfn )
{
    localbodyconvstatusfn_ = convstatusfn;
    localbodyconvfn_ = convfn;
}

mGlobal(General) bool OD_Get_Body_Conversion_Status()
{
    return (*localbodyconvstatusfn_)();
}

mGlobal(General) bool OD_Convert_Body_To_OD5( uiString& msg )
{
    return (*localbodyconvfn_)( msg );
}
