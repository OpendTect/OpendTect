/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 3-8-1994
-*/


#include "ioman.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "iodir.h"
#include "iopar.h"
#include "iostrm.h"
#include "iosubdir.h"
#include "keystrs.h"
#include "oddirs.h"
#include "od_ostream.h"
#include "safefileio.h"
#include "separstr.h"
#include "settings.h"
#include "perthreadrepos.h"
#include "strmprov.h"
#include "survinfo.h"
#include "timefun.h"
#include "transl.h"
#include "uistrings.h"


IOMan* IOMan::theinst_	= 0;
#define mToRootDirID DirID::getInvalid()


IOMan& IOM()
{
    if ( !IOMan::theinst_ )
	{ IOMan::theinst_ = new IOMan; IOMan::theinst_->init(); }
    return *IOMan::theinst_;
}

IOMan::IOMan( const char* rd )
	: NamedMonitorable("IO Manager")
	, dirptr_(0)
	, survchgblocked_(false)
	, state_(IOMan::NeedInit)
	, newIODir(this)
	, surveyToBeChanged(this)
	, surveyChanged(this)
	, afterSurveyChange(this)
	, applicationClosing(this)
	, entryRemoved(this)
	, entryAdded(this)
{
    rootdir_ = rd && *rd ? rd : GetDataDir();
    if ( !File::isDirectory(rootdir_) )
	rootdir_ = GetBaseDataDir();
}


