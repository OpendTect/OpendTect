/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "filesystemaccess.h"

#include "file.h"
#include "filepath.h"
#include "od_ostream.h"
#include "oscommand.h"
#include "uistrings.h"

#include <iostream>

#ifdef __win__
# include "winutils.h"
# ifdef __msvc__
#  define popen _popen
#  define pclose _pclose
#  define fileno(s) _fileno(s)
#  include "winstreambuf.h"
# endif
#else
# include <fcntl.h>
# include <fstream>
# include "sys/stat.h"
# include <unistd.h>
#endif

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#ifdef __mac__
#define st_atim st_atimespec
#define st_ctim st_ctimespec
#define st_mtim st_mtimespec
#endif


namespace File
{

// 0=modification, 1=access, 2=creation

static QDateTime getDateTime( const char* fnm, bool followlink, int state )
{
    if ( state < 0 || state > 2 )
	return QDateTime();

    static const auto& fsa = OD::FileSystemAccess::getLocal();
    Time::FileTimeSet times;
    if ( !fsa.getTimes(fnm,times,followlink) )
	return QDateTime();

    const std::timespec time = state == 0
			? times.getModificationTime()
			: (state == 1 ? times.getAccessTime()
				      : times.getCreationTime());
    if ( mIsUdf(time.tv_sec) )
	return QDateTime();

    const od_int64 nrmsec = time.tv_sec * 1000 + time.tv_nsec / 1e6;
    return QDateTime::fromMSecsSinceEpoch( nrmsec );
}

} // namespace File



mImplFactory( OD::FileSystemAccess, OD::FileSystemAccess::factory );

static const char* sProtSep = FilePath::uriProtocolSeparator();
static const int cProtSepLen = StringView(sProtSep).size();

#define mLocalFileSystemNotInited	0
#define mLocalFileSystemIniting		1
#define mLocalFileSystemInited		2
static Threads::Atomic<int> lfsinitstate_ = mLocalFileSystemNotInited;
static const OD::FileSystemAccess* lfsinst_ = nullptr;
static ObjectSet<const OD::FileSystemAccess> systemaccesses_;


class LocalFileSystemAccess : public OD::FileSystemAccess
{ mODTextTranslationClass(LocalFileSystemAccess);
public:
			~LocalFileSystemAccess()	{}

private:
			LocalFileSystemAccess() = default;

    const char*		protocol() const override { return sFactoryKeyword(); }
    uiString		userName() const override { return sFactoryUserName(); }
    bool		readingSupported() const override { return true; }
    bool		writingSupported() const override { return true; }
    bool		queriesSupported() const override { return true; }
    bool		operationsSupported() const override { return true; }

    bool		exists(const char*) const override;
    bool		isReadable(const char*) const override;
    bool		isFile(const char*) const override;
    bool		isDirectory(const char*) const override;
    bool		isWritable(const char*) const override;
    BufferString	timeCreated(const char*,bool followlink) const override;
    BufferString	timeLastModified(const char*,
					 bool followlink) const override;
    od_int64		getTimeInMilliSeconds(const char*,bool modif,
					 bool followlink) const override;
    bool		getTimes(const char*,Time::FileTimeSet&,
				 bool followlink) const override;
    bool		setTimes(const char*,const Time::FileTimeSet&,
				 bool followlink) const override;

    bool	remove(const char*,bool recursive=true) const override;
    bool	setWritable(const char*,bool yn,bool recursive) const override;
    bool	rename(const char* from,const char* to,
		       uiString* errmsg=nullptr) const override;
    bool	copy(const char* from,const char* to,bool preserve,
		     uiString* errmsg,TaskRunner*) const override;
    od_int64	getFileSize(const char*, bool followlink) const override;
    bool	createDirectory(const char*) const override;
    bool	listDirectory(const char*,File::DirListType,
				      BufferStringSet&,
				      const char* mask) const override;

    StreamData	createOStream(const char*,bool binary,
			      bool editmode) const override;

    StreamData	createIStream(const char*,bool binary) const override;

    static OD::FileSystemAccess*	createInstance()
					{ return new LocalFileSystemAccess(); }
public:

    static void		initClass();
    static const char*	sFactoryKeyword() { return "file"; }
    static uiString	sFactoryUserName() { return tr("Local"); }

};

// Set up a mechanism to run initClass once. Since this will likely be done
// while initialising the static file and global variables, we cannot
// put this in initbasic.cc.

