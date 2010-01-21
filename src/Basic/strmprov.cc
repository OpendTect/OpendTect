/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Stream Provider functions
-*/

static const char* rcsID = "$Id: strmprov.cc,v 1.105 2010-01-21 05:37:09 cvsranojay Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "strmprov.h"
#include "datapack.h"
#include "keystrs.h"
#include "iopar.h"
#include "envvars.h"

#ifdef __win__
# include "winutils.h"
# include <windows.h>
# include <istream>


# ifdef __msvc__
#  define popen _popen
#  define pclose _pclose
#  define fileno(s) _fileno(s)
#  include "errh.h"
#  include "winstreambuf.h"
# endif
#endif

#ifndef __msvc__
# include <ext/stdio_filebuf.h>
# define mStdIOFileBuf __gnu_cxx::stdio_filebuf<char>
#endif

#include "filegen.h"
#include "filepath.h"
#include "string2.h"
#include "strmoper.h"
#include "callback.h"
#include "namedobj.h"
#include "debugmasks.h"
#include "oddirs.h"
#include "errh.h"
#include "executor.h"
#include "fixedstreambuf.h"


static BufferString oscommand( 2048, false );
const char* StreamProvider::sStdIO()	{ return "Std-IO"; }
const char* StreamProvider::sStdErr()	{ return "Std-Err"; }
#define mPreLoadChunkSz 8388608
		// 8 MB

#ifndef __win__

#define mkUnLinked(fnm) fnm

#else

static const char* mkUnLinked( const char* fnm )
{
    if ( !fnm || !*fnm )
	return fnm;

    // Maybe the file itself is a link
    static BufferString ret; ret = File_linkTarget(fnm);
    if ( File_exists(ret) )
	return ret.buf();

    // Maybe there are links in the directories
    FilePath fp( fnm );
    int nrlvls = fp.nrLevels();
    for ( int idx=0; idx<nrlvls; idx++ )
    {
	BufferString dirnm = fp.dirUpTo( idx );
	const bool islink = File_isLink(dirnm);
	if ( islink )
	    dirnm = File_linkTarget( fp.dirUpTo(idx) );
	if ( !File_exists(dirnm) )
	    return fnm;

	if ( islink )
	{
	    FilePath fp2( dirnm );
	    for ( int ilvl=idx+1; ilvl<nrlvls; ilvl++ )
		fp2.add( fp.dir(ilvl) );

	    fp = fp2;
	    nrlvls = fp.nrLevels();
	}
    }

    ret = fp.fullPath();
    return ret.buf();
}

#endif


bool ExecOSCmd( const char* comm, bool inbg )
{
    if ( !comm || !*comm ) return false;

#ifndef __win__

    BufferString oscmd(comm);

    if ( inbg ) 
	oscmd += "&";

    int res = system( oscmd );
    return !res;

#else

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(STARTUPINFO));
    ZeroMemory( &pi, sizeof(pi) );
    si.cb = sizeof(STARTUPINFO);
    
    if(  inbg )
    {
	si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
	si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);	
	si.wShowWindow = SW_HIDE;
    }
    
   //Start the child process. 
    int res = CreateProcess( NULL,	// No module name (use command line). 
        const_cast<char*>( comm ),
        NULL,				// Process handle not inheritable. 
        NULL,				// Thread handle not inheritable. 
        FALSE,				// Set handle inheritance to FALSE. 
        0,				// Creation flags. 
        NULL,				// Use parent's environment block. 
        NULL,       			// Use parent's starting directory. 
        &si, &pi );
	
    if ( res )
    {
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );
    }
    else
    {
	char *ptr = NULL;
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
	    FORMAT_MESSAGE_FROM_SYSTEM,
	    0, GetLastError(), 0, (char *)&ptr, 1024, NULL);

	fprintf(stderr, "\nError: %s\n", ptr);
	LocalFree(ptr);
    }

    return res;

#endif
}

const char* GetExecCommand( const char* prognm, const char* filenm )
{
    static BufferString cmd;
    cmd = "@";
    cmd += mGetExecScript(); cmd += " "; cmd += prognm;

    BufferString fnm( filenm );
    replaceCharacter( fnm.buf(), ' ', (char)128 );
    FilePath fp( fnm );
    cmd += " \'"; cmd += fp.fullPath( FilePath::Unix ); cmd += "\' ";
    return cmd;
}


