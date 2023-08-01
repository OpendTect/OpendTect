/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ioman.h"

#include "applicationdata.h"
#include "ascstream.h"
#include "commandlineparser.h"
#include "compoundkey.h"
#include "ctxtioobj.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "iodir.h"
#include "iopar.h"
#include "iostrm.h"
#include "iosubdir.h"
#include "keystrs.h"
#include "msgh.h"
#include "oddirs.h"
#include "perthreadrepos.h"
#include "safefileio.h"
#include "separstr.h"
#include "settings.h"
#include "strmprov.h"
#include "surveydisklocation.h"
#include "surveyfile.h"
#include "survinfo.h"
#include "timefun.h"
#include "transl.h"
#include "uistrings.h"

#include "hiddenparam.h"

extern "C" { mGlobal(Basic) void SetCurBaseDataDir(const char*); }

static HiddenParam<IOMan,SurveyDiskLocation*> prevrootdiriommgr_(nullptr);
static HiddenParam<IOMan,CNotifier<IOMan,const MultiID&>*>
							impUpdatedNotf(nullptr);
static HiddenParam<IOMan,Notifier<IOMan>*> prepSurvChgNotif(nullptr);

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
	SurveyInfo* si = SurveyInfo::readDirectory( iom_.rootDir() );
	iom_.init( si );
	if ( iom_.isBad() )
	    uirv = iom_.uiMessage();
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
	prevrootdiriommgr_.removeAndDeleteParam( &iom_ );
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


static const MultiID emptykey( "-1.-1" );


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
		       "IOMan::setDataSource_ is called" );
	NotifyExitProgram( applicationClosing );
    }

    return theinstmgr->IOM();
}



IOMan::IOMan( const FilePath& rootdir )
    : NamedCallBacker("IO Manager")
    , rootdir_(rootdir.fullPath())
    , lock_(false)
    , newIODir(this)
    , entryRemoved(this)
    , entryAdded(this)
    , entryChanged(this)
    , surveyToBeChanged(this)
    , surveyChanged(this)
    , afterSurveyChange(this)
    , applicationClosing(this)
    , curlvl_(-1)
{
    impUpdatedNotf.setParam( this, new CNotifier<IOMan,const MultiID&>(this) );
    prepSurvChgNotif.setParam( this, new Notifier<IOMan>(this) );
    prevrootdiriommgr_.setParam( this, nullptr );
    SetCurBaseDataDir( rootdir.pathOnly().buf() );
    SurveyInfo::deleteInstance();
    SurveyInfo::setSurveyName( rootdir.baseName().buf() );
}


IOMan::IOMan( const char* rd )
    : NamedCallBacker("IO Manager")
    , lock_(false)
    , newIODir(this)
    , entryRemoved(this)
    , entryAdded(this)
    , entryChanged(this)
    , surveyToBeChanged(this)
    , surveyChanged(this)
    , afterSurveyChange(this)
    , applicationClosing(this)
    , curlvl_(-1)
{
    impUpdatedNotf.setParam( this, new CNotifier<IOMan,const MultiID&>(this) );
    prepSurvChgNotif.setParam( this, new Notifier<IOMan>(this) );
    prevrootdiriommgr_.setParam( this, nullptr );
    rootdir_ = rd && *rd ? rd : GetDataDir();
    if ( !File::isDirectory(rootdir_) )
	rootdir_ = GetBaseDataDir();
}


CNotifier<IOMan,const MultiID&>& IOMan::implUpdated()
{
    return *impUpdatedNotf.getParam( this );
}


Notifier<IOMan>& IOMan::prepareSurveyChange()
{
    return *prepSurvChgNotif.getParam( this );
}


void IOMan::init()
{
    init( nullptr );
}