void LocalFileSystemAccess::initClass()
{
    while ( lfsinitstate_!=mLocalFileSystemInited )
    {
	if ( lfsinitstate_.setIfValueIs( mLocalFileSystemNotInited,
				   mLocalFileSystemIniting ) )
	{
	    // first thread to get here ...
	    FileSystemAccess::factory().addCreator( createInstance,
						      sFactoryKeyword(),
						      sFactoryUserName() );

	    lfsinst_ = new LocalFileSystemAccess;
	    systemaccesses_ += lfsinst_;
	    lfsinitstate_ = mLocalFileSystemInited;
	}
    }
}


#define mGetFileNameAndRetFalseIfEmpty() \
    const BufferString fnm = withoutProtocol( uri ); \
    if ( fnm.isEmpty() ) \
	return false


bool LocalFileSystemAccess::exists( const char* uri ) const
{
    mGetFileNameAndRetFalseIfEmpty();
    return QFile::exists( fnm.buf() );
}


bool LocalFileSystemAccess::isReadable( const char* uri ) const
{
    mGetFileNameAndRetFalseIfEmpty();
    QFileInfo qfi( fnm.buf() );
    return qfi.isReadable();
}


bool LocalFileSystemAccess::isFile( const char* uri ) const
{
    mGetFileNameAndRetFalseIfEmpty();
    QFileInfo qfi( fnm.buf() );
    return qfi.isFile();
}


bool LocalFileSystemAccess::createDirectory( const char* uri ) const
{
    mGetFileNameAndRetFalseIfEmpty();
    QDir qdir;
    return qdir.mkpath( fnm.str() );
}


bool LocalFileSystemAccess::listDirectory( const char* uri,
			File::DirListType dlt, BufferStringSet& filenames,
			const char* mask ) const
{
    if ( !isDirectory(uri) )
	return false;

    BufferString fnm = withoutProtocol( uri );

    QDir qdir( fnm.str() );
    if ( mask && *mask )
    {
	QStringList filters;
	filters << mask;
	qdir.setNameFilters( filters );
    }

    QDir::Filters dirfilters;
    if ( dlt == File::FilesInDir )
	dirfilters = QDir::Files | QDir::Hidden;
    else if ( dlt == File::DirsInDir )
	dirfilters = QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden;
    else
	dirfilters = QDir::Dirs | QDir::NoDotAndDotDot | QDir::Files
				| QDir::Hidden;

    const QStringList qlist = qdir.entryList( dirfilters );
    for ( int idx=0; idx<qlist.size(); idx++ )
	filenames.add( qlist[idx] );

    return true;
}


bool LocalFileSystemAccess::isDirectory( const char* uri ) const
{
    mGetFileNameAndRetFalseIfEmpty();
    QFileInfo qfi( fnm.str() );
    if ( qfi.isDir() )
	return true;

    BufferString lnkfnm( fnm, ".lnk" );
    qfi.setFile( lnkfnm.str() );
    return qfi.isDir();
}



bool LocalFileSystemAccess::remove( const char* uri, bool recursive ) const
{
    mGetFileNameAndRetFalseIfEmpty();
    if ( isFile(fnm) || File::isLink(fnm) )
	return QFile::remove( fnm.str() );

    if ( recursive && isDirectory( fnm ) )
    {
	QDir dir( fnm.buf() );
	return dir.removeRecursively();
    }

    return false;
}


bool LocalFileSystemAccess::setWritable( const char* uri, bool yn,
					 bool recursive ) const
{
    mGetFileNameAndRetFalseIfEmpty();

    OS::MachineCommand mc;
    if ( __iswin__ )
    {
	mc.setProgram( "ATTRIB" );
	mc.addArg( yn ? "-R" : "+R" ).addArg( fnm );
	if ( recursive && isDirectory(fnm) )
	    mc.addArg( "/S" ).addArg( "/D" );
    }
    else
    {
	mc.setProgram( "chmod" );
	if ( recursive && isDirectory(fnm) )
	    mc.addArg( "-R" );
	mc.addArg( yn ? "ug+w" : "a-w" ).addArg( fnm );
    }

    return mc.execute();
}


bool LocalFileSystemAccess::isWritable( const char* uri ) const
{
    mGetFileNameAndRetFalseIfEmpty();
    const QFileInfo qfi( fnm.buf() );
    const bool iswritable = qfi.isWritable();
#ifdef __unix__
    return iswritable;
#else
    if ( !iswritable || WinUtils::IsUserAnAdmin() )
	return iswritable;

    return WinUtils::pathContainsTrustedInstaller(fnm)
		? false : iswritable;
#endif
}


