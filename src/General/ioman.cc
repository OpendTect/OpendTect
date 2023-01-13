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
#include "survinfo.h"
#include "timefun.h"
#include "transl.h"
#include "uistrings.h"

extern "C" { mGlobal(Basic) void SetCurBaseDataDirOverrule(const char*); }

class IOMManager : public CallBacker
{
public:

    IOMManager()
	: iom_(*new IOMan())
    {
	mAttachCB( iom_.applicationClosing, IOMManager::closedCB );
    }

    ~IOMManager()
    {
	detachAllNotifiers();
	delete& iom_;
    }

    void init()	    { iom_.init(); }

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


static const MultiID emptykey( "-1.-1" );


IOMan& IOM()
{
    Threads::Locker locker( IOMManager::lock_ );
    if ( !theinstmgr )
    {
	theinstmgr = new IOMManager;
	theinstmgr->init();
	NotifyExitProgram( applicationClosing );
    }

    return theinstmgr->IOM();
}



IOMan::IOMan( const char* rd )
    : NamedCallBacker("IO Manager")
    , lock_(Threads::Lock(false))
    , newIODir(this)
    , entryRemoved(this)
    , entryAdded(this)
    , entryChanged(this)
    , surveyToBeChanged(this)
    , surveyChanged(this)
    , afterSurveyChange(this)
    , applicationClosing(this)
{
    rootdir_ = rd && *rd ? rd : GetDataDir();
    if ( !File::isDirectory(rootdir_) )
	rootdir_ = GetBaseDataDir();
}


void IOMan::init()
{
    state_ = Bad;
    if ( rootdir_.isEmpty() )
    {
	msg_ = tr("Survey Data Root is not set");
	return;
    }

    if ( !File::isDirectory(rootdir_) )
    {
	msg_ = tr("Survey Data Root does not exist or is not a folder:\n%1")
		.arg(rootdir_);
	return;
    }

    if ( !to(emptykey,true) )
    {
        FilePath surveyfp( GetDataDir(), ".omf" );
        if ( File::exists(surveyfp.fullPath().buf()) )
        {
	    msg_ = tr("Warning: Invalid '.omf' found in:\n%1.\n"
		    "This survey is corrupt.").arg( rootdir_ );
	    return;
        }

        FilePath basicfp( mGetSetupFileName(SurveyInfo::sKeyBasicSurveyName()),
			  ".omf" );
        File::copy( basicfp.fullPath(),surveyfp.fullPath() );
	if ( !to(emptykey,true) )
        {
	    msg_ = tr("Warning: Invalid or no '.omf' found in:\n%1.\n"
		    "This survey is corrupt.").arg( rootdir_ );
	    return;
        }
    }

    state_ = Good;
    curlvl_ = 0;

    const int nrstddirdds = IOObjContext::totalNrStdDirs();
    const IOObjContext::StdDirData* prevdd = nullptr;
    bool needwrite = false;
    FilePath rootfp( rootdir_, "X" );
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
	BufferString basicdirnm = basicfp.fullPath();
	if ( !File::exists(basicdirnm) )
	    // Oh? So this is removed from the Basic Survey
	    // Let's hope they know what they're doing
	    { prevdd = dd; continue; }

	rootfp.setFileName( dd->dirnm_ );
	BufferString dirnm = rootfp.fullPath();

#define mErrMsgRet(s) msg_ = s; ErrMsg(message()); state_ = Bad; return
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
	    else if ( !File::copy(basicdirnm,dirnm) )
	    {
		mErrMsgRet( tr("Cannot create directory: %1.\n"
			    "You probably do not have write permissions in %2.")
			    .arg(dirnm).arg(rootfp.pathOnly()) );
	    }
	}

	// So, we have copied the directory.
	// Now create an entry in the root omf
	IOSubDir* iosd = new IOSubDir( dd->dirnm_ );
	iosd->key_ = dd->id_;
	iosd->dirnm_ = rootdir_;
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

    Survey::GMAdmin().fillGeometries(nullptr);
}


void IOMan::reInit( bool dotrigger )
{
    if ( dotrigger && !IOM().isBad() )
	IOM().surveyToBeChanged.trigger();

    if ( IOM().changeSurveyBlocked() )
    {
	IOM().setChangeSurveyBlocked(false);
	return;
    }

    TranslatorGroup::clearSelHists();

    deleteAndZeroPtr( dirptr_ );
    survchgblocked_ = false;
    state_ = IOMan::NeedInit;

    rootdir_ = GetDataDir();
    if ( !File::isDirectory(rootdir_) )
	rootdir_ = GetBaseDataDir();

    init();
    if ( !IOM().isBad() )
    {
	SurveyInfo::setSurveyName( SI().getDirName() );
	setupCustomDataDirs(-1);
	if ( dotrigger )
	{
	    IOM().surveyChanged.trigger();
	    IOM().afterSurveyChange.trigger();
	}

	ResetDefaultDirs();
    }
}