void IOMan::init()
{
    state_ = Bad;
    errmsg_.setEmpty();
    if ( rootdir_.isEmpty() )
    {
	errmsg_ = tr( "Directory for data storage is not set" );
	return;
    }

    if ( !to(mToRootDirID,true) )
    {
        FilePath surveyfp( GetDataDir(), ".omf" );
	if ( !File::exists(surveyfp.fullPath().buf()) )
	{
	    errmsg_.append( tr("Trying to restoring default .omf file:"), true);
	    FilePath basicfp(
		mGetSetupFileName(SurveyInfo::sKeyBasicSurveyName()), ".omf" );
	    uiString cperrmsg;
	    if ( !File::copy(basicfp.fullPath(),surveyfp.fullPath(),&cperrmsg) )
	    {
		errmsg_.append( cperrmsg, true );
		return;
	    }

	    if ( !to(mToRootDirID,true) )
		return;
	}
	else if ( !File::isReadable(surveyfp.fullPath().buf()) )
	    return;
	else if ( File::exists(surveyfp.fullPath().buf()) &&
		  File::isReadable(surveyfp.fullPath().buf()) )
        {
	    errmsg_.append( tr( "Invalid '.omf': You may try to (re)move it to "
				"force its regeneration on next start" ), true);
	    return;
        }
    }

    state_ = Good;
    curlvl_ = 0;
    bool needwrite = false;

    int nrstddirdds = IOObjContext::totalNrStdDirs();
    const IOObjContext::StdDirData* prevdd = 0;
    const bool needsurvtype = !SI().survdatatypeknown_;
    FilePath rootfp( rootdir_, "X" );
    for ( int idx=0; idx<nrstddirdds; idx++ )
    {
	IOObjContext::StdSelType stdseltyp = (IOObjContext::StdSelType)idx;
	const IOObjContext::StdDirData* dd
			    = IOObjContext::getStdDirData( stdseltyp );
	PtrMan<IOObj> dirioobj = dirptr_->getEntry( DBKey(dd->id_) );
	if ( dirioobj )
	{
	    if ( needsurvtype && stdseltyp == IOObjContext::Seis )
	    {
		IODir seisiodir( dirioobj->key().dirID() );
		bool has2d = false, has3d = false;
		const BufferString seisstr( "Seismic Data" );
		const BufferString tr2dstr( "2D" );
		const BufferString trsegystr( "SEG-Y" );
		IODirIter iter( seisiodir );
		while ( iter.next() )
		{
		    const IOObj& subioobj = iter.ioObj();
		    if ( seisstr != subioobj.group() ||
			 trsegystr == subioobj.translator() ) continue;

		    const bool is2d = tr2dstr == subioobj.translator();
		    if ( is2d ) has2d = true;
		    else	has3d = true;
		    if ( has2d && has3d ) break;
		}
		SurveyInfo& si( const_cast<SurveyInfo&>(SI()) );
		si.survdatatypeknown_ = true;
		si.survdatatype_ = !has2d ? SurveyInfo::No2D
					    // thus also if nothing found
				 : (has3d ? SurveyInfo::Both2DAnd3D
					  : SurveyInfo::Only2D);
		si.write();
	    }

	    prevdd = dd; continue;
	}

	// Oops, a data directory required is missing
	// We'll try to recover by using the 'Basic Survey' in the app
	FilePath basicfp(
		mGetSetupFileName(SurveyInfo::sKeyBasicSurveyName()), "X" );
	basicfp.setFileName( dd->dirnm_ );
	BufferString basicdirnm = basicfp.fullPath();
	if ( !File::exists(basicdirnm) )
	    // Oh? So this is removed from the Basic Survey
	    // Let's hope they know what they're doing
	    { prevdd = dd; continue; }

	rootfp.setFileName( dd->dirnm_ );
	BufferString dirnm = rootfp.fullPath();

	if ( !File::exists(dirnm) )
	{
	    // This directory should have been in the survey.
	    // It is not. If it is the seismic directory, we do not want to
	    // continue. Otherwise, we want to copy the Basic Survey
	    // directory.
	    if ( stdseltyp == IOObjContext::Seis )
	    {
		errmsg_ = tr("Corrupt survey: missing directory: %1")
			    .arg( dirnm );
		state_ = Bad;
		return;
	    }
	    else if ( !File::copy(basicdirnm,dirnm) )
	    {
		errmsg_ = tr("Cannot create directory: %1.\n"
			 "You probably do not have write permissions in %2")
			.arg( dirnm )
			.arg( rootfp.pathOnly() );
		state_ = Bad;
		return;
	    }
	}

	// So, we have copied the directory.
	// Now create an entry in the root omf
	IOSubDir* iosd = new IOSubDir( dd->dirnm_ );
	iosd->key_ = dd->id_;
	iosd->dirnm_ = rootdir_;
	PtrMan<IOObj> previoobj = prevdd
			       ? dirptr_->getEntry( DBKey(prevdd->id_) )
			       : dirptr_->getEntryByIdx( 0 );
	int idxof = dirptr_->indexOf( previoobj->key() );
	dirptr_->objs_.insertAfter( iosd, idxof );

	prevdd = dd;
	needwrite = true;
    }

    if ( needwrite )
    {
	dirptr_->doWrite();
	to( mToRootDirID, true );
    }

    Survey::GMAdmin().fillGeometries(0);
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

    StreamProvider::unLoadAll();
    TranslatorGroup::clearSelHists();

    delete dirptr_; dirptr_ = 0;
    survchgblocked_ = false;
    state_ = IOMan::NeedInit;

    rootdir_ = GetDataDir();
    if ( !File::isDirectory(rootdir_) )
	rootdir_ = GetBaseDataDir();

    init();
    if ( !IOM().isBad() )
    {
	SurveyInfo::setSurveyName( SI().getDirName() );
	setupCustomDataDirs(-1,errmsg_);
	if ( dotrigger )
	{
	    IOM().surveyChanged.trigger();
	    IOM().afterSurveyChange.trigger();
	}
    }
}


IOMan::~IOMan()
{
    sendDelNotif();
    delete dirptr_;
}


bool IOMan::isBad() const
{
    mLock4Read();
    return gtIsBad();
}


bool IOMan::isReady() const
{
    mLock4Read();
    return gtIsBad() || !dirptr_ ? false : dirptr_->dirID().isValid();
}