bool ExecuteScriptCommand( const char* prognm, const char* filenm )
{
    static BufferString cmd;

#ifdef __msvc__
    cmd = BufferString( prognm );
    cmd += " \"";
    cmd += filenm;
    cmd += "\"";
    return ExecOSCmd( cmd, false );
#endif
    
    cmd = GetExecCommand( prognm, filenm );
    StreamProvider strmprov( cmd );
#if defined( __win__ ) || defined( __mac__ )
    bool inbg = true;
#else
    bool inbg = false;
#endif
    if ( !strmprov.executeCommand(inbg) )
    {
	BufferString s( "Failed to submit command '" );
	s += strmprov.command(); s += "'";
	ErrMsg( s );
	return false;
    }

    return true;
}


//---- StreamData ----

StreamData& StreamData::operator =( const StreamData& sd )
{
    if ( this != &sd )
	copyFrom( sd );
    return *this;
}


void StreamData::close()
{
    if ( istrm && istrm != &std::cin )
    	delete istrm;

    if ( ostrm )
    {
	ostrm->flush();
	if ( ostrm != &std::cout && ostrm != &std::cerr )
	    delete ostrm;
    }

    if ( fp_ && fp_ != stdin && fp_ != stdout && fp_ != stderr )
	{ if ( ispipe_ ) pclose(fp_); else fclose(fp_); }

    initStrms();
}


bool StreamData::usable() const
{
    return ( istrm || ostrm ) && ( !ispipe_ || fp_ );
}


void StreamData::copyFrom( const StreamData& sd )
{
    istrm = sd.istrm; ostrm = sd.ostrm;
    fp_ = sd.fp_; ispipe_ = sd.ispipe_;
    setFileName( sd.fname_ );
}


void StreamData::transferTo( StreamData& sd )
{
    sd.copyFrom( *this );
    initStrms();
}


void StreamData::setFileName( const char* f )
{
    delete [] fname_;
    fname_ = f ? new char [strlen(f)+1] : 0;
    if ( fname_ ) strcpy( fname_, f );
}


//---- Pre-loaded data ----


class StreamProviderPreLoadDataPack : public BufferDataPack
{
public:

StreamProviderPreLoadDataPack( char* b, od_int64 s,
				const char* nm, const char* ki )
    : BufferDataPack(b,s,"Pre-loaded file")
    , keyid_(ki)
{
    setName( nm );
}

void dumpInfo( IOPar& iop ) const
{
    BufferDataPack::dumpInfo( iop );
    iop.set( IOPar::compKey("Object",sKey::ID), keyid_ );
}

    BufferString	keyid_;

};


class StreamProviderPreLoadedData : public Executor
{
public:

StreamProviderPreLoadedData( const char* nm, const char* id )
    : Executor("Pre-loading data")
    , dp_(0)
    , id_(id)
    , filesz_(0)
    , chunkidx_(0)
    , msg_("Reading '")
    , fnm_(nm)
{
    FilePath fp( nm ); msg_ += fp.fileName(); msg_ += "'";

    sd_ = StreamProvider(nm).makeIStream(true,false);
    if ( !sd_.usable() )
	{ msg_ = "Cannot open '"; msg_ += nm; msg_ += "'"; }
    else
    {
	od_int64 bufsz = File_getKbSize( nm ) + 1;
	bufsz *= 1024;
	char* buf = 0;
	mTryAlloc(buf,char [ bufsz ])
	if ( !buf )
	{
	    msg_ = "Failed to allocate "; msg_ += bufsz / 1024;
	    msg_ += " kb of memory";
	}
	else
	{
	    dp_ = new StreamProviderPreLoadDataPack( buf, bufsz, nm, id );
	    DPM(DataPackMgr::BufID()).addAndObtain( dp_ );
	}
    }

    if ( !dp_ )
	sd_.close();
}

~StreamProviderPreLoadedData()
{
    sd_.close();
    if ( dp_ )
	DPM(DataPackMgr::BufID()).release( dp_->id() );
}

const BufferString& fileName() const
{
    return dp_ ? dp_->name() : fnm_;
}

const char* message() const { return msg_.buf(); }
const char* nrDoneText() const { return "MBs read"; }
od_int64 nrDone() const { return filesz_ / 1024; }
od_int64 totalNr() const { return dp_ ? dp_->size() / 1024 : -1; }

int nextStep()
{
    if ( !isOK() ) return ErrorOccurred();

    std::istream& strm = *sd_.istrm;
    od_int64 offs = chunkidx_; offs *= mPreLoadChunkSz;
    strm.read( dp_->buf() + offs, mPreLoadChunkSz );
    chunkidx_++;
    if ( strm.good() )
    {
	filesz_ += mPreLoadChunkSz;
	return MoreToDo();
    }
    else if ( strm.eof() )
    {
	filesz_ += strm.gcount();
	sd_.close();
	return Finished();
    }

    msg_ = "Read error for '"; msg_ += fnm_; msg_ += "'";
    return ErrorOccurred();
}

bool go( TaskRunner& tr )
{
    return tr.execute( *this );
}

bool isOK() const
{
    return dp_;
}