void IOMan::init( SurveyInfo* nwsi )
{
    PtrMan<SurveyInfo> newsi = nwsi;
    const FilePath rootdir( rootdir_ );
    const SurveyDiskLocation rootdirsdl( rootdir );
    state_ = Bad;
    if ( rootdirsdl.basePath().isEmpty() || rootdirsdl.dirName().isEmpty() )
	{ msg_ = tr("Survey Data Root is not set"); return; }

    const BufferString rootdirnm( rootdir.fullPath() );
    if ( !File::isDirectory(rootdirnm.str()) )
    {
	msg_ = tr("Survey Data Root does not exist or is not a folder:\n%1")
		.arg(rootdirnm.str());
	return;
    }

    if ( !to(emptykey,true) )
    {
	FilePath surveyfp( rootdirnm.str(), ".omf" );
	if ( surveyfp.exists() )
        {
	    msg_ = tr("Warning: Invalid '.omf' found in:\n%1.\n"
		    "This survey is corrupt.").arg( rootdirnm.str() );
	    return;
        }

        FilePath basicfp( mGetSetupFileName(SurveyInfo::sKeyBasicSurveyName()),
			  ".omf" );
        File::copy( basicfp.fullPath(),surveyfp.fullPath() );
	if ( !to(emptykey,true) )
        {
	    msg_ = tr("Warning: Invalid or no '.omf' found in:\n%1.\n"
		    "This survey is corrupt.").arg( rootdirnm.str() );
	    return;
        }
    }

    state_ = Good;
    curlvl_ = 0;
    SetCurBaseDataDir( rootdirsdl.basePath().str() );
    SurveyInfo::setSurveyName( rootdirsdl.dirName().str() );
    if ( newsi )
    {
	SurveyInfo::deleteInstance();
	SurveyInfo::pushSI( newsi.release() );
    }

    const int nrstddirdds = IOObjContext::totalNrStdDirs();
    const IOObjContext::StdDirData* prevdd = nullptr;
    bool needwrite = false;
    FilePath rootfp( rootdirnm.str(), "X" );
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
	FilePath basicfp( mGetSetupFileName(SurveyInfo::sKeyBasicSurveyName()),
			  "X" );
	basicfp.setFileName( dd->dirnm_ );
	if ( !basicfp.exists() )
	    // Oh? So this is removed from the Basic Survey
	    // Let's hope they know what they're doing
	    { prevdd = dd; continue; }

	rootfp.setFileName( dd->dirnm_ );
	BufferString dirnm = rootfp.fullPath();

#define mErrMsgRet(s) msg_ = s; ErrMsg(uiMessage()); state_ = Bad; return
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
	to( emptykey, true );
    }

    Survey::GMAdmin().fillGeometries( nullptr );
}


bool IOMan::isOK()
{
    Threads::Locker locker( IOMManager::lock_ );
    return theinstmgr.ptr() && !IOM().isBad();
}