bool IOMan::newSurvey( SurveyInfo* newsi, uiString* errmsg )
{
    SurveyInfo oldsi;
    if ( !IOM().isBad() )
	oldsi = SI();

    if ( !newsi )
	SurveyInfo::setSurveyName( "" );
    else
    {
	SurveyInfo::setSurveyName( newsi->getDirName() );
	SurveyInfo::pushSI( newsi );
    }

    IOM().reInit( true );
    if ( !IOM().isBad() )
    {
	SurveyInfo::deleteOriginal();
	return true;
    }

    if ( errmsg )
	*errmsg = IOM().errMsg();

    SurveyInfo::deleteInstance();
    SurveyInfo::setSurveyName( oldsi.getDirName() );
    IOM().reInit( true );

    return false;
}


bool IOMan::setSurvey( const char* survname, uiString* errmsg )
{
    SurveyInfo* newsi = 0;

    const FilePath surveyfp( GetBaseDataDir(), survname );
    const BufferString surveyfnm( surveyfp.fullPath() );
    if ( File::exists(surveyfnm.str()) && File::isReadable(surveyfnm.str()) )
    {
	uiString localerrmsg;
	uiString& msg = errmsg ? *errmsg : localerrmsg;
	newsi = SurveyInfo::read( surveyfnm.str(), msg );
	if ( !newsi || msg.isSet() )
	    return false;
    }

    return newSurvey( newsi, errmsg );
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


static bool validOmf( const char* dir, uiString& msg )
{
    FilePath fp( dir ); fp.add( ".omf" );
    BufferString fname = fp.fullPath();
    if ( !File::checkDirectory(fname,true,msg) )
	return false;

    SafeFileIO fstrm( fname );
    const bool success = fstrm.open( true, true );
    msg = fstrm.errMsg();
    fstrm.closeSuccess();
    if ( success )
	return true;

    fp.setFileName( ".omb" );
    BufferString fnamebak = fp.fullPath();
    SafeFileIO fstrmback( fnamebak );
    const bool cancopy = fstrmback.open( true, true );
    fstrmback.closeSuccess();
    if ( !cancopy )
	return false;

    msg = od_static_tr("IOManvalidOmf",
		       "Restoring .omf file from .omb file");
    uiString msg2;
    if ( !File::copy(fname,fnamebak,&msg2) )
    {
	msg.append( msg2, true );
	return false;
    }

    return true;
}


static bool validSurveyFile( const char* dir, uiString& msg )
{
    FilePath fp( dir ); fp.add( SurveyInfo::sKeySetupFileName() );
    BufferString fname = fp.fullPath();
    SafeFileIO fstrm( fname );
    const bool success = fstrm.open( true, true );
    msg = fstrm.errMsg();
    fstrm.closeSuccess();

    return success;
}


bool IOMan::validSurveySetup( uiString& errmsg, uiString& warnmsg )
{
    errmsg.setEmpty();
    const BufferString basedatadir( GetBaseDataDir() );
    if ( basedatadir.isEmpty() )
    {
	errmsg = tr("Please set the environment variable DTECT_DATA.");
	return false;
    }
    else if ( !File::exists(basedatadir) )
    {
	errmsg = tr("$DTECT_DATA=%1\n"
		    "This is not a valid OpendTect data storage directory.")
		   .arg( toUiString(GetBaseDataDir()) );
	return false;
    }
    else if ( !validOmf(basedatadir,errmsg) )
    {
	errmsg.append( tr("$DTECT_DATA=%1\n"
	       "This is not a valid OpendTect data storage directory.\n")
	       .arg(toUiString(GetBaseDataDir())),true);
	return false;
    }

    const BufferString projdir = GetDataDir();
    if ( projdir != basedatadir && File::isDirectory(projdir) )
    {
	const bool noomf = !validOmf( projdir, warnmsg );
	uiString warnmsg2;
	const bool nosurv = !validSurveyFile( projdir, warnmsg2 );
	if ( warnmsg2.isSet() )
	    warnmsg.append( warnmsg2, true );

	if ( !noomf && !nosurv )
	{
	    if ( !IOM().isBad() )
		return true; // This is normal

	    // But what's wrong here? In any case - survey is not good.
	}
	else
	{
	    warnmsg.append( tr("This survey cannot be used."), true );
	}
    }

    // Survey name in ~/.od/survey is invalid or absent. If there, lose it ...
    const BufferString survfname = SurveyInfo::surveyFileName();
    if ( File::exists(survfname) && !File::remove(survfname) )
    {
	errmsg = tr("The file:\n%1\ncontains an invalid survey.\n\n"
		    "Please remove this file")
		   .arg( survfname );
	return false;
    }

    SurveyInfo::setSurveyName( "" ); // force user-set of survey

    IOM().reInit( false );
    return true;
}


bool IOMan::setRootDir( const char* dirnm )
{
    mLock4Read();
    if ( !dirnm || rootdir_ == dirnm )
	return true;
    if ( !File::isDirectory(dirnm) )
	return false;

    mLock2Write();
    rootdir_ = dirnm;
    bool rv = setDir( rootdir_ );
    mSendEntireObjChgNotif();
    return rv;
}


#define mGoTo(where,forcereread) \
    goTo( where, forcereread, mAccessLockHandler() )
#define mSendNewIODirNotif() \
    mSendChgNotif( cNewIODirChangeType(), 0 ); \
    self.newIODir.trigger()


bool IOMan::goTo( const IOSubDir* sd, bool forcereread,
		    AccessLockHandler& mAccessLockHandler() ) const
{
    if ( gtIsBad() )
    {
	if ( !mGoTo(mToRootDirID,true) || gtIsBad() )
	    return false;
	return forcereread ? false : mGoTo( sd, true );
    }

    if ( !forcereread )
    {
	if ( !sd && curlvl_ == 0 )
	    return true;
	else if ( dirptr_ && sd && sd->key().dirID() == dirptr_->dirID() )
	    return true;
    }

    const char* dirnm = sd ? sd->dirName() : rootdir_.buf();
    if ( !File::isDirectory(dirnm) )
	return false;

    IOMan& self = *const_cast<IOMan*>( this );
    mLock2Write();

    const bool rv = self.setDir( dirnm );
    mSendNewIODirNotif();
    mReLock();
    return rv;
}


DBKey IOMan::createNewKey( DirID dirid )
{
    mLock4Read();
    if ( !mGoTo(dirid,true) || !dirptr_ )
	return DBKey::getInvalid();

    return dirptr_->newKey();
}


bool IOMan::to( DBKey::DirID dirid, bool forcereread )
{
    mLock4Read();
    return mGoTo( dirid, forcereread );
}


bool IOMan::to( const DBKey& ky, bool forcereread )
{
    mLock4Read();
    return mGoTo( ky.dirID(), forcereread );
}


static int gtLevelOf( const char* dirnm, int startat )
{
    if ( !dirnm )
	return 0;

    const int lendir = FixedString(dirnm).size();
    if ( lendir <= startat )
	return 0;

    int lvl = 0;
    const char* ptr = ((const char*)dirnm) + startat;
    while ( ptr )
    {
	ptr++; lvl++;
	ptr = firstOcc( ptr, *FilePath::dirSep(FilePath::Local) );
    }
    return lvl;
}


bool IOMan::goTo( DirID dirid, bool forcereread,
		    AccessLockHandler& mAccessLockHandler() ) const
{
    if ( rootdir_.isEmpty() )
	return false;

    const bool issamedir = dirptr_ && dirid == dirptr_->dirID();
    if ( !forcereread && issamedir )
	return true;

    IODir* newdir = dirid.isInvalid() ? new IODir(rootdir_) : new IODir(dirid);
    if ( !newdir || newdir->isBad() )
    {
	if ( newdir )
	    errmsg_ = newdir->errMsg();
	return false;
    }

    bool sendnotif = dirptr_;
    IOMan& self = *const_cast<IOMan*>( this );
    mAccessLockHandler().convertToWrite();
    if ( dirptr_ )
	delete self.dirptr_;
    self.dirptr_ = newdir;
    self.curlvl_ = gtLevelOf( curDirName(), rootdir_.size() );

    if ( sendnotif )
    {
	mSendNewIODirNotif();
	mReLock();
    }

    return true;
}


IOObj* IOMan::get( const DBKey& ky ) const
{
    return IODir::getObj( ky, errmsg_ );
}


IOObj* IOMan::getOfGroup( const char* tgname, bool first,
			  bool onlyifsingle ) const
{
    mLock4Read();
    if ( gtIsBad() || !tgname || !dirptr_ )
	return 0;

    IOObj* ioobj = 0;
    IODirIter iter( *dirptr_ );
    while ( iter.next() )
    {
	const IOObj& curioobj = iter.ioObj();
	if ( curioobj.group() == tgname )
	{
	    if ( onlyifsingle && ioobj )
		{ delete ioobj; return 0; }

	    ioobj = curioobj.clone();
	    if ( first && !onlyifsingle )
		break;
	}
    }
    mUnlockAllAccess();

    return ioobj;
}


IOObj* IOMan::getLocal( const char* objname, const char* trgrpnm ) const
{
    const FixedString fsobjnm( objname );
    if ( fsobjnm.isEmpty() )
	return 0;

    if ( fsobjnm.startsWith("ID=<") )
    {
	BufferString oky( objname+4 );
	char* ptr = oky.find( '>' );
	if ( ptr ) *ptr = '\0';
	return get( DBKey::getFromString(oky) );
    }

    mLock4Read();
    if ( dirptr_ )
	return dirptr_->getEntryByName( objname, trgrpnm );
    mUnlockAllAccess();

    if ( DBKey::isValidString(objname) )
	return get( DBKey::getFromString(objname) );

    return 0;
}


IOObj* IOMan::getFirst( const IOObjContext& ctxt, int* nrfound ) const
{
    if ( nrfound )
	*nrfound = 0;
    if ( !ctxt.trgroup_ )
	return 0;

    mLock4Read();
    if ( !mGoTo(ctxt.getSelDirID(),false) || !dirptr_ )
	return 0;

    IOObj* ret = 0;
    IODirIter iter( *dirptr_ );
    while ( iter.next() )
    {
	const IOObj& ioobj = iter.ioObj();
	if ( ctxt.validIOObj(ioobj) )
	{
	    if ( !ret )
		ret = ioobj.clone();
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
	    return 0;
	}

	if ( !DBKey::isValidString(res) )
	{
	    CtxtIOObj ctio( ctxt );
	    mLock4Read();
	    if ( !mGoTo(ctio.ctxt_.getSelDirID(),false) )
		return 0;
	    if ( !dirptr_ )
		return 0;
	    PtrMan<IOObj> ioob = dirptr_->getEntryByName( res.buf(), 0 );
	    mUnlockAllAccess();
	    if ( ioob )
		res = ioob->key().toString();
	    else if ( mknew )
	    {
		ctio.setName( res );
		const_cast<IOMan*>(this)->getEntry( ctio );
		if ( ctio.ioobj_ )
		{
		    res = ctio.ioobj_->key().toString();
		    ctio.setObj( 0 );
		}
	    }
	}
    }

    IOObj* ioobj = get( DBKey::getFromString(res) );
    if ( !ioobj )
	errmsg = BufferString( "Value for ", iopkey, " is invalid." );

    return ioobj;
}


bool IOMan::isKeyString( const char* kystr ) const
{
    return DBKey::isValidString( kystr );
}


BufferString IOMan::nameOf( const DBKey& id ) const
{
    BufferString ret;
    IOObj* ioobj = get( id );
    if ( !ioobj )
	{ ret = "ID=<"; ret += id; ret += ">"; }
    else
	{ ret = ioobj->name(); delete ioobj; }

    return ret;
}


BufferString IOMan::nameFor( const char* kystr ) const
{
    if ( !isKeyString(kystr) )
	return kystr;

    const DBKey id = DBKey::getFromString( kystr );
    return nameOf( id );
}


const char* IOMan::curDirName() const
{
    mLock4Read();
    return dirptr_ ? dirptr_->dirName() : (const char*)rootdir_;
}


DBKey::DirID IOMan::dirID() const
{
    mLock4Read();
    return dirptr_ ? dirptr_->dirID() : DirID::get(1);
}


bool IOMan::setDir( const char* dirname )
{
    if ( !dirname )
	dirname = rootdir_;
    if ( !dirname || !*dirname )
	return false;

    IODir* newdirptr = new IODir( dirname );
    if ( !newdirptr )
	return false;
    if ( newdirptr->isBad() )
	{ delete newdirptr; return false; }

    mLock4Write();
    bool sendnotif = dirptr_;
    delete dirptr_;
    dirptr_ = newdirptr;
    curlvl_ = gtLevelOf( curDirName(), rootdir_.size() );

    if ( sendnotif )
	{ IOMan& self = *this; mSendNewIODirNotif(); }

    return true;
}


void IOMan::getEntry( CtxtIOObj& ctio, bool mktmp, int translidx )
{
    ctio.setObj( 0 );
    if ( ctio.ctxt_.name().isEmpty() )
	return;

    mLock4Read();
    if ( !mGoTo(ctio.ctxt_.getSelDirID(),false) || !dirptr_ )
	return;

    ctio.ctxt_.fillTrGroup();
    PtrMan<IOObj> ioobj = dirptr_->getEntryByName( ctio.ctxt_.name(),
					ctio.ctxt_.translatorGroupName() );
    if ( ioobj && ctio.ctxt_.translatorGroupName() != ioobj->group() )
	ioobj = 0;

    bool needstrigger = false;
    if ( !ioobj )
    {
	const DBKey newky( mktmp ? dirptr_->newTmpKey() : dirptr_->newKey() );
	ioobj = crWriteIOObj( ctio, newky, translidx );
	if ( ioobj )
	{
	    ioobj->pars().merge( ctio.ctxt_.toselect_.require_ );
	    dirptr_->addObj( (IOObj*)ioobj );
	    needstrigger = true;
	}
    }

    ctio.setObj( ioobj.release() );

    if ( needstrigger )
    {
	mSendChgNotif( cEntryAddedChangeType(), -1 );
	entryAdded.trigger( ctio.ioobj_->key() );
    }
}


#define mIsValidTransl(transl) \
    IOObjSelConstraints::isAllowedTranslator( \
	    transl->userName(),ctio.ctxt_.toselect_.allowtransls_) \
	    && transl->isUserSelectable(false)

IOObj* IOMan::crWriteIOObj( const CtxtIOObj& ctio, const DBKey& newkey,
			    int translidx ) const
{
    const ObjectSet<const Translator>& templs =
	ctio.ctxt_.trgroup_->templates();

    if ( templs.isEmpty() )
    {
	BufferString msg( "Translator Group '",ctio.ctxt_.translatorGroupName(),
			  "is empty." );
	msg.add( ".\nCannot create a default write IOObj for " )
	   .add( ctio.ctxt_.name() );
	pErrMsg( msg ); return 0;
    }

    const Translator* templtr = 0;

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

    return templtr ? templtr->createWriteIOObj( ctio.ctxt_, newkey ) : 0;
}


IOObj* IOMan::getByName( const char* objname, const char* tgname ) const
{
    if ( !objname || !*objname )
	return 0;

    for ( int itype=0; itype<TranslatorGroup::groups().size(); itype++ )
    {
	const TranslatorGroup& tgrp = *TranslatorGroup::groups()[itype];
	if ( tgname && FixedString(tgname) != tgrp.groupName() )
	    continue;

	IODir iodir( tgrp.ioCtxt().getSelDirID() );
	if ( iodir.isBad() )
	    continue;

	IOObj* res = iodir.getEntryByName( objname, tgname );
	if ( res )
	    return res;
    }

    return 0;
}


bool IOMan::isPresent( const DBKey& id ) const
{
    PtrMan<IOObj> obj = get( id );
    return obj != 0;
}


bool IOMan::isPresent( const char* objname, const char* tgname ) const
{
    PtrMan<IOObj> obj = getByName( objname, tgname );
    return obj != 0;
}


bool IOMan::commitChanges( const IOObj& ioobj )
{
    PtrMan<IOObj> ioobjclone = ioobj.clone();
    mLock4Read();
    mGoTo( ioobjclone->key().dirID(), false );
    if ( !dirptr_ )
	return false;

    mLock2Write();
    if ( !dirptr_->commitChanges(ioobjclone) )
	{ errmsg_ = dirptr_->errMsg(); return false; }

    return true;
}


bool IOMan::permRemove( const DBKey& ky )
{
    const DirID dirid( ky.dirID() );
    mLock4Read();
    const bool issamedir = dirptr_ && dirid == dirptr_->dirID();
    bool removesucceeded;
    if ( issamedir )
    {
	mLock2Write();
	removesucceeded = dirptr_->permRemove( ky );
    }
    else
    {
	IODir iodir( dirid );
	removesucceeded = iodir.permRemove( ky );
    }
    if ( removesucceeded )
    {
	mSendChgNotif( cEntryRemovedChangeType(), -1 );
	entryRemoved.trigger( ky );
    }

    return removesucceeded;
}


class SurveyDataTreePreparer
{ mODTextTranslationClass(SurveyDataTreePreparer);
public:
			SurveyDataTreePreparer( const IOMan::CustomDirData& dd )
			    : dirdata_(dd)
			{}

    bool		prepDirData();
    bool		prepSurv();
    bool		createDataTree();

    const IOMan::CustomDirData&	dirdata_;
    uiString		errmsg_;
};


bool SurveyDataTreePreparer::prepDirData()
{
    IOMan::CustomDirData* dd = const_cast<IOMan::CustomDirData*>( &dirdata_ );

    dd->desc_.replace( ':', ';' );
    dd->dirname_.clean();

    if ( dd->dirnr_ <= 200000 )
    {
	errmsg_ = tr("Invalid selection key passed for '%1'")
		    .arg( dd->desc_ );
	return false;
    }

    return true;
}


bool SurveyDataTreePreparer::prepSurv()
{
    if ( IOM().isBad() )
	{ errmsg_ = tr("IO Manager in 'Bad' state"); return false; }

    const DBKey dirky( DBKey::DirID::get(dirdata_.dirnr_) );

    PtrMan<IOObj> ioobj = IOM().get( dirky );
    if ( ioobj )
	return true;

    IOM().toRoot();
    if ( IOM().isBad() )
    {
	errmsg_ = tr("Can't go to root of survey");
	return false;
    }

    MonitorLock ml( IOM() );
    IODir* topdir = IOM().dirptr_;
    if ( !topdir || topdir->name() != "Appl dir" )
	return true;

    if ( !createDataTree() )
	return false;

    // Maybe the parent entry is already there, then this would succeed now:
    ioobj = IOM().get( dirky );
    if ( ioobj )
	return true;

    if ( !IOM().dirptr_->addObj(IOMan::getIOSubDir(dirdata_),true) )
    {
	errmsg_ = tr("Couldn't add '%1' directory to root .omf")
		    .arg( dirdata_.dirname_ );
	errmsg_.append( uiStrings::sCheckPermissions(), true );
	return false;
    }

    return true;
}


bool SurveyDataTreePreparer::createDataTree()
{
    MonitorLock ml( IOM() );
    if ( !IOM().dirptr_ )
	{ errmsg_ = tr("Invalid current survey"); return false; }

    FilePath fp( IOM().dirptr_->dirName() );
    fp.add( dirdata_.dirname_ );
    const BufferString thedirnm( fp.fullPath() );
    bool dircreated = false;
    if ( !File::exists(thedirnm) )
    {
	if ( !File::createDir(thedirnm) )
	{
	    errmsg_ = tr("Cannot create '%1' directory in survey")
			.arg( dirdata_.dirname_ );
	    errmsg_.append( uiStrings::sCheckPermissions(), true );
	    return false;
	}

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

	errmsg_ = tr("Could not create '.omf' file in %1")
		    .arg( dirdata_.dirname_ );
	errmsg_.append( uiStrings::sCheckPermissions(), true );
	return false;
    }

    strm << GetProjectVersionName();
    strm << "\nObject Management file\n";
    strm << Time::getDateTimeString();
    strm << "\n!\nID: " << dirdata_.dirnr_ << "\n!\n"
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


IOMan::DirID IOMan::addCustomDataDir( const IOMan::CustomDirData& dd,
					uiString& errmsg )
{
    SurveyDataTreePreparer sdtp( dd );
    if ( !sdtp.prepDirData() )
    {
	errmsg = sdtp.errmsg_;
	return DirID::getInvalid();
    }

    TypeSet<IOMan::CustomDirData>& cdds = getCDDs();
    for ( int idx=0; idx<cdds.size(); idx++ )
    {
	const IOMan::CustomDirData& cdd = cdds[idx];
	if ( cdd.dirnr_ == dd.dirnr_ )
	    return DBKey::DirID::get( cdd.dirnr_ );
    }

    cdds += dd;
    int idx = cdds.size() - 1;
    const char* survnm = IOM().surveyName();
    if ( survnm && *survnm )
	setupCustomDataDirs( idx, errmsg );

    return DBKey::DirID::get( cdds[idx].dirnr_ );
}


void IOMan::setupCustomDataDirs( int taridx, uiString& errmsg )
{
    const TypeSet<IOMan::CustomDirData>& cdds = getCDDs();
    for ( int idx=0; idx<cdds.size(); idx++ )
    {
	if ( taridx >= 0 && idx != taridx )
	    continue;

	SurveyDataTreePreparer sdtp( cdds[idx] );
	if ( !sdtp.prepSurv() )
	    errmsg = sdtp.errmsg_;
    }
}


IOSubDir* IOMan::getIOSubDir( const IOMan::CustomDirData& cdd )
{
    IOSubDir* sd = new IOSubDir( cdd.dirname_ );
    sd->setDirName( IOM().rootDir() );
    sd->setKey( DBKey( DBKey::DirID::get(cdd.dirnr_) ) );
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
    {
	// probably we're in a survey. Still let's be sure:
	fp.setFileName( "Seismics" );
	if ( File::isDirectory(fp.fullPath()) )
	    return false;
    }

    return true;
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


void IOMan::findTempObjs( ObjectSet<IOObj>& ioobjs,
			 const IOObjSelConstraints* cnstrts ) const
{
    for ( int iseltyp=0; iseltyp<(int)IOObjContext::None; iseltyp++ )
    {
	const IOObjContext::StdSelType st = (IOObjContext::StdSelType)iseltyp;
	const IOObjContext::StdDirData* sdd = IOObjContext::getStdDirData( st );
	if ( sdd )
	    IODir::getTmpIOObjs( sdd->id_, ioobjs, cnstrts );
    }
}