    const BufferString	id_;
    StreamData		sd_;
    BufferDataPack*	dp_;
    int			chunkidx_;
    od_int64		filesz_;
    BufferString	msg_;
    BufferString	fnm_;

    char		buf_[mPreLoadChunkSz];

};


static ObjectSet<StreamProviderPreLoadedData>& PLDs()
{
    static ObjectSet<StreamProviderPreLoadedData>* plds = 0;
    if ( !plds ) plds = new ObjectSet<StreamProviderPreLoadedData>;
    return *plds;
}


class StreamProviderDataPreLoader : public Executor
{
public:

StreamProviderDataPreLoader( const BufferStringSet& nms, const char* id )
    : Executor("Pre-loading data")
    , id_(id)
    , curpld_(0)
    , curnmidx_(-1)
    , totnr_(0)
    , nrdone_(0)
{
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	const char* fnm = nms.get( idx );
	if ( !File_exists(fnm) )
	    continue;

	fnms_.add( fnm );
	totnr_ += File_getKbSize( fnm );
    }
    mkNewPLD();
}

bool mkNewPLD()
{
    if ( curpld_ )
    {
	if ( curpld_->isOK() )
	{
	    PLDs() += curpld_;
	    nrdone_ += curpld_->nrDone();
	}
	else
	{
	    totnr_ -= File_getKbSize( curpld_->fnm_ );
	    delete curpld_;
	}
    }

    curpld_ = 0;
    curnmidx_++;

    if ( curnmidx_ < fnms_.size() )
    {
	const char* fnm = fnms_.get( curnmidx_ );
	if ( StreamProvider::isPreLoaded(fnm,false) )
	    StreamProvider::unLoad(fnm,false);
	curpld_ = new StreamProviderPreLoadedData( fnm, id_ );
    }

    return curpld_;
}

const char* message() const	{ return curpld_ ? curpld_->message() : ""; }
const char* nrDoneText() const	{ return curpld_ ? curpld_->nrDoneText() : ""; }
od_int64 totalNr() const	{ return totnr_; }
od_int64 nrDone() const	
{ return nrdone_ + (curpld_ ? curpld_->nrDone() : 0); }

int nextStep()
{
    if ( !curpld_ ) return Executor::Finished();

    int res = curpld_->nextStep();
    if ( res != Executor::Finished() )
	return res;
    else
	return mkNewPLD() ? Executor::MoreToDo() : Executor::Finished();
}

    const BufferString		id_;
    BufferStringSet		fnms_;
    int				curnmidx_;
    od_int64			totnr_;
    od_int64			nrdone_;
    StreamProviderPreLoadedData* curpld_;

};


static int getPLID( const char* key, bool isid )
{
    const ObjectSet<StreamProviderPreLoadedData>& plds = PLDs();
    for ( int idx=0; idx<plds.size(); idx++ )
    {
	const StreamProviderPreLoadedData& pld = *plds[idx];
	if ( !pld.isOK() ) continue;

	if ( (!isid && pld.fileName() == key) || (isid && pld.id_ == key) )
	    return idx;
    }
    return -1;
}


bool StreamProvider::isPreLoaded( const char* key, bool isid )
{
    return getPLID(key,isid) >= 0;
}


bool StreamProvider::preLoad( const char* fnm, TaskRunner& tr, const char* id )
{
    if ( !fnm || !*fnm || isPreLoaded(fnm,false) ) return true;

    StreamProviderPreLoadedData* newpld =
			new StreamProviderPreLoadedData( fnm, id );
    if ( newpld->go(tr) && newpld->isOK() )
	PLDs() += newpld;
    else
	{ delete newpld; newpld = 0; }

    return newpld;
}


bool StreamProvider::preLoad( const BufferStringSet& fnms, TaskRunner& tr,
				const char* id )
{
    if ( fnms.isEmpty() ) return true;

    StreamProviderDataPreLoader exec( fnms, id );
    return tr.execute( exec );
}