bool LocalFileSystemAccess::rename( const char* fromuri,
				    const char* touri, uiString* errmsg ) const
{
    const BufferString from = withoutProtocol( fromuri );
    const BufferString to = withoutProtocol( touri );
    if ( from.isEmpty() || to.isEmpty() )
    {
	if ( errmsg )
	{
	    if ( from.isEmpty() )
		errmsg->appendPhrase( tr("Empty target path found") );
	    else
		errmsg->appendPhrase( tr("Empty destination path found") );
	}

	return false;
    }

    const char* oldname = from.buf();
    const char* newname = to.buf();

    if ( !exists(oldname) )
    {
	if ( errmsg )
	    errmsg->append( uiStrings::phrDoesntExist(::toUiString(oldname)) );

	return false;
    }

    const FilePath destpath( newname );
    if ( destpath.exists() )
    {
	if ( errmsg )
	{
	    uiString msg = tr( "Destination '%1' already exists" )
					.arg( destpath.fullPath() );
	    msg.appendPhrase( tr("Please remove or rename manually") );
	    errmsg->appendPhrase( msg );
	}

	return false;
    }

    const FilePath destdir( destpath.pathOnly() );
    const BufferString targetbasedir( destdir.fullPath() );
    if ( !exists(targetbasedir) )
    {
	if ( !File::createDir(targetbasedir) )
	{
	    if ( errmsg )
		errmsg->append( uiStrings::phrCannotCreateDirectory(
						toUiString(targetbasedir)) );

	    return false;
	}
    }
#ifdef __unix__
    else if ( !isWritable(targetbasedir) )
    {
	if ( errmsg )
	    errmsg->append(uiStrings::phrCannotWrite(
						toUiString(targetbasedir)));

	return false;
    }
#endif

    int itatr=0;
    int nritatr = 10;
    bool res=false;
    while ( ++itatr < nritatr )
    {
	res = QFile::rename( oldname, newname );
	if ( res )
	    return true;
	else if ( !exists(oldname) && exists(newname) )
	{// False negative detection.
	    return true;
	}

	if ( !res && isDirectory(oldname) )
	{
	    QDir dir;
	    res = dir.rename( oldname, newname );
	    if ( res )
		return true;
	    else if ( !exists(oldname) && exists(newname) )
		return true;
	    else
	    {
		const FilePath sourcefp( oldname );
		dir.setCurrent( QString(sourcefp.pathOnly()) );
		const QString newnm = dir.relativeFilePath( newname );
		res = dir.rename( QString(sourcefp.fileName()), newnm );
		if ( res )
		    return true;
		else if ( !exists(oldname) && exists(newname) )
		    return true;
	    }
	}

	Threads::sleep(0.1);
    }

    if ( !res )
    { //Trying c rename function
	::rename( oldname, newname );
	if ( !exists(oldname) && exists(newname) )
	    return true;
    }

    OS::MachineCommand mc;
    if ( !__iswin__ )
	mc.setProgram("mv");
    else
    {
	const BufferString destdrive = destdir.rootPath();
	const FilePath sourcefp( oldname );
	const BufferString sourcedrive = sourcefp.rootPath();
	if ( destdrive != sourcedrive )
	{
	    res = File::copyDir( oldname, newname, errmsg );
	    if ( !res )
		return false;

	    res = File::removeDir( oldname );
	    return res;
	}

	mc.setProgram( "move" );
    }

    mc.addArg( oldname ).addArg( newname );
    BufferString stdoutput, stderror;
    res = mc.execute( stdoutput, &stderror );
    if ( !res && errmsg )
    {
	if ( !stderror.isEmpty() )
	    errmsg->appendPhrase( toUiString(stderror) );

	if ( !stdoutput.isEmpty() )
	    errmsg->appendPhrase( toUiString(stdoutput) );

	errmsg->appendPhrase( tr("Failed to rename using system command.") );
    }

    return res;
}


