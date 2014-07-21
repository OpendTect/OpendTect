/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 3-8-1994
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "ioman.h"

#include "ascstream.h"
#include "ctxtioobj.h"
#include "file.h"
#include "filepath.h"
#include "iodir.h"
#include "iopar.h"
#include "iostrm.h"
#include "iosubdir.h"
#include "keystrs.h"
#include "msgh.h"
#include "oddirs.h"
#include "od_ostream.h"
#include "separstr.h"
#include "settings.h"
#include "perthreadrepos.h"
#include "strmprov.h"
#include "survinfo.h"
#include "timefun.h"
#include "genc.h"
#include "transl.h"


IOMan* IOMan::theinst_	= 0;
static const MultiID emptykey( "" );


IOMan& IOM()
{
    if ( !IOMan::theinst_ )
	{ IOMan::theinst_ = new IOMan; IOMan::theinst_->init(); }
    return *IOMan::theinst_;
}

IOMan::IOMan( const char* rd )
	: NamedObject("IO Manager")
	, dirptr_(0)
	, survchgblocked_(false)
	, state_(IOMan::NeedInit)
	, lock_(Threads::Lock(false))
	, newIODir(this)
	, entryRemoved(this)
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
    if ( !to( emptykey, true ) )
    {
        FilePath surveyfp( GetDataDir(), ".omf" );
        if ( File::exists(surveyfp.fullPath().buf()) )
        {
            msg_ = "Warning: Invalid '.omf' found in:\n";
	    msg_ += rootdir_;
            msg_ += ".\nThis survey is corrupt.";
	    return;
        }

        FilePath basicfp( mGetSetupFileName(SurveyInfo::sKeyBasicSurveyName()),
			  ".omf" );
        File::copy( basicfp.fullPath(),surveyfp.fullPath() );
        if ( !to( emptykey, true ) )
        {
            msg_ = "Warning: Invalid or no '.omf' found in:\n";
	    msg_ += rootdir_;
            msg_ += ".\nThis survey is corrupt.";
	    return;
        }
    }

    state_ = Good;
    curlvl_ = 0;

    if ( dirptr_->key().isUdf() ) return;

    int nrstddirdds = IOObjContext::totalNrStdDirs();
    const IOObjContext::StdDirData* prevdd = 0;
    const bool needsurvtype = !SI().survdatatypeknown_;
    bool needwrite = false;
    FilePath rootfp( rootdir_, "X" );
    for ( int idx=0; idx<nrstddirdds; idx++ )
    {
	IOObjContext::StdSelType stdseltyp = (IOObjContext::StdSelType)idx;
	const IOObjContext::StdDirData* dd
			    = IOObjContext::getStdDirData( stdseltyp );
	const IOObj* dirioobj = dirptr_->get( MultiID(dd->id) );
	if ( dirioobj )
	{
	    if ( needsurvtype && stdseltyp == IOObjContext::Seis )
	    {
		IODir seisiodir( dirioobj->key() );
		bool has2d = false, has3d = false;
		const BufferString seisstr( "Seismic Data" );
		const BufferString tr2dstr( "2D" );
		const BufferString trsegystr( "SEG-Y" );
		for ( int iobj=0; iobj<seisiodir.size(); iobj++ )
		{
		    const IOObj& subioobj = *seisiodir.get( iobj );
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
	FilePath basicfp( mGetSetupFileName(SurveyInfo::sKeyBasicSurveyName()),
			  "X" );
	basicfp.setFileName( dd->dirnm );
	BufferString basicdirnm = basicfp.fullPath();
	if ( !File::exists(basicdirnm) )
	    // Oh? So this is removed from the Basic Survey
	    // Let's hope they know what they're doing
	    { prevdd = dd; continue; }

	rootfp.setFileName( dd->dirnm );
	BufferString dirnm = rootfp.fullPath();

#define mErrMsgRet(s) ErrMsg(s); msg_ = s; state_ = Bad; return
	if ( !File::exists(dirnm) )
	{
	    // This directory should have been in the survey.
	    // It is not. If it is the seismic directory, we do not want to
	    // continue. Otherwise, we want to copy the Basic Survey directory.
	    if ( stdseltyp == IOObjContext::Seis )
	    {
		BufferString msg( "Corrupt survey: missing directory: " );
		msg += dirnm; mErrMsgRet( msg );
	    }
	    else if ( !File::copy(basicdirnm,dirnm) )
	    {
		BufferString msg( "Cannot create directory: " ); msg += dirnm;
		msg += ". You probably do not have write permissions in ";
		msg += rootfp.pathOnly(); mErrMsgRet( msg );
	    }
	}

	// So, we have copied the directory.
	// Now create an entry in the root omf
	IOSubDir* iosd = new IOSubDir( dd->dirnm );
	iosd->key_ = dd->id;
	iosd->dirnm_ = rootdir_;
	const IOObj* previoobj = prevdd ? dirptr_->get( MultiID(prevdd->id) )
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
	setupCustomDataDirs(-1);
	if ( dotrigger )
	{
	    IOM().surveyChanged.trigger();
	    IOM().afterSurveyChange.trigger();
	}
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
	mErrRetNotODDir(0)
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
    if ( !to( dirkey, true ) )
	return MultiID::udf();

    return dirptr_->newKey();
}


bool IOMan::to( const MultiID& ky, bool forcereread )
{
    Threads::Locker lock( lock_ );
    const bool issamedir = dirptr_ && ky == dirptr_->key();
    if ( !forcereread && issamedir )
	return true;

    MultiID dirkey;
    IOObj* refioobj = IODir::getObj( ky );
    if ( refioobj )
	dirkey = refioobj->isSubdir() ? ky : MultiID(ky.upLevel());
    else
    {
	dirkey = ky.upLevel();
	refioobj = IODir::getObj( dirkey );
	if ( !refioobj )
	    dirkey = "";
    }
    delete refioobj;

    IODir* newdir = dirkey.isEmpty() ? new IODir(rootdir_) : new IODir(dirkey);
    if ( !newdir || newdir->isBad() )
	return false;

    bool needtrigger = dirptr_;
    if ( dirptr_ )
	delete dirptr_;
    dirptr_ = newdir;
    curlvl_ = levelOf( curDirName() );
    if ( needtrigger )
	newIODir.trigger();

    return true;
}


IOObj* IOMan::get( const MultiID& k ) const
{
    Threads::Locker lock( lock_ );
    if ( !IOObj::isKey(k) )
	return 0;

    MultiID ky( k );
    char* ptr = firstOcc( ky.getCStr(), '|' );
    if ( ptr ) *ptr = '\0';
    ptr = firstOcc( ky.getCStr(), ' ' );
    if ( ptr ) *ptr = '\0';

    if ( dirptr_ )
    {
	const IOObj* ioobj = dirptr_->get( ky );
	if ( ioobj ) return ioobj->clone();
    }

    return IODir::getObj( ky );
}


IOObj* IOMan::getOfGroup( const char* tgname, bool first,
			  bool onlyifsingle ) const
{
    Threads::Locker lock( lock_ );
    if ( isBad() || !tgname ) return 0;

    const IOObj* ioobj = 0;
    for ( int idx=0; idx<dirptr_->size(); idx++ )
    {
	if ( dirptr_->get(idx)->group()==tgname )
	{
	    if ( onlyifsingle && ioobj ) return 0;

	    ioobj = dirptr_->get( idx );
	    if ( first && !onlyifsingle ) break;
	}
    }

    return ioobj ? ioobj->clone() : 0;
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
	return get( MultiID(oky.buf()) );
    }

    if ( dirptr_ )
    {
	const IOObj* ioobj = dirptr_->get( objname, trgrpnm );
	if ( ioobj ) return ioobj->clone();
    }

    if ( IOObj::isKey(objname) )
	return get( MultiID(objname) );

    return 0;
}


IOObj* IOMan::getFirst( const IOObjContext& ctxt, int* nrfound ) const
{
    Threads::Locker lock( lock_ );
    if ( !ctxt.trgroup ) return 0;

    IOM().to( ctxt.getSelKey() );

    const ObjectSet<IOObj>& ioobjs = dirptr_->getObjs();
    IOObj* ret = 0; if ( nrfound ) *nrfound = 0;
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
	    return 0;
	}

	if ( !IOObj::isKey(res.buf()) )
	{
	    CtxtIOObj ctio( ctxt );
	    IOM().to( ctio.ctxt.getSelKey() );
	    const IOObj* ioob = dirptr_->get( res.buf(), 0 );
	    if ( ioob )
		res = ioob->key();
	    else if ( mknew )
	    {
		ctio.setName( res );
		IOM().getEntry( ctio );
		if ( ctio.ioobj )
		{
		    IOM().commitChanges( *ctio.ioobj );
		    return ctio.ioobj;
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
    if ( !ky || !*ky || !isdigit(*ky) ) return false;

    bool digitseen = false;
    while ( *ky )
    {
	if ( isdigit(*ky) )
	    digitseen = true;
	else if ( *ky == '|' )
	    return digitseen;
	else if ( *ky != '.' )
	    return false;
	ky++;
    }

    return true;
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
    if ( needtrigger )
	newIODir.trigger();
    return true;
}


void IOMan::getEntry( CtxtIOObj& ctio, bool mktmp, int translidx )
{
    Threads::Locker lock( lock_ );
    ctio.setObj( 0 );
    if ( ctio.ctxt.name().isEmpty() )
	return;
    to( ctio.ctxt.getSelKey() );

    const IOObj* ioobj = dirptr_->get( ctio.ctxt.name(),
					ctio.ctxt.trgroup->userName() );
    ctio.ctxt.fillTrGroup();
    if ( ioobj && ctio.ctxt.trgroup->userName() != ioobj->group() )
	ioobj = 0;

    if ( !ioobj )
    {
	MultiID newkey( mktmp ? ctio.ctxt.getSelKey() : dirptr_->newKey() );
	if ( mktmp )
	    newkey.add( IOObj::tmpID() );

	ioobj = crWriteIOObj( ctio, newkey, translidx );
	if ( ioobj )
	{
	    ioobj->pars().merge( ctio.ctxt.toselect.require_ );
	    dirptr_->addObj( (IOObj*)ioobj );
	}
    }

    ctio.setObj( ioobj ? ioobj->clone() : 0 );
}


IOObj* IOMan::crWriteIOObj( const CtxtIOObj& ctio, const MultiID& newkey,
			    int translidx ) const
{
    const ObjectSet<const Translator>& templs = ctio.ctxt.trgroup->templates();

    if ( templs.isEmpty() )
    {
	BufferString msg( "Translator Group '", ctio.ctxt.trgroup->userName(),
			  "is empty." );
	msg.add( ".\nCannot create a default write IOObj for " )
	   .add( ctio.ctxt.name() );
	pErrMsg( msg ); return 0;
    }

    const Translator* templtr = 0;

    if ( templs.validIdx(translidx) )
	templtr = ctio.ctxt.trgroup->templates()[translidx];
    else if ( !ctio.ctxt.deftransl.isEmpty() )
	templtr = ctio.ctxt.trgroup->getTemplate(ctio.ctxt.deftransl,true);
    if ( !templtr )
    {
	translidx = ctio.ctxt.trgroup->defTranslIdx();
	templtr = templs[translidx];
    }

    return templtr->createWriteIOObj( ctio.ctxt, newkey );
}


int IOMan::levelOf( const char* dirnm ) const
{
    Threads::Locker lock( lock_ );
    if ( !dirnm ) return 0;

    int lendir = FixedString(dirnm).size();
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
    Threads::Locker lock( lock_ );
    if ( !dirptr_ || !dirptr_->permRemove(ky) )
	return false;

    CBCapsule<MultiID> caps( ky, this );
    entryRemoved.trigger( &caps );
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

    int nr = dd->selkey_.ID( 0 );
    if ( nr <= 200000 )
	mErrRet("Invalid selection key passed for '",dd->desc_,"'")

    dd->selkey_ = "";
    dd->selkey_.setID( 0, nr );

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
    if ( !topdir->main() || topdir->main()->name() == "Appl dir" )
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
    strm << Time::getDateTimeString();
    strm << "\n!\nID: " << dirdata_.selkey_ << "\n!\n"
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

    return true;
}