void StreamProvider::unLoad( const char* key, bool isid )
{
    ObjectSet<StreamProviderPreLoadedData>& plds = PLDs();
    while ( true )
    {
	int plid = getPLID( key, isid );
	if ( plid < 0 ) return;
	delete plds.remove( plid, false );
    }
}


void StreamProvider::unLoadAll()
{
    deepErase( PLDs() );
}


void StreamProvider::getPreLoadedIDs( BufferStringSet& bss )
{
    bss.erase();
    ObjectSet<StreamProviderPreLoadedData>& plds = PLDs();
    for ( int idx=0; idx<plds.size(); idx++ )
    {
	const StreamProviderPreLoadedData& pld = *plds[idx];
	if ( pld.isOK() )
	    bss.addIfNew( pld.id_ );
    }
}


void StreamProvider::getPreLoadedFileNames( const char* id,
					    BufferStringSet& bss )
{
    bss.erase();
    ObjectSet<StreamProviderPreLoadedData>& plds = PLDs();
    for ( int idx=0; idx<plds.size(); idx++ )
    {
	const StreamProviderPreLoadedData& pld = *plds[idx];
	if ( !pld.isOK() ) continue;

	if ( !id || pld.id_ == id )
	    bss.add( pld.fileName() );
    }
}


int StreamProvider::getPreLoadedDataPackID( const char* fnm )
{
    ObjectSet<StreamProviderPreLoadedData>& plds = PLDs();
    for ( int idx=0; idx<plds.size(); idx++ )
    {
	const StreamProviderPreLoadedData& pld = *plds[idx];
	if ( !pld.isOK() ) continue;

	if ( pld.fileName() == fnm )
	    return pld.dp_->id();
    }
    return -1;
}


StreamData StreamProvider::makePLIStream( int plid )
{
    StreamProviderPreLoadedData& pld = *PLDs()[plid];
    StreamData ret; ret.setFileName( pld.fileName() );
    std::fixedstreambuf* fsb
		= new std::fixedstreambuf( pld.dp_->buf(), pld.filesz_, false );
    ret.istrm = new std::istream( fsb );
    return ret;
}


StreamProvider::StreamProvider( const char* devname )
    	: rshcomm_("rsh")
{
    set( devname );
}


StreamProvider::StreamProvider( const char* hostnm, const char* fnm,
				StreamConn::Type typ )
	: isbad_(false)
	, type_(typ)
	, hostname_(hostnm)
	, fname_(fnm?fnm:sStdIO())
    	, rshcomm_("rsh")
{
    if ( fname_.isEmpty() ) isbad_ = true;
}


void StreamProvider::set( const char* devname )
{
    type_ = StreamConn::File;
    isbad_ = false;
    blocksize_ = 0;
    hostname_ = "";

    if ( !devname || !strcmp(devname,sStdIO()) || !strcmp(devname,sStdErr()) )
    {
	type_ = StreamConn::File;
	fname_ = devname ? devname : sStdIO();
	return;
    }
    else if ( !*devname )
    {
	isbad_ = true; fname_ = "";
	return;
    }

    char* ptr = (char*)devname;
    if ( *ptr == '@' ) { type_ = StreamConn::Command; ptr++; }
    mSkipBlanks( ptr );
    fname_ = ptr;

    // separate hostname from filename
    ptr = strchr( fname_.buf(), ':' );
    if ( ptr ) 
    {   // check for win32 drive letters.

	bool isdrive = false;
	// if only one char before the ':', it must be a drive letter.
	if ( ptr == fname_.buf() + 1 
		|| ( *fname_.buf()=='\"' && (ptr == fname_.buf()+2) ) 
		|| ( *fname_.buf()=='\'' && (ptr == fname_.buf()+2) ) )
	{
	    isdrive = true;
	}
	else
	{
	    BufferString buf(fname_.buf());
	    char* ptr2 = strchr( buf.buf(), ':' );
	    *ptr2='\0';

	    ptr2 = buf.buf();
	    mSkipBlanks( ptr2 );

	    // If spaces are found, it cannot come from a hostname.
	    // So, further up in the string must be a drive letter
	    isdrive = strchr( ptr2, ' ' );
	}

	if( isdrive )
	    ptr = fname_.buf();
	else
	{
	    *ptr++ = '\0';
	    hostname_ = fname_;
	}
    }
    else
	ptr = fname_.buf();

    if ( *ptr == '@' ) { type_ = StreamConn::Command; ptr++; }

    char* ptrname = fname_.buf();
    while ( *ptr ) *ptrname++ = *ptr++;
    *ptrname = '\0';

    if ( type_ != StreamConn::Command && matchString( "/dev/", fname_ ) )
	type_ = StreamConn::Device;
}