bool LocalFileSystemAccess::copy( const char* fromuri, const char* touri,
				  bool preserve, uiString* errmsg,
				  TaskRunner* taskrun ) const
{
    const BufferString from = withoutProtocol( fromuri );
    const BufferString to = withoutProtocol( touri );
    if ( from.isEmpty() || to.isEmpty() )
	return false;

    if ( (isDirectory(from) || isDirectory(to)) &&
	  !File::isLink(from) )
	return File::copyDir( from, to, preserve, errmsg, taskrun );

    if ( !File::checkDir(from,true,errmsg) || !File::checkDir(to,false,errmsg) )
	return false;

    if ( exists(to) && !isDirectory(to) )
	remove( to );

    bool ret = true;
    if ( preserve && !__iswin__ && File::isLink(from.buf()) )
    {
	const BufferString linkval( File::linkValue(from.buf()) );
	ret = File::createLink( linkval.buf(), to.buf() );
    }
    else
    {
	QFile qfile( from.buf() );
	ret = qfile.copy( to.buf() );
	if ( !ret && errmsg )
	    errmsg->setFrom( qfile.errorString() );
    }

    if ( ret && preserve && !__iswin__ ) //Not required on Windows
    {
	Time::FileTimeSet times;
	if ( getTimes(from.buf(),times,false) )
	    setTimes( to.buf(), times, false );
    }

    return ret;
}


od_int64 LocalFileSystemAccess::getFileSize( const char* uri,
					     bool followlink ) const
{
    const BufferString fnm = withoutProtocol( uri );
    if ( fnm.isEmpty() )
	return 0;

    if ( !followlink && File::isLink(fnm) )
    {
	od_int64 filesize = 0;
#ifdef __win__
	HANDLE file = CreateFile( fnm, GENERIC_READ, 0, NULL, OPEN_EXISTING,
				  FILE_ATTRIBUTE_NORMAL, NULL );
	filesize = GetFileSize( file, NULL );
	CloseHandle( file );
#else
	struct stat filestat;
	filesize = lstat( fnm, &filestat )>=0 ? filestat.st_size : 0;
#endif

	return filesize;
    }

    QFileInfo qfi( fnm.buf() );
    return qfi.size();
}


BufferString LocalFileSystemAccess::timeCreated( const char* uri,
						 bool followlink ) const
{
    const BufferString fnm = withoutProtocol( uri );
    if ( fnm.isEmpty() )
	return BufferString::empty();

    const QDateTime dt = File::getDateTime( fnm.buf(), followlink, 2 );
    return dt.toString( Qt::ISODate );
}


BufferString LocalFileSystemAccess::timeLastModified( const char* uri,
						      bool followlink ) const
{
    const BufferString fnm = withoutProtocol( uri );
    if ( fnm.isEmpty() )
	return BufferString::empty();

    const QDateTime dt = File::getDateTime( fnm.buf(), followlink, 0 );
    return dt.toString( Qt::ISODate );
}


od_int64 LocalFileSystemAccess::getTimeInMilliSeconds( const char* uri,
				bool lastmodif, bool followlink ) const
{
    const BufferString fnm = withoutProtocol( uri );
    if ( fnm.isEmpty() )
	return -1;

    Time::FileTimeSet times;
    if ( !getTimes(fnm.buf(),times,followlink) )
	return -1;

    const std::timespec time = lastmodif ? times.getModificationTime()
					 : times.getCreationTime();
    if ( mIsUdf(time.tv_sec) )
	return -1;

    return time.tv_sec * 1000 + time.tv_nsec / 1e6;
}