IOMan::~IOMan()
{
    delete dirptr_;
}


bool IOMan::isReady() const
{
    return isBad() || !dirptr_ ? false : !dirptr_->key().isUdf();
}


bool IOMan::newSurvey( SurveyInfo* newsi )
{
    SurveyInfo::deleteInstance();
    if ( !newsi )
	SurveyInfo::setSurveyName( "" );
    else
    {
	SurveyInfo::setSurveyName( newsi->getDirName() );
	SurveyInfo::pushSI( newsi );
    }

    IOM().reInit( true );
    return !IOM().isBad();
}


bool IOMan::setSurvey( const char* survname )
{
    SurveyInfo::deleteInstance();
    SurveyInfo::setSurveyName( survname );

    IOM().reInit( true );
    return !IOM().isBad();
}


void IOMan::surveyParsChanged()
{
    IOM().surveyToBeChanged.trigger();
    if ( IOM().changeSurveyBlocked() )
	{ IOM().setChangeSurveyBlocked(false); return; }
    IOM().surveyChanged.trigger();
    IOM().afterSurveyChange.trigger();
}


const char* IOMan::surveyName() const
{
    return GetSurveyName();
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
        errmsg = "$DTECT_DATA="; errmsg += GetBaseDataDir(); \
        errmsg += "\nThis is not a valid OpendTect data storage directory."; \
	if ( fname ) \
	    { errmsg += "\n[Cannot find: "; errmsg += fname; errmsg += "]"; } \
        return false; \
    }