bool StreamProvider::isNormalFile() const
{
    return type_ == StreamConn::File && hostname_.isEmpty();
}


bool StreamProvider::rewind() const
{
    if ( isbad_ ) return false;
    else if ( type_ != StreamConn::Device ) return true;

    if ( !hostname_.isEmpty() )
	sprintf( oscommand.buf(), "%s %s \"mt -f %s rewind\"",
		 rshcomm_.buf(), hostname_.buf(), fname_.buf() );
    else
	sprintf( oscommand.buf(), "mt -f %s rewind", fname_.buf() );
    return ExecOSCmd(oscommand);
}


bool StreamProvider::offline() const
{
    if ( isbad_ ) return false;
    else if ( type_ != StreamConn::Device ) return true;

    if ( !hostname_.isEmpty() )
	sprintf( oscommand.buf(), "%s %s \"mt -f %s offline\"",
				  rshcomm_.buf(),
				  hostname_.buf(), fname_.buf() );
    else
	sprintf( oscommand.buf(), "mt -f %s offline", fname_.buf() );
    return ExecOSCmd(oscommand);
}


bool StreamProvider::skipFiles( int nr ) const
{
    if ( isbad_ ) return false;
    if ( type_ != StreamConn::Device ) return false;

    if ( !hostname_.isEmpty() )
	sprintf( oscommand.buf(), "%s %s \"mt -f %s fsf %d\"",
				rshcomm_.buf(),
				hostname_.buf(), fname_.buf(), nr );
    else
	sprintf( oscommand.buf(), "mt -f %s fsf %d", fname_.buf(), nr );
    return ExecOSCmd(oscommand);
}


const char* StreamProvider::fullName() const
{
    oscommand = "";
    if ( type_ == StreamConn::Command )
	oscommand += "@";
    if ( !hostname_.isEmpty() ) 
    {
	oscommand += hostname_;
	oscommand += ":";
    }
    oscommand += fname_.buf();

    return oscommand;
}


void StreamProvider::addPathIfNecessary( const char* path )
{
    if ( isbad_ ) return;

    if ( type_ != StreamConn::File
      || !path || ! *path
      || fname_ == sStdIO() || fname_ == sStdErr() )
	return;

    FilePath fp( fname_ );
    if ( fp.isAbsolute() )
	return;

    fp.insert( path );
    fname_ = fp.fullPath();
}


