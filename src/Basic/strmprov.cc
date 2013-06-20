/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Stream Provider functions
-*/

static const char* rcsID mUsedVar = "$Id$";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "strmprov.h"
#include "datapack.h"
#include "keystrs.h"
#include "iopar.h"
#include "envvars.h"
#include "staticstring.h"

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

#include "file.h"
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
    static BufferString ret; ret = File::linkTarget(fnm);
    if ( File::exists(ret) )
	return ret.buf();

    // Maybe there are links in the directories
    FilePath fp( fnm );
    int nrlvls = fp.nrLevels();
    for ( int idx=0; idx<nrlvls; idx++ )
    {
	BufferString dirnm = fp.dirUpTo( idx );
	const bool islink = File::isLink(dirnm);
	if ( islink )
	    dirnm = File::linkTarget( fp.dirUpTo(idx) );
	if ( !File::exists(dirnm) )
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


bool ExecOSCmd( const char* comm, bool inconsole, bool inbg )
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

#endif
}

#ifndef __msvc__
//! Create Execute command
const char* GetExecCommand(const char* prognm,const char* filenm);

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
#endif


bool ExecuteScriptCommand( const char* prognm, const char* filenm )
{
    static BufferString cmd;
#if defined( __win__ ) || defined( __mac__ )
    bool inbg = true;
#else
    bool inbg = false;
#endif

#ifdef __msvc__
    cmd = BufferString( prognm );
    cmd += " \"";
    cmd += filenm;
    cmd += "\"";
    return ExecOSCmd( cmd, true, inbg );
#else
    cmd = GetExecCommand( prognm, filenm );
    StreamProvider strmprov( cmd );

    if ( !strmprov.executeCommand(inbg) )
    {
	BufferString s( "Failed to submit command '" );
	s += strmprov.command(); s += "'";
	ErrMsg( s );
	return false;
    }

    return true;
#endif
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
    iop.set( IOPar::compKey("Object",sKey::ID()), keyid_ );
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
	od_int64 bufsz = File::getKbSize( nm ) + 1;
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
    return TaskRunner::execute( &tr, *this );
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
	if ( !File::exists(fnm) )
	    continue;

	fnms_.add( fnm );
	totnr_ += File::getKbSize( fnm );
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
	    totnr_ -= File::getKbSize( curpld_->fnm_ );
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
    return TaskRunner::execute( &tr, exec );
}