bool IOMan::close( bool dotrigger )
{
    if ( dotrigger && !isBad() )
	prepareSurveyChange().trigger();

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


void IOMan::reInit( SurveyInfo* nwsi )
{
    PtrMan<SurveyInfo> newsi = nwsi;
    const bool dotrigger = newsi;
    if ( !close(dotrigger) ) //Still notifying using previous survey
	return;

    SurveyDiskLocation rootloc( rootdir_ );
    rootloc.setDirName( newsi->getDirName() );
    rootdir_ = rootloc.fullPath();
    init( newsi.release() );
    if ( !isBad() )
    {
	setupCustomDataDirs(-1);
	if ( dotrigger )
	{
	    surveyChanged.trigger();
	    afterSurveyChange.trigger();
	}

	ResetDefaultDirs();
    }
}


IOMan::~IOMan()
{
    delete dirptr_;
    if ( prevrootdiriommgr_.hasParam(this) )
	prevrootdiriommgr_.removeAndDeleteParam( this );

    if (impUpdatedNotf.hasParam(this) )
	impUpdatedNotf.removeAndDeleteParam( this );

    if (prepSurvChgNotif.hasParam(this) )
	prepSurvChgNotif.removeAndDeleteParam( this );
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


bool IOMan::newSurvey( SurveyInfo* nwsi )
{
    PtrMan<SurveyInfo> newsi = nwsi;
    if ( !newsi )
    {
	SurveyInfo::deleteInstance();
	SurveyInfo::setSurveyName( "" );
	IOM().close( true );
	return true;
    }

    const FilePath rootdir( IOM().rootDir() );
    SurveyDiskLocation rootdirsdl( rootdir );
    if ( IOMan::isOK() && newsi->getDataDirName() != rootdirsdl.basePath() )
	pFreeFnErrMsg("Incorrect switching to another data root");

    IOM().reInit( newsi.release() );
    return !IOM().isBad();
}


bool IOMan::setSurvey( const char* survname )
{
    Threads::Locker locker( IOMManager::lock_ );
    if ( !theinstmgr )
	{ pFreeFnErrMsg("Data root has not been set"); }

    locker.unlockNow();

    const FilePath rootdir( IOM().rootDir() );
    const SurveyDiskLocation& rootdirsdl( rootdir );
    if ( !IOM().isBad() && rootdirsdl.dirName() == survname )
	return true;

    const FilePath fp( rootdirsdl.basePath(), survname );
    if ( !isValidSurveyDir(fp.fullPath()) )
	return false;

    return IOM().setRootDir( fp, true ).isOK();
}


void IOMan::surveyParsChanged()
{
    IOM().prepareSurveyChange().trigger();
    if ( IOM().changeSurveyBlocked() )
    {
	IOM().setChangeSurveyBlocked(false);
	return;
    }

    IOM().surveyToBeChanged.trigger();
    IOM().surveyChanged.trigger();
    IOM().afterSurveyChange.trigger();
}


const char* IOMan::surveyName() const
{
    mDeclStaticString(ret);
    const FilePath rootdir( rootdir_ );
    const SurveyDiskLocation rootdirsdl( rootdir );
    ret = rootdirsdl.dirName();
    return ret.buf();
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
	mErrRetNotODDir(nullptr)
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

    IOM().reInit( nullptr );
    const bool isok = !IOM().isBad();
    if ( !isok )
	errmsg = IOM().message();

    return isok;
}


bool IOMan::setRootDir( const char* rootdirstr )
{
    Threads::Locker lock( lock_ );
    FilePath dirfp;
    bool ischecked = false;
    if ( rootdirstr && *rootdirstr )
	dirfp.set( rootdirstr );
    else
    {
	dirfp.set( GetDataDir() );
	ischecked = true;
    }

    return setRootDir( dirfp, ischecked ).isOK();
}


static uiRetVal lastiomsmsg_;

uiRetVal IOMan::setRootDir( const FilePath& dirfp, bool ischecked )
{
    Threads::Locker lock( lock_ );
    const BufferString dirnm = dirfp.fullPath();
    if ( dirnm.isEmpty() )
	return tr( "Cannot set IOM Root Dir with empty path" );

    FilePath rootdir( rootdir_ );
    uiRetVal uirv;
    if ( dirnm == rootdir.fullPath() )
	return uirv;

    if ( !ischecked && !isValidSurveyDir(dirnm.str()) )
    {
	uirv = lastiomsmsg_;
	return uirv;
    }

    rootdir_.set( dirnm.str() );
    SurveyInfo* newsi = SurveyInfo::readDirectory( dirnm.str() );
    reInit( newsi );
    uirv = msg_;
    if ( !uirv.isOK() )
	return uirv;

    if ( !setDir(nullptr) )
	uirv = msg_;

    return uirv;
}


bool IOMan::to( const IOSubDir* sd, bool forcereread )
{
    if ( isBad() )
    {
	if ( !to("0",true) || isBad() )
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

    const BufferString dirnm( sd ? sd->dirName() : rootdir_.buf() );
    if ( !File::isDirectory(dirnm.buf()) )
	return false;

    return setDir( dirnm.buf() );
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
    if ( !File::isDirectory(rootdir_.buf()) || (!forcereread && isBad()) )
	return false;

    MultiID key = ky;
    if ( key.groupID()>100000 && key.objectID()<0 )
	key.setGroupID(-1).setObjectID(ky.groupID());

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

    auto* newdir = dirkey.isUdf() ? new IODir( rootdir_.buf() )
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


IOObj* IOMan::getOfGroup( const char* tgname, bool first,
			  bool onlyifsingle ) const
{
    Threads::Locker lock( lock_ );
    if ( isBad() || !tgname || !dirptr_ ) return nullptr;

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
	if ( ptr ) *ptr = '\0';
	return get( MultiID(oky.buf()) );
    }

    if ( dirptr_ )
    {
	const IOObj* ioobj = dirptr_->get( objname, trgrpnm );
	if ( ioobj ) return ioobj->clone();
    }

    if ( IOObj::isKey(objname) )
	return get( MultiID(objname) );

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
    if ( !basekey.isEmpty() ) basekey.add( "." );
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

    IOObj* ioobj = get( MultiID(res.buf()) );
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

    MultiID ky( id );
    IOObj* ioobj = get( ky );
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


const char* IOMan::curDirName() const
{
    return dirptr_ ? dirptr_->dirName() : rootdir_.buf();
}


const MultiID& IOMan::key() const
{
    return dirptr_ ? dirptr_->key() : emptykey;
}


bool IOMan::setDir( const char* dirname )
{
    Threads::Locker lock( lock_ );
    BufferString dirnm( dirname );
    if ( dirnm.isEmpty() )
	dirnm.set( rootdir_.buf() );

    if ( dirnm.isEmpty() )
    {
	msg_ = tr("Cannot set directory from empty string");
	return false;
    }

    PtrMan<IODir> newdirptr = new IODir( dirnm.str() );
    if ( !newdirptr )
    {
	msg_ = tr("Cannot switch to database directory '%1'").arg( dirname );
	return false;
    }

    if ( newdirptr->isBad() )
    {
	msg_ = tr("Cannot switch to database directory '%1'").arg( dirname );
	return false;
    }

    bool needtrigger = dirptr_;
    delete dirptr_;
    dirptr_ = newdirptr.release();
    curlvl_ = levelOf( curDirName() );

    lock.unlockNow();

    if ( needtrigger )
	newIODir.trigger();

    return true;
}


void IOMan::getEntry( CtxtIOObj& ctio, bool mktmp, int translidx )
{
    getObjEntry( ctio, false, mktmp, translidx );
}


void IOMan::getNewEntry( CtxtIOObj& ctio, bool mktmp, int translidx )
{
    getObjEntry( ctio, true, mktmp, translidx );
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

    const IOObj* ioobj = dirptr_->get( ctio.ctxt_.name(),
					ctio.ctxt_.trgroup_->groupName() );
    ctio.ctxt_.fillTrGroup();
    if ( ioobj && ctio.ctxt_.trgroup_->groupName() != ioobj->group() )
	ioobj = nullptr;

    bool needstrigger = false;
    if ( !ioobj || (isnew && !mktmp) )
    {
	MultiID newkey( mktmp ? ctio.ctxt_.getSelKey() : dirptr_->newKey() );
	if ( mktmp )
	    newkey.setObjectID( IOObj::tmpID() );

	ioobj = crWriteIOObj( ctio, newkey, translidx );
	if ( ioobj )
	{
	    ioobj->pars().merge( ctio.ctxt_.toselect_.require_ );
	    if ( !dirptr_->addObj((IOObj*)ioobj) )
		return;

	    needstrigger = true;
	}
    }

    ctio.setObj( ioobj ? ioobj->clone() : nullptr );
    lock.unlockNow();

    if ( needstrigger )
    {
	const MultiID mid = ioobj->key();
	entryAdded.trigger( mid );
    }
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
	pErrMsg( msg ); return nullptr;
    }

    const Translator* templtr = nullptr;

    if ( templs.validIdx(translidx) )
	templtr = ctio.ctxt_.trgroup_->templates()[translidx];
    else if ( !ctio.ctxt_.deftransl_.isEmpty() )
	templtr = ctio.ctxt_.trgroup_->getTemplate(ctio.ctxt_.deftransl_,true);

    if ( !templtr )
    {
	translidx = ctio.ctxt_.trgroup_->defTranslIdx();
	if ( (translidx>=0) && mIsValidTransl(templs[translidx]) )
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
    return trans && trans->implIsLink( ioobj );
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
    if ( !iostrm || trans->implIsLink(ioobj) )
	return IOM().commitChanges( *ioobj );

    if ( !iostrm->implExists(true) )
    {
	iostrm->genFileName();
	return IOM().commitChanges( *ioobj );
    }

    IOStream chiostrm;
    chiostrm.copyFrom( iostrm );
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
    if ( newfnm && !doReloc(key,trans,*iostrm,chiostrm) )
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
	if ( !doReloc(key,trans,*iostrm,chiostrm) )
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
    mDynamicCastGet(IOStream*,iostrm,ioobj.ptr())
    BufferString oldfnm( iostrm->fullUserExpr() );
    IOStream chiostrm;
    chiostrm.copyFrom( iostrm );
    if ( !File::isDirectory(newdir) )
	return false;

    FilePath fp( oldfnm ); fp.setPath( newdir );
    chiostrm.fileSpec().setFileName( fp.fullPath() );
    if ( !doReloc(key,trans,*iostrm,chiostrm) )
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
	    *uirv = tr("Could not delete data file(s).");

	return false;
    }

    if ( rmentry && !IOM().permRemove(key) )
	return false;

    return true;
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
    return obj != nullptr;
}


bool IOMan::isPresent( const char* objname, const char* tgname ) const
{
    PtrMan<IOObj> obj = get( objname, tgname );
    return obj != nullptr;
}


int IOMan::levelOf( const char* dirnm ) const
{
    Threads::Locker lock( lock_ );
    if ( !dirnm )
	return 0;

    int lendir = StringView( dirnm ).size();
    int lenrootdir = rootdir_.size();
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

    return dirptr_ ? dirptr_->commitChanges(clone) : false;
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

    const CompoundKey defaultkey( trl->group()->getSurveyDefaultKey(ioobj) );
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
	mDefineStaticLocalObject( MultiID, none, ("") );
	return none;
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
    sd->setDirName( IOM().rootDir() );
    sd->setKey( cdd.selkey_ );
    sd->isbad_ = false;
    return sd;
}


bool IOMan::isValidDataRoot( const char* basedatadir )
{
    Threads::Locker locker( IOMManager::lock_ );
    uiRetVal& uirv = lastiomsmsg_.setOK();
    uirv = SurveyInfo::isValidDataRoot( basedatadir );
    if ( !uirv.isOK() )
	return false;

    const IODir datarootdir( basedatadir );
    if ( datarootdir.isEmpty() )
	uirv = tr("Data root folder '%1' is empty").arg( basedatadir );
    else if ( datarootdir.isBad() || !datarootdir.get("Appl dir","Appl") )
	uirv = tr("Data root folder '%1' seems corrupted").arg( basedatadir );

    return uirv.isOK();
}


bool IOMan::prepareDataRoot( const char* dirnm )
{
    const BufferString stdomf( mGetSetupFileName("omf") );
    const BufferString datarootomf = FilePath( dirnm ).add( ".omf" ).fullPath();
    if ( !File::copy(stdomf,datarootomf) )
       return false;

    Threads::Locker locker( IOMManager::lock_ );
    lastiomsmsg_.setOK();
    return isValidDataRoot( dirnm );
}


bool IOMan::isValidSurveyDir( const char* surveydir )
{
    Threads::Locker locker( IOMManager::lock_ );
    uiRetVal& uirv  = lastiomsmsg_.setOK();
    uirv = SurveyInfo::isValidSurveyDir( surveydir );
    return uirv.isOK();
}


uiRetVal IOMan::isValidMsg()
{
    Threads::Locker locker( IOMManager::lock_ );
    return lastiomsmsg_;
}


uiRetVal IOMan::setDataSource( const char* dataroot, const char* survdir,
			       bool refresh )
{
    return setDataSource_( dataroot, survdir, refresh );
}


uiRetVal IOMan::setDataSource_( const char* dataroot, const char* survdir,
			       bool /* refresh */ )
{
    Threads::Locker locker( IOMManager::lock_ );
    uiRetVal& uirv = lastiomsmsg_.setOK();
    if ( isValidDataRoot(dataroot) )
	SetCurBaseDataDir( dataroot );
    else
	return uirv;

    const SurveyDiskLocation sdl( survdir, dataroot );
    const BufferString iomrootdir = sdl.fullPath();
    if ( !isValidSurveyDir(iomrootdir.buf()) )
	return uirv;

    const FilePath rootdirfp( iomrootdir.buf() );
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
    return setDataSource_( fullpath, refresh );
}


uiRetVal IOMan::setDataSource_( const char* fullpath, bool refresh )
{
    const FilePath fp( fullpath );
    return setDataSource_( fp.pathOnly().buf(), fp.fileName().buf(), refresh );
}


uiRetVal IOMan::setDataSource( const IOPar& iop, bool refresh )
{
    BufferString dataroot, survdir;
    if ( !iop.get(sKey::DataRoot(),dataroot) )
	dataroot.set( GetBaseDataDir() );

    if ( !iop.get(sKey::Survey(),survdir) )
	survdir.set( SurveyInfo::curSurveyName() );

    return setDataSource_( dataroot.buf(), survdir.buf(), refresh );
}


uiRetVal IOMan::setDataSource( const CommandLineParser& clp, bool refresh )
{
    return setDataSource_( clp, refresh );
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


uiRetVal IOMan::setDataSource_( const CommandLineParser& clp, bool refresh )
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

    return setDataSource_( dataroot.buf(), survdir.buf(), refresh );
}


BufferString IOMan::fullSurveyPath() const
{
    FilePath fp( rootDir(), surveyName() );
    return fp.fullPath();
}


bool IOMan::recordDataSource( const SurveyDiskLocation& sdl,
			      uiRetVal& uirv ) const
{
    return recordDataSource_( sdl, uirv );
}


bool IOMan::recordDataSource_( const SurveyDiskLocation& sdl, uiRetVal& uirv )
{
    const bool survok = writeSettingsSurveyFile_( sdl.dirName(), uirv );
    const bool datadirok = SetSettingsDataDir( sdl.basePath(), uirv );
    return survok && datadirok;
}


bool IOMan::writeSettingsSurveyFile( const char* surveydirnm,
				     uiRetVal& uirv ) const
{
    return writeSettingsSurveyFile_( surveydirnm, uirv );
}


bool IOMan::writeSettingsSurveyFile_( const char* surveydirnm, uiRetVal& uirv )
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
	const FilePath fp( rootDir(), dd->dirnm_ );
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

    const FilePath fp( rootDir(), dd->dirnm_ );
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
	    const FilePath fp( rootDir(), dd->dirnm_ );
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
    const uiRetVal uirv = IOMan::setDataSource_( fullpath, refresh );
    ret = uirv.getText();
    return ret.buf();
}


bool IOMan::isUsingTempSurvey() const
{
    return prevrootdiriommgr_.getParam( this );
}


void IOMan::setTempSurvey( const SurveyDiskLocation& sdl )
{
    IOM().setTempSurvey_( sdl );
}


uiRetVal IOMan::setTempSurvey_( const SurveyDiskLocation& sdl )
{
    Threads::Locker locker( IOMManager::lock_ );
    uiRetVal& uirv = lastiomsmsg_.setOK();

    const BufferString dataroot( sdl.basePath() );
    if ( !isValidDataRoot(dataroot) )
	return uirv;

    const BufferString iomrootdir( sdl.fullPath() );
    if ( !isValidSurveyDir(iomrootdir.buf()) )
	return uirv;

    prevrootdiriommgr_.deleteAndNullPtrParam( this );
    prevrootdiriommgr_.setParam( this, new SurveyDiskLocation( rootdir_ ) );
    const FilePath fp( iomrootdir.buf() );
    return setRootDir( fp, true );
}


void IOMan::cancelTempSurvey()
{
    IOM().cancelTempSurvey_();
}


uiRetVal IOMan::cancelTempSurvey_()
{
    uiRetVal uirv;
    if ( !prevrootdiriommgr_.getParam(this) )
    {
	uirv = tr("Cannot cancel temp survey: No valid project before");
	return uirv;
    }

    const FilePath fp( prevrootdiriommgr_.getParam(this)->fullPath() );
    uirv = setRootDir( fp );
    prevrootdiriommgr_.deleteAndNullPtrParam( this );
    return uirv;
}


HiddenParam<SurveyChanger,uiRetVal*> surveychangermsgmgr_(nullptr);

// SurveyChanger
SurveyChanger::SurveyChanger( const SurveyDiskLocation& sdl )
{
    surveychangermsgmgr_.setParam( this, new uiRetVal );
    if ( sdl.isCurrentSurvey() )
	return;

    const FilePath curfp( IOM().rootDir() );
    const SurveyDiskLocation cursdl( curfp );
    if ( cursdl != sdl )
	*surveychangermsgmgr_.getParam( this ) = IOM().setTempSurvey_( sdl );
}


SurveyChanger::~SurveyChanger()
{
    surveychangermsgmgr_.removeAndDeleteParam( this );
    if ( hasChanged() )
	IOM().cancelTempSurvey_();
}


bool SurveyChanger::hasChanged()
{
    return IOM().isUsingTempSurvey();
}


uiRetVal SurveyChanger::message() const
{
    return *surveychangermsgmgr_.getParam( this );
}


SurveyDiskLocation SurveyChanger::changedToSurvey()
{
    const FilePath fp( IOM().rootDir() );
    return SurveyDiskLocation( fp );
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