StreamData StreamProvider::makeIStream( bool binary, bool allowpl ) const
{
    const bool isfile = type_ != StreamConn::Command;
    const bool islocal = hostname_.isEmpty();
    StreamData sd;
    if ( isfile && islocal )
	sd.setFileName( mkUnLinked(fname_) );
    else
	sd.setFileName( fname_ );
    if ( isbad_ || fname_.isEmpty() )
	return sd;

    if ( fname_ == sStdIO() || fname_ == sStdErr() )
    {
	sd.istrm = &std::cin;
	return sd;
    }

    if ( allowpl )
    {
	const int plid = getPLID( sd.fileName(), false );
	if ( plid >= 0 )
	    return makePLIStream( plid );
    }

    if ( isfile && islocal )
    {
	bool doesexist = File_exists( sd.fileName() );
	if ( !doesexist )
	{
	    FilePath fp( fname_ );
	    BufferString fullpath = fp.fullPath( FilePath::Local, true );
	    if ( !File_exists(fullpath) )
		fullpath = fp.fullPath( FilePath::Local, false );
	    // Sometimes the filename _is_ weird, and the cleanup is wrong
	    doesexist = File_exists( fullpath );
	    if ( doesexist )
		sd.setFileName( fullpath );
	}

#ifdef __msvc__
	sd.istrm = new std::winifstream(
#else
	sd.istrm = new std::ifstream(
#endif
	    sd.fileName(), binary ? std::ios_base::in | std::ios_base::binary
				  : std::ios_base::in );

	if ( doesexist ? sd.istrm->bad() : !sd.istrm->good() )
	    { delete sd.istrm; sd.istrm = 0; }
	return sd;
    }

    mkOSCmd( true );

    sd.fp_ = popen( oscommand, "r" );
    sd.ispipe_ = true;

    if ( sd.fp_ )
    {
#ifdef __msvc__
	std::filebuf* fb = new std::filebuf( sd.fp_ );
	sd.istrm = new std::istream( fb );
#else
# if __GNUC__ > 2
	//TODO change StreamData to include filebuf?
	mStdIOFileBuf* stdiofb = new mStdIOFileBuf( sd.fp_, std::ios_base::in );
	sd.istrm = new std::istream( stdiofb );
# else
	sd.istrm = new std::ifstream( fileno(sd.fp) );  
# endif
#endif
    }

    return sd;
}


StreamData StreamProvider::makeOStream( bool binary ) const
{
    StreamData sd;
    sd.setFileName( mkUnLinked(fname_) );
    if ( isbad_ || fname_.isEmpty() )
	return sd;

    else if ( fname_ == sStdIO() )
    {
	sd.ostrm = &std::cout;
	return sd;
    }
    else if ( fname_ == sStdErr() )
    {
	sd.ostrm = &std::cerr;
	return sd;
    }

    if ( type_ != StreamConn::Command && hostname_.isEmpty() )
    {
#ifdef __msvc__
	sd.ostrm = new std::winofstream(
#else
	sd.ostrm = new std::ofstream(
#endif
	    sd.fileName(), binary ? std::ios_base::out | std::ios_base::binary
				  : std::ios_base::out );
	if ( sd.ostrm->bad() )
	    { delete sd.ostrm; sd.ostrm = 0; }
	return sd;
    }

    mkOSCmd( false );

    sd.fp_ = popen( oscommand, "w" );
    sd.ispipe_ = true;

    if ( sd.fp_ )
    {
#ifdef __msvc__
	std::filebuf* fb = new std::filebuf( sd.fp_ );
	sd.ostrm = new std::ostream( fb );
#else
# if __GNUC__ > 2
	mStdIOFileBuf* stdiofb = new mStdIOFileBuf( sd.fp_,std::ios_base::out );
	sd.ostrm = new std::ostream( stdiofb );
# else
	sd.ostrm = new std::ofstream( fileno(sd.fp) );
# endif
#endif
    }

    return sd;
}


#ifdef __msvc__
bool ExecWinCmd( const char* comm, bool inbg, bool inconsole )
{
    if ( !comm || !*comm ) return false;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(STARTUPINFO));
    ZeroMemory( &pi, sizeof(pi) );
    si.cb = sizeof(STARTUPINFO);

    if ( !inconsole )
    {
	si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
	si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);	
	si.wShowWindow = SW_HIDE;
    }
    
   //Start the child process. 
    int res = CreateProcess( NULL,	// No module name (use command line). 
        const_cast<char*>( comm ),
        NULL,				// Process handle not inheritable. 
        NULL,				// Thread handle not inheritable. 
        FALSE,				// Set handle inheritance to FALSE. 
        0,				// Creation flags. 
        NULL,				// Use parent's environment block. 
        NULL,       			// Use parent's starting directory. 
        &si, &pi );
	
    if ( res )
    {
	if ( !inbg )  WaitForSingleObject( pi.hProcess, INFINITE );
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );
    }
    else
    {
	char *ptr = NULL;
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
	    FORMAT_MESSAGE_FROM_SYSTEM,
	    0, GetLastError(), 0, (char *)&ptr, 1024, NULL);

	fprintf(stderr, "\nError: %s\n", ptr);
	LocalFree(ptr);
    }

    return res;
}
#endif


bool StreamProvider::executeCommand( bool inbg, bool inconsole ) const
{
    mkOSCmd( true );
#ifdef __msvc__
    if ( inconsole )
    {
	mkBatchCmd( oscommand );
	return ExecWinCmd( oscommand, inbg, inconsole );
    }
#endif
    return ExecOSCmd( oscommand, inbg );
}


void StreamProvider::mkBatchCmd( BufferString& comm ) const
{
    FilePath batfp( FilePath::getTempDir() );
    batfp.add( "odtmp.bat" );
   
    FILE *fp;
    fp = fopen( batfp.fullPath(), "wt" );
    fprintf( fp, "@echo off\n" );
    fprintf( fp,"%s\n", comm.buf() );
    fprintf( fp, "pause\n" );
    fclose( fp );

    comm = batfp.fullPath();
}


#ifdef __win__