bool IOMan::validSurveySetup( BufferString& errmsg )
{
    errmsg = "";
    const BufferString basedatadir( GetBaseDataDir() );
    if ( basedatadir.isEmpty() )
	mErrRet("Please set the environment variable DTECT_DATA.")
    else if ( !File::exists(basedatadir) )
	mErrRetNotODDir(nullptr)
    else if ( !validOmf(basedatadir) )
	mErrRetNotODDir(".omf")

    const BufferString projdir = GetDataDir();
    if ( projdir != basedatadir && File::isDirectory(projdir) )
    {
	const bool noomf = !validOmf( projdir );
	const bool nosurv = File::isEmpty( FilePath(projdir).
					   add(SurveyInfo::sKeySetupFileName()).
					   fullPath() );
	if ( !noomf && !nosurv )
	{
	    if ( !IOM().isBad() )
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

    // Survey name in ~/.od/survey is invalid or absent. If there, lose it ...
    const BufferString survfname = SurveyInfo::surveyFileName();
    if ( File::exists(survfname) && !File::remove(survfname) )
    {
	errmsg.set( "The file:\n" ).add( survfname )
	    .add( "\ncontains an invalid survey.\n\nPlease remove this file" );
	return false;
    }

    SurveyInfo::setSurveyName( "" ); // force user-set of survey

    IOM().reInit( false );
    return true;
}


bool IOMan::setRootDir( const char* dirnm )
{
    Threads::Locker lock( lock_ );
    if ( !dirnm || rootdir_==dirnm ) return true;
    if ( !File::isDirectory(dirnm) ) return false;
    rootdir_ = dirnm;
    return setDir( rootdir_ );
}


bool IOMan::to( const IOSubDir* sd, bool forcereread )
{
    if ( isBad() )
    {
	if ( !to("0",true) || isBad() ) return false;
	return to( sd, true );
    }
    else if ( !forcereread )
    {
	if ( !sd && curlvl_ == 0 )
	    return true;
	else if ( dirptr_ && sd && sd->key() == dirptr_->key() )
	    return true;
    }

    const char* dirnm = sd ? sd->dirName() : rootdir_.buf();
    if ( !File::isDirectory(dirnm) )
	return false;

    return setDir( dirnm );
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
	{ ret = "ID=<"; ret += id; ret += ">"; }
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


const char* IOMan::curDirName() const
{
    return dirptr_ ? dirptr_->dirName() : (const char*)rootdir_;
}


const MultiID& IOMan::key() const
{
    return dirptr_ ? dirptr_->key() : emptykey;
}


bool IOMan::setDir( const char* dirname )
{
    Threads::Locker lock( lock_ );
    if ( !dirname ) dirname = rootdir_;
    if ( !dirname || !*dirname )
	return false;

    IODir* newdirptr = new IODir( dirname );
    if ( !newdirptr ) return false;
    if ( newdirptr->isBad() )
    {
	delete newdirptr;
	return false;
    }

    bool needtrigger = dirptr_;
    delete dirptr_;
    dirptr_ = newdirptr;
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
			  "is empty." );
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
    if ( !dirnm ) return 0;

    int lendir = StringView(dirnm).size();
    int lenrootdir = rootdir_.size();
    if ( lendir <= lenrootdir ) return 0;

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

    return dirptr_ ? dirptr_->commitChanges( clone ) : false;
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
    IOSubDir* sd = new IOSubDir( cdd.dirname_ );
    sd->setDirName( IOM().rootDir() );
    sd->setKey( cdd.selkey_ );
    sd->isbad_ = false;
    return sd;
}


bool IOMan::isValidDataRoot( const char* d )
{
    FilePath fp( d ? d : GetBaseDataDir() );
    const BufferString dirnm( fp.fullPath() );
    if ( !File::isDirectory(dirnm) || !File::isWritable(dirnm) )
	return false;

    fp.add( ".omf" );
    if ( !File::exists(fp.fullPath()) )
	return false;

    fp.setFileName( ".survey" );
    if ( File::exists(fp.fullPath()) )
	ErrMsg( "Warning: .survey file found in Data Root");

    IODir datarootdir( fp.pathOnly().buf() );
    return !datarootdir.isBad() && !datarootdir.isEmpty() &&
	datarootdir.get("Appl dir","Appl");
}


bool IOMan::prepareDataRoot( const char* dirnm )
{
    const BufferString stdomf( mGetSetupFileName("omf") );
    const BufferString datarootomf = FilePath( dirnm ).add( ".omf" ).fullPath();
    return File::copy( stdomf, datarootomf ) && isValidDataRoot( dirnm );
}


bool IOMan::isValidSurveyDir( const char* d )
{
    FilePath fp( d );
    fp.add( ".omf" );
    if ( !File::exists(fp.fullPath()) )
	return false;

    fp.setFileName( ".survey" );
    if ( !File::exists(fp.fullPath()) )
	return false;

    fp.setFileName( "Seismics" );
    if ( !File::isDirectory(fp.fullPath()) )
	return false;

    return true;
}


uiRetVal IOMan::setDataSource( const char* dataroot, const char* survdir,
			       bool refresh )
{
    uiRetVal uirv;
    bool res = setRootDir( dataroot );
    if ( res )
	res = setSurvey( survdir );

    if ( !res )
	uirv.set( toUiString("Can not set DataRoot and Survey") );

    return uirv;
}


uiRetVal IOMan::setDataSource( const char* fullpath, bool refresh )
{
    FilePath fp( fullpath );
    const BufferString pathnm( fp.pathOnly() );
    const BufferString filenm( fp.fileName() );
    return setDataSource( pathnm, filenm, refresh );
}


uiRetVal IOMan::setDataSource( const IOPar& iop, bool refresh )
{
    return setDataSource( iop.find(sKey::DataRoot()), iop.find(sKey::Survey()),
			  refresh );
}


uiRetVal IOMan::setDataSource( const CommandLineParser& clp, bool refresh )
{
    bool usecur = true;
    const BufferString newpath = clp.getFullSurveyPath( &usecur );
    return setDataSource( newpath, refresh );
}


BufferString IOMan::fullSurveyPath() const
{
    FilePath fp( rootDir(), surveyName() );
    return fp.fullPath();
}


bool IOMan::recordDataSource( const SurveyDiskLocation& sdl,
			      uiRetVal& uirv ) const
{
    const bool survok = writeSettingsSurveyFile( sdl.dirName(), uirv );
    const bool datadirok = SetSettingsDataDir( sdl.basePath(), uirv );
    return survok && datadirok;
}


bool IOMan::writeSettingsSurveyFile( const char* surveydirnm,
				     uiRetVal& uirv ) const
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
    const BufferString tmpdataroot = FilePath::getTempFullPath( nullptr,
								nullptr );
    if ( !File::createDir( tmpdataroot ) )
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
    const uiRetVal uirv = IOM().setDataSource( fullpath, refresh );
    ret = uirv.getText();
    return ret.buf();
}


void IOMan::setTempSurvey( const SurveyDiskLocation& sdl )
{
    SetCurBaseDataDirOverrule( sdl.basePath() );
    SurveyInfo::setSurveyName( sdl.dirName() );
    IOM().setRootDir( GetDataDir() );
}


void IOMan::cancelTempSurvey()
{
    SetCurBaseDataDirOverrule( "" );
    SurveyInfo::setSurveyName( "" );
    IOM().setRootDir( GetDataDir() );
}


// SurveyChanger
SurveyChanger::SurveyChanger( const SurveyDiskLocation& sdl )
{
    needscleanup_ = false;
    if ( sdl.isCurrentSurvey() )
	return;

    SurveyDiskLocation cursdl = changedToSurvey();
    if ( cursdl != sdl )
    {
	IOMan::setTempSurvey( sdl );
	needscleanup_ = true;
    }
}


SurveyChanger::~SurveyChanger()
{
    if ( needscleanup_ )
	IOMan::cancelTempSurvey();
}


bool SurveyChanger::hasChanged()
{
    return !changedToSurvey().isCurrentSurvey();
}


SurveyDiskLocation SurveyChanger::changedToSurvey()
{
    const FilePath fp( GetDataDir() );
    return SurveyDiskLocation( fp );
}