bool LocalFileSystemAccess::getTimes( const char* uri, Time::FileTimeSet& times,
				      bool followlink ) const
{
    const BufferString fnm = withoutProtocol( uri );
    if ( fnm.isEmpty() )
	return false;

    std::timespec modtime, acctime, crtime;
#ifdef __win__
    HANDLE hfile = CreateFile( fnm, GENERIC_READ,
			       FILE_SHARE_READ, NULL,
			       OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if ( hfile == INVALID_HANDLE_VALUE )
	return false;

    FILETIME ftmodtime, ftacctime, ftcrtime;
    if ( !GetFileTime(hfile,&ftcrtime,&ftacctime,&ftmodtime) )
    {
	CloseHandle( hfile );
	return false;
    }

    CloseHandle( hfile );
    WinUtils::FileTimeToTimet( ftmodtime, modtime );
    WinUtils::FileTimeToTimet( ftacctime, acctime );
    WinUtils::FileTimeToTimet( ftcrtime, crtime );
#else
    struct stat filestat;
    if ( followlink )
    {
	if ( stat(fnm,&filestat) != 0 )
	    return false;
    }
    else
    {
	if ( lstat(fnm,&filestat) != 0 )
	    return false;
    }

    modtime = filestat.st_mtim;
    acctime = filestat.st_atim;
    crtime = filestat.st_ctim;
#endif
    times.setModificationTime( modtime ).setAccessTime( acctime )
	 .setCreationTime( crtime );

    return true;
}


bool LocalFileSystemAccess::setTimes( const char* uri,
				      const Time::FileTimeSet& times,
				      bool followlink ) const
{
    const BufferString fnm = withoutProtocol( uri );
    if ( fnm.isEmpty() )
	return false;

    const bool hasmodtime = times.hasModificationTime();
    bool hasacctime = times.hasAccessTime();
#ifdef __win__
    FILETIME ftmodtime, ftacctime, ftcrtime;
    const bool hascrtime = times.hasCreationTime();
    if ( hasmodtime )
	WinUtils::TimespecToFileTime( times.getModificationTime(), ftmodtime );

    if ( hasacctime )
	WinUtils::TimespecToFileTime( times.getAccessTime(), ftacctime );
    else
    {
	ftacctime.dwLowDateTime = 0XFFFFFFFF;
	ftacctime.dwHighDateTime = 0XFFFFFFFF;
	hasacctime = true;
    }

    if ( hascrtime )
	WinUtils::TimespecToFileTime( times.getCreationTime(), ftcrtime );

    HANDLE hfile = CreateFile( fnm.buf(), GENERIC_WRITE,
			       FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			       OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    bool res = false;
    if ( hfile != INVALID_HANDLE_VALUE )
    {
	res = SetFileTime( hfile,
	    hascrtime ? &ftcrtime : NULL,
	    hasacctime ? &ftacctime : NULL,
	    hasmodtime ? &ftmodtime : NULL );
	CloseHandle( hfile );
    }

    return res;
#else
    std::timespec filetimes[2];
    if ( hasacctime )
	filetimes[0] = times.getAccessTime();
    else
	filetimes[0].tv_sec = UTIME_OMIT;

    if ( hasmodtime )
	filetimes[1] = times.getModificationTime();
    else
	filetimes[1].tv_sec = UTIME_OMIT;

    const int flag = followlink ? 0 : AT_SYMLINK_NOFOLLOW;
    return utimensat( 0, fnm.buf(), filetimes, flag ) == 0;
#endif
}


StreamData LocalFileSystemAccess::createOStream( const char* uri,
					bool binary, bool editmode ) const
{
    const BufferString fnm = withoutProtocol( uri );
    if ( fnm.isEmpty() )
	return StreamData();

    StreamData res;
    if ( fnm == od_stream::sStdIO() )
    {
	res.setOStrm( &std::cout );
	return res;
    }
    else if ( fnm == od_stream::sStdErr() )
    {
	res.setOStrm( &std::cerr );
	return res;
    }

    auto* impl = new StreamData::StreamDataImpl;
    impl->fname_ = uri;
    std::ios_base::openmode openmode = std::ios_base::out;
    if ( binary )
	openmode |= std::ios_base::binary;

    if ( editmode )
	openmode |= std::ios_base::in;

#ifdef __msvc__
    if ( File::isHidden(fnm.buf() ) )
	File::hide( fnm.buf(), false );

    impl->ostrm_ = new std::winofstream( fnm.buf(), openmode );
#else
    impl->ostrm_ = new std::ofstream( fnm.buf(), openmode );
#endif

    if ( !impl->ostrm_ || !impl->ostrm_->good() )
	deleteAndNullPtr( impl->ostrm_ );

    res.setImpl( impl );

    return res;
}


StreamData LocalFileSystemAccess::createIStream( const char* uri,
						 bool binary ) const
{
    BufferString fnm = withoutProtocol( uri );
    if ( fnm.isEmpty() )
	return StreamData();

    StreamData res;
    if ( fnm == od_stream::sStdIO() || fnm == od_stream::sStdErr() )
    {
	res.setIStrm( &std::cin );
	return res;
    }

    auto* impl = new StreamData::StreamDataImpl;
    impl->fname_ = uri;

    if ( !exists(fnm) )
    {
	FilePath fp( fnm );
	BufferString fullpath = fp.fullPath( FilePath::Local, true );
	if ( !exists(fullpath) )
	    fullpath = fp.fullPath( FilePath::Local, false );
	// Sometimes the filename _is_ weird, and the cleanup is wrong
	if ( exists( fullpath ) )
	    impl->fname_ = fullpath;
    }

    std::ios_base::openmode openmode = std::ios_base::in;
    if ( binary )
	openmode = openmode | std::ios_base::binary;

    deleteAndNullPtr( impl->istrm_ );
#ifdef __msvc__
    impl->istrm_ = new std::winifstream( impl->fname_, openmode );
#else
    impl->istrm_ = new std::ifstream( impl->fname_, openmode );
#endif

    if ( !impl->istrm_ || !impl->istrm_->good() )
	deleteAndNullPtr( impl->istrm_ );

    res.setImpl( impl );
    return res;
}


namespace OD
{

BufferString FileSystemAccess::getProtocol( const char* filename )
{
    BufferString res = filename;

    char* protend = res.find( sProtSep );
    if ( protend )
	*protend = 0;
    else
	res = LocalFileSystemAccess::sFactoryKeyword();

    return res;
}


BufferString FileSystemAccess::withoutProtocol( const char* uri )
{
    BufferString input( uri );
    char* protend = input.find( sProtSep );
    if ( !protend )
	return input;

    return BufferString( protend + cProtSepLen );
}


BufferString FileSystemAccess::iconForProtocol( const char* prot )
{
    return BufferString( "fileaccess_", prot );
}


BufferString FileSystemAccess::withProtocol( const char* fnm, const char* prot )
{
    if ( StringView(prot) == LocalFileSystemAccess::sFactoryKeyword() )
	prot = 0;

    if ( !fnm || !*fnm )
	return prot && *prot ? BufferString( prot, sProtSep )
			     : BufferString::empty();

    const char* protend = firstOcc( fnm, sProtSep );
    BufferString newfnm;
    if ( prot )
	newfnm.add( prot ).add( sProtSep );

    if ( !protend )
	newfnm.add( fnm );
    else
	newfnm.add( protend + cProtSepLen );

    return newfnm;
}


void FileSystemAccess::getProtocolNames( BufferStringSet& factnms, bool forread)
{
    factnms = factory().getNames();
    for ( int idx=0; idx<factnms.size(); idx++ )
    {
	const FileSystemAccess& fsa = getByProtocol( factnms.get(idx) );
	if ( (forread && !fsa.readingSupported())
	  || (!forread && !fsa.writingSupported()) )
	{
	    factnms.removeSingle( idx );
	    idx--;
	}
    }
}


bool FileSystemAccess::exists( const char* uri ) const
{
    return isReadable( uri );
}


bool FileSystemAccess::isFile( const char* uri ) const
{
    return isReadable( uri );
}


od_int64 FileSystemAccess::getFileSize( const char* uri, bool followlink ) const
{
    return isReadable(uri) ? -1 : 0;
}


const FileSystemAccess& FileSystemAccess::get( const char* fnm )
{
    BufferString protocol = getProtocol( fnm );
    return gtByProt( protocol );
}


const FileSystemAccess& FileSystemAccess::getByProtocol( const char* prot )
{
    BufferString protocol( prot );
    return gtByProt( protocol );
}


const FileSystemAccess& FileSystemAccess::getLocal()
{
    BufferString empty;
    return gtByProt( empty );
}


bool FileSystemAccess::isLocal() const
{
    return this == &getLocal();
}


const FileSystemAccess& FileSystemAccess::gtByProt( BufferString& protocol )
{
    if ( lfsinitstate_ != mLocalFileSystemInited )
	LocalFileSystemAccess::initClass();

    if ( protocol.isEmpty() )
	return *lfsinst_;

    // search for previously used instance. First exact match (e.g. "http")
    for ( int idx=0; idx<systemaccesses_.size(); idx++ )
    {
	const FileSystemAccess& item = *systemaccesses_[idx];
	if ( protocol == item.protocol() )
	    return item;
    }
    // maybe we've been passed a variant (e.g. "https" for "http")
    for ( int idx=0; idx<systemaccesses_.size(); idx++ )
    {
	const FileSystemAccess& item = *systemaccesses_[idx];
	if ( protocol.startsWith(item.protocol()) )
	    return item;
    }

    // OK, so we have not made an instantiation of the protocol yet
    // See if we have anything for it in the factory

    const BufferStringSet nms = factory().getNames();
    if ( !nms.isPresent(protocol) )
    {
	// again, we may have a variant
	for ( int idx=0; idx<nms.size(); idx++ )
	    if ( protocol.startsWith(nms.get(idx)) )
		protocol = nms.get(idx);
		// no break, we want to use the last one added
    }

    FileSystemAccess* res = factory().create( protocol );
    if ( !res )
    {
	ErrMsg( BufferString("Unknown file access protocol:\n",protocol) );
	return *lfsinst_;
    }

    systemaccesses_ += res;
    return *res;
}

} // namespace OD