static const char* getCmd( const char* fnm )
{
    BufferString execnm( fnm );

    char* ptr = strchr( execnm.buf() , ':' );

    if ( !ptr )
	return fnm;

    char* args=0;

    // if only one char before the ':', it must be a drive letter.
    if ( ptr == execnm.buf() + 1 )
    {
	ptr = strchr( ptr , ' ' );
	if ( ptr ) { *ptr = '\0'; args = ptr+1; }
    }
    else if ( ptr == execnm.buf()+2) 
    {
	char sep = *execnm.buf();
	if ( sep == '\"' || sep == '\'' )
	{
	    execnm=fnm+1;
	    ptr = strchr( execnm.buf() , sep );
	    if ( ptr ) { *ptr = '\0'; args = ptr+1; }
	}
    }
    else
	return fnm;

    if ( strstr(execnm,".exe") || strstr(execnm,".EXE") 
       || strstr(execnm,".bat") || strstr(execnm,".BAT")
       || strstr(execnm,".com") || strstr(execnm,".COM") )
	return fnm;

    const char* interp = 0;

    if ( strstr(execnm,".csh") || strstr(execnm,".CSH") )
	interp = "tcsh.exe";
    else if ( strstr(execnm,".sh") || strstr(execnm,".SH") ||
	      strstr(execnm,".bash") || strstr(execnm,".BASH") )
	interp = "sh.exe";
    else if ( strstr(execnm,".awk") || strstr(execnm,".AWK") )
	interp = "awk.exe";
    else if ( strstr(execnm,".sed") || strstr(execnm,".SED") )
	interp = "sed.exe";
    else if ( File_exists( execnm ) )
    {
	// We have a full path to a file with no known extension,
	// but it exists. Let's peek inside.

	StreamData sd = StreamProvider( execnm ).makeIStream();
	if ( !sd.usable() )
	    return fnm;

	BufferString line;
	sd.istrm->getline( line.buf(), 40 ); sd.close();

	if ( !strstr(line,"#!") && !strstr(line,"# !") )
	    return fnm;

	if ( strstr(line,"csh") )
	    interp = "tcsh.exe";
	else if ( strstr(line,"awk") )
	    interp = "awk.exe";
	else if ( strstr(line,"sh") )
	    interp = "sh.exe";
    }
    
    if ( interp )
    {
	static BufferString fullexec;

	fullexec = "\"";

	FilePath interpfp;

	if ( getCygDir() )
	{
	    interpfp.set( getCygDir() );
	    interpfp.add("bin").add(interp);
	}

	if ( !File_exists( interpfp.fullPath() ) )
	{
	    interpfp.set( GetSoftwareDir(0) );
	    interpfp.add("bin").add("win").add("sys")
		    .add(interp).fullPath();
	}

	fullexec += interpfp.fullPath();

	fullexec += "\" '";
	FilePath execfp( execnm );
	fullexec += execfp.fullPath( FilePath::Unix );
	fullexec += "'";

	if ( args && *args )
	{
	    fullexec += " ";
	    fullexec += args;
	}

	return fullexec;
    }

    return fnm;
}
#endif


#ifdef __win__
# define mGetCmd(fname_) getCmd(fname_)
#else
# define mGetCmd(fname_) fname_.buf()
#endif

void StreamProvider::mkOSCmd( bool forread ) const
{
    if ( hostname_.isEmpty() )
	oscommand = mGetCmd(fname_);
    else
    {
	switch ( type_ )
	{
	case StreamConn::Device:
	    if ( forread )
	    {
		if ( blocksize_ > 0 )
		    sprintf( oscommand.buf(), "%s %s dd if=%s ibs=%ld",
				rshcomm_.buf(), hostname_.buf(),
				fname_.buf(), blocksize_ );
		else
		    sprintf( oscommand.buf(), "%s %s dd if=%s",
				rshcomm_.buf(), hostname_.buf(), fname_.buf() );
	    }
	    else
	    {
		if ( blocksize_ > 0 )
		    sprintf( oscommand.buf(), "%s %s dd of=%s obs=%ld",
				  rshcomm_.buf(), hostname_.buf(),
				  fname_.buf(), blocksize_ );
		else
		    sprintf( oscommand.buf(), "%s %s dd of=%s",
				  rshcomm_.buf(),
				  hostname_.buf(), fname_.buf() );
	    }
	break;
	case StreamConn::Command:
	    sprintf( oscommand.buf(), "%s %s %s", rshcomm_.buf(),
		     hostname_.buf(), mGetCmd(fname_) );
	break;
	case StreamConn::File:
	    if ( forread )
		sprintf( oscommand.buf(), "%s %s cat %s",
				rshcomm_.buf(), hostname_.buf(), fname_.buf() );
	    else
		sprintf( oscommand.buf(), "%s %s tee %s > /dev/null",
				rshcomm_.buf(), hostname_.buf(), fname_.buf() );
	break;
	}
    }




    if ( DBG::isOn(DBG_IO) )
    {
	BufferString msg( "About to execute: '" );
	msg += oscommand;
	msg += "'";
	DBG::message( msg );
    }
}