void StreamProvider::unLoad( const char* key, bool isid )
{
    ObjectSet<StreamProviderPreLoadedData>& plds = PLDs();
    while ( true )
    {
	int plid = getPLID( key, isid );
	if ( plid < 0 ) return;
	delete plds.removeSingle( plid, false );
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


StreamProvider::StreamProvider( const char* inp )
    : rshcomm_("rsh")
    , iscomm_(false)
{
    set( inp );
}


StreamProvider::StreamProvider( const char* hostnm, const char* fnm,
				bool iscomm )
    : isbad_(false)
    , iscomm_(iscomm)
    , hostname_(hostnm)
    , fname_(fnm?fnm:sStdIO())
    , rshcomm_("rsh")
{
    if ( fname_.isEmpty() ) isbad_ = true;
}


void StreamProvider::set( const char* inpstr )
{
    iscomm_ = isbad_ = false;
    hostname_.setEmpty(); fname_.setEmpty();

    FixedString inp = inpstr;

    if ( !inp || inp==sStdIO() || inp==sStdErr() )
	{ fname_ = inpstr ? inpstr : sStdIO(); return; }
    else if ( inp.isEmpty() )
	{ isbad_ = true; return; }

    char* ptr = (char*)inpstr;
    mSkipBlanks( ptr );
    if ( *ptr == '@' ) { iscomm_ = true; ptr++; }

    mSkipBlanks( ptr );
    fname_ = ptr;

    ptr = fname_.buf();
    // separate hostname from filename
#ifdef __win__
    if ( *ptr == '\\' && *(ptr+1) == '\\' )
    {
	char* endptr = strchr( ptr+2, '\\' );
	if ( endptr ) *endptr++ = '\0';
	hostname_ = ptr+2;
	if ( !endptr )
	    { fname_ = sStdIO(); return; }
    }
#else
    ptr = strchr( ptr, ':' );
    if ( ptr && *(ptr+1) == '/' && *(ptr+2) == '/' )
    {
	pErrMsg(BufferString(ptr," looks like a URL. Not supported (yet)"));
	ptr = 0;
    }
    if ( ptr ) 
    {
	const BufferString fnamestr = fname_;
	*ptr++ = '\0';
	if ( !strchr(fname_.buf(),' ') )
	    hostname_ = fname_;
	else	// ':' may just be a part of an argument in the command string.
	{
	    fname_ = fnamestr;
	    ptr = fname_.buf();
	}
    }
    if ( !ptr )
	ptr = fname_.buf();
#endif

    mSkipBlanks( ptr );
    if ( *ptr == '@' ) { iscomm_ = true; ptr++; }
    BufferString tmp( ptr );
    fname_ = tmp;
}


bool StreamProvider::isNormalFile() const
{
    return !iscomm_ && hostname_.isEmpty();
}


const char* StreamProvider::fullName() const
{
    static StaticStringManager stm;
    BufferString& ret = stm.getString();
    ret = "";
    if ( iscomm_ )
	ret += "@";
    if ( !hostname_.isEmpty() ) 
    {
#ifdef __win__
	ret += "\\\\"; ret += hostname_;
#else
	ret += hostname_; ret += ":";
#endif
    }
    ret += fname_.buf();

    return ret.buf();
}


void StreamProvider::addPathIfNecessary( const char* path )
{
    if ( isbad_ ) return;

    if ( iscomm_ || !path || ! *path
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
    const bool islocal = hostname_.isEmpty();
    StreamData sd;
    if ( !iscomm_ && islocal )
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

    if ( !iscomm_ && islocal )
    {
	bool doesexist = File::exists( sd.fileName() );
	if ( !doesexist )
	{
	    FilePath fp( fname_ );
	    BufferString fullpath = fp.fullPath( FilePath::Local, true );
	    if ( !File::exists(fullpath) )
		fullpath = fp.fullPath( FilePath::Local, false );
	    // Sometimes the filename _is_ weird, and the cleanup is wrong
	    doesexist = File::exists( fullpath );
	    if ( doesexist )
		sd.setFileName( fullpath );
	}

#ifdef __msvc__
	sd.istrm = new std::winifstream
#else
	sd.istrm = new std::ifstream
#endif
	  ( sd.fileName(), binary ? std::ios_base::in | std::ios_base::binary
				  : std::ios_base::in );

	if ( !sd.istrm->good() )
	    { delete sd.istrm; sd.istrm = 0; }
	return sd;
    }

    BufferString cmd;
    mkOSCmd( true, cmd );

    sd.fp_ = popen( cmd, "r" );
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


StreamData StreamProvider::makeOStream( bool binary, bool editmode ) const
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

    if ( !iscomm_ && hostname_.isEmpty() )
    {
        std::ios_base::openmode openmode = std::ios_base::out;
        if ( binary )
            openmode = openmode | std::ios_base::binary;

        if ( editmode )
            openmode = openmode | std::ios_base::in;

#ifdef __msvc__
	sd.ostrm = new std::winofstream( sd.fileName(), openmode );
#else
	sd.ostrm = new std::ofstream( sd.fileName(), openmode );
#endif

	if ( !sd.ostrm->good() )
	    { delete sd.ostrm; sd.ostrm = 0; }
	return sd;
    }

    BufferString cmd;
    mkOSCmd( false, cmd );

    sd.fp_ = popen( cmd, "w" );
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


bool StreamProvider::executeCommand( bool inbg, bool inconsole ) const
{
    BufferString cmd;
    mkOSCmd( true, cmd );
#ifdef __msvc__
    if ( inconsole )
	mkBatchCmd( cmd );
#endif
    return ExecOSCmd( cmd, inconsole, inbg );
}


void StreamProvider::mkBatchCmd( BufferString& comm ) const
{
    const BufferString fnm(
	    	FilePath(FilePath::getTempDir(),"odtmp.bat").fullPath() );

    FILE *fp = fopen( fnm, "wt" );
    fprintf( fp, "@echo off\n%s\npause\n", comm.buf() );
    fclose( fp );

    comm = fnm;
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
    else if ( File::exists( execnm ) )
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
	static StaticStringManager stm;
	BufferString& fullexec = stm.getString();

	fullexec = "\"";
	FilePath interpfp;

	if ( getCygDir() )
	{
	    interpfp.set( getCygDir() );
	    interpfp.add("bin").add(interp);
	}

	if ( !File::exists( interpfp.fullPath() ) )
	{
	    interpfp.set( GetSoftwareDir(0) );
	    interpfp.add("bin").add("win").add("sys").add(interp);
	}

	fullexec.add( interpfp.fullPath() ).add( "\" '" )
	    .add( FilePath(execnm).fullPath(FilePath::Unix) ).add( "'" );
	if ( args && *args )
	    fullexec.add( " " ).add( args );

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

void StreamProvider::mkOSCmd( bool forread, BufferString& cmd ) const
{
    if ( hostname_.isEmpty() )
	cmd = mGetCmd(fname_);
    else
    {
	if ( iscomm_ )
	{
	    sprintf( cmd.buf(), "%s %s %s", rshcomm_.buf(),
		     hostname_.buf(), mGetCmd(fname_) );
	}
	else
	{
	    if ( forread )
		sprintf( cmd.buf(), "%s %s cat %s",
				rshcomm_.buf(), hostname_.buf(), fname_.buf() );
	    else
		sprintf( cmd.buf(), "%s %s tee %s > /dev/null",
				rshcomm_.buf(), hostname_.buf(), fname_.buf() );
	}
    }




    if ( DBG::isOn(DBG_IO) )
    {
	BufferString msg( "About to execute: '" );
	msg += cmd;
	msg += "'";
	DBG::message( msg );
    }
}


#define mRemoteTest(act) \
    FILE* fp = popen( cmd, "r" ); \
    char c; fscanf( fp, "%c", &c ); \
    pclose( fp ); \
    act (c == '1')


bool StreamProvider::exists( int fr ) const
{
    if ( isbad_ ) return false;
    if ( iscomm_ ) return fr;

    if ( hostname_.isEmpty() )
	return fname_ == sStdIO() || fname_ == sStdErr() ? true
	     : File::exists( fname_ );

    BufferString cmd;
    sprintf( cmd.buf(), "%s %s 'test -%c %s && echo 1'", rshcomm_.buf(),
	    			hostname_.buf(), fr ? 'r' : 'w', fname_.buf() );
    mRemoteTest(return);
}


bool StreamProvider::remove( bool recursive ) const
{
    if ( isbad_ || iscomm_ ) return false;

    if ( hostname_.isEmpty() )
	return fname_ == sStdIO() || fname_ == sStdErr()
	    ? false : File::remove( fname_ );

    BufferString cmd;
    sprintf( cmd.buf(), "%s %s '/bin/rm -%s %s && echo 1'",
	      rshcomm_.buf(), hostname_.buf(), recursive ? "r" : "",
	      fname_.buf() );

    mRemoteTest(return);
}


bool StreamProvider::setReadOnly( bool yn ) const
{
    if ( isbad_ || iscomm_ ) return false;

    if ( hostname_.isEmpty() )
	return fname_ == sStdIO() || fname_ == sStdErr() ? false :
	       File::makeWritable( fname_, !yn, false );

    BufferString cmd;
    sprintf( cmd.buf(), "%s %s 'chmod %s %s && echo 1'",
	      rshcomm_.buf(), hostname_.buf(), yn ? "a-w" : "ug+w",
	      fname_.buf() );

    mRemoteTest(return);
}


bool StreamProvider::isReadOnly() const
{
    if ( isbad_ || iscomm_ ) return true;

    if ( hostname_.isEmpty() )
	return fname_ == sStdIO() || fname_ == sStdErr() ? false :
		!File::isWritable( fname_ );

    BufferString cmd;
    sprintf( cmd.buf(), "%s %s 'test -w %s && echo 1'",
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
    const bool issane = newnm && *newnm && !isbad_ && !iscomm_;

    if ( cb && cb->willCall() )
    {
	BufferString msg;
	if ( issane )
	    mkRelocMsg( fname_, newnm, msg );
	else if ( iscomm_ )
	    msg = "Cannot rename commands";
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
		    File::rename( fname_, newnm );
	else
	{
	    BufferString cmd;
	    sprintf( cmd.buf(), "%s %s '/bin/mv -f %s %s && echo 1'",
		      rshcomm_.buf(), hostname_.buf(), fname_.buf(), newnm );
	    mRemoteTest(rv =);
	}
    }

    if ( rv )
	set( newnm );
    return rv;
}