#define mRemoteTest(act) \
    FILE* fp = popen( oscommand, "r" ); \
    char c; fscanf( fp, "%c", &c ); \
    pclose( fp ); \
    act (c == '1')


bool StreamProvider::exists( int fr ) const
{
    if ( isbad_ ) return false;

    if ( type_ == StreamConn::Command )
	return fr;

    if ( hostname_.isEmpty() )
	return fname_ == sStdIO() || fname_ == sStdErr() ? true
	     : File_exists( fname_.buf() );

    sprintf( oscommand.buf(), "%s %s 'test -%c %s && echo 1'", rshcomm_.buf(),
	    			hostname_.buf(), fr ? 'r' : 'w', fname_.buf() );
    mRemoteTest(return);
}


bool StreamProvider::remove( bool recursive ) const
{
    if ( isbad_ || type_ != StreamConn::File ) return false;

    if ( hostname_.isEmpty() )
	return fname_ == sStdIO() || fname_ == sStdErr() ? false :
		File_remove( fname_.buf(), recursive );

    sprintf( oscommand.buf(), "%s %s '/bin/rm -%s %s && echo 1'",
	      rshcomm_.buf(), hostname_.buf(), recursive ? "r" : "",
	      fname_.buf() );

    mRemoteTest(return);
}


bool StreamProvider::setReadOnly( bool yn ) const
{
    if ( isbad_ || type_ != StreamConn::File ) return false;

    if ( hostname_.isEmpty() )
	return fname_ == sStdIO() || fname_ == sStdErr() ? false :
	       File_makeWritable( fname_.buf(), mFile_NotRecursive, !yn );

    sprintf( oscommand.buf(), "%s %s 'chmod %s %s && echo 1'",
	      rshcomm_.buf(), hostname_.buf(), yn ? "a-w" : "ug+w",
	      fname_.buf() );

    mRemoteTest(return);
}


bool StreamProvider::isReadOnly() const
{
    if ( isbad_ || type_ != StreamConn::File ) return true;

    if ( hostname_.isEmpty() )
	return fname_ == sStdIO() || fname_ == sStdErr() ? false :
		!File_isWritable( fname_.buf() );

    sprintf( oscommand.buf(), "%s %s 'test -w %s && echo 1'",
	      rshcomm_.buf(), hostname_.buf(), fname_.buf() );

    mRemoteTest(return !);
}


static void mkRelocMsg( const char* oldnm, const char* newnm,BufferString& msg )
{
    msg = "Relocating '";
    while ( *oldnm && *newnm && *oldnm == *newnm )
	{ oldnm++; newnm++; }
    msg += oldnm; msg += "' to '"; msg += newnm; msg += "' ...";
}


void StreamProvider::sendCBMsg( const CallBack* cb, const char* msg )
{
    NamedObject nobj( msg );
    CBCapsule<const char*> caps( msg, &nobj );
    CallBack(*cb).doCall( &caps );
}


bool StreamProvider::rename( const char* newnm, const CallBack* cb )
{
    bool rv = false;
    const bool issane = newnm && *newnm && !isbad_ && type_ == StreamConn::File;

    if ( cb && cb->willCall() )
    {
	BufferString msg;
	if ( issane )
	    mkRelocMsg( fname_, newnm, msg );
	else if ( type_ != StreamConn::File )
	    msg = "Cannot rename commands or devices";
	else
	{
	    if ( isbad_ )
		msg = "Cannot rename invalid filename";
	    else
		msg = "No filename provided for rename";
	}
	sendCBMsg( cb, msg );
    }

    if ( issane )
    {
	if ( hostname_.isEmpty() )
	    rv = fname_ == sStdIO() || fname_ == sStdErr() ? true :
		    File_rename( fname_.buf(), newnm );
	else
	{
	    sprintf( oscommand.buf(), "%s %s '/bin/mv -f %s %s && echo 1'",
		      rshcomm_.buf(), hostname_.buf(), fname_.buf(), newnm );
	    mRemoteTest(rv =);
	}
    }

    if ( rv )
	set( newnm );
    return rv;
}
