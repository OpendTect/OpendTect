/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert / Kris
 Date:		2017
________________________________________________________________________

-*/

#include "filesystemaccess.h"

#include "file.h"
#include "filepath.h"
#include "od_ostream.h"
#include "oscommand.h"


#ifdef __win__
# include <iostream>
# include "winutils.h"
# ifdef __msvc__
#  define popen _popen
#  define pclose _pclose
#  define fileno(s) _fileno(s)
#  include "winstreambuf.h"
# endif
#else
# include <fstream>
# include "sys/stat.h"
# include <unistd.h>
# include <utime.h>
#endif


#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>


mImplFactory( FileSystemAccess, FileSystemAccess::factory );

static const char* sProtSep = FilePath::uriProtocolSeparator();
static const int cProtSepLen = FixedString(sProtSep).size();


class LocalFileSystemAccess : public FileSystemAccess
{ mODTextTranslationClass(LocalFileSystemAccess);
public:

    static const char*	sFactoryKeyword() { return "file"; }
    static uiString	sFactoryUserName() { return tr("Local"); }
    virtual const char* protocol() const { return sFactoryKeyword(); }
    virtual uiString	userName() const { return sFactoryUserName(); }

    virtual bool	exists(const char*) const;
    virtual bool	isReadable(const char*) const;
    virtual bool	isFile(const char*) const;
    virtual bool	isDirectory(const char*) const;
    virtual bool	isWritable(const char*) const;

    virtual bool	remove(const char*,bool recursive=true) const;
    virtual bool	setWritable(const char*,bool yn,bool recursive) const;
    virtual bool	rename(const char* from,const char* to,
				uiString* errmsg=nullptr) const;
    virtual bool	copy(const char* from,const char* to,
			     uiString* errmsg=nullptr) const;
    virtual od_int64	getFileSize(const char*, bool followlink) const;
    virtual bool	createDirectory(const char*) const;
    virtual bool	listDirectory(const char*,File::DirListType,
				      BufferStringSet&,
				      const char* mask) const;

    virtual StreamData	createOStream(const char*,
				      bool binary,bool editmode) const;

    virtual StreamData	createIStream(const char*,bool binary) const;

    static void		initClass();

private:

    static FileSystemAccess*	createInstance()
				{ return new LocalFileSystemAccess; }
};


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


BufferString FileSystemAccess::withProtocol( const char* fnm,
						const char* prot )
{
    if ( FixedString(prot) == LocalFileSystemAccess::sFactoryKeyword() )
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


void FileSystemAccess::getProtocolNames( BufferStringSet& factnms,
					   bool forread )
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
{ return isReadable( uri ); }

bool FileSystemAccess::isFile( const char* uri ) const
{ return isReadable( uri ); }

od_int64 FileSystemAccess::getFileSize( const char* uri, bool ) const
{ return isReadable(uri) ? -1 : 0; }


// Set up a mechanism to run initClass once. Since this will likely be done
// while initialising the static file and global variables, we cannot
// put this in initbasic.cc.

#define mLocalFileSystemNotInited	0
#define mLocalFileSystemIniting		1
#define mLocalFileSystemInited		2
static Threads::Atomic<int> lfsinitstate_ = mLocalFileSystemNotInited;
static const FileSystemAccess* lfsinst_ = 0;
static ObjectSet<const FileSystemAccess> systemaccesses_;


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

    const BufferString fnm = withoutProtocol( uri );

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

    QStringList qlist = qdir.entryList( dirfilters );
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



bool LocalFileSystemAccess::remove( const char* uri,
					  bool recursive ) const
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

#ifdef OD_NO_QT
    return false;
#else
# ifdef __win__
    OS::MachineCommand mc( "ATTRIB" );
    mc.addArg( yn ? "-R" : "+R" ).addArg( fnm );
    if ( recursive && isDirectory(fnm) )
	mc.addArg( "/S" ).addArg( "/D" );
# else
    OS::MachineCommand mc( "chmod" );
    if ( recursive && isDirectory(fnm) )
	mc.addArg( "-R" );
    mc.addArg( yn ? "ug+w" : "a-w" ).addArg( fnm );
# endif

    return mc.execute();
#endif


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
	return false;

    return QFile::rename( from.buf(), to.buf() );
}


bool LocalFileSystemAccess::copy( const char* fromuri,
					const char* touri,
					uiString* errmsg ) const
{
    const BufferString from = withoutProtocol( fromuri );
    const BufferString to = withoutProtocol( touri );
    if ( from.isEmpty() || to.isEmpty() )
	return false;

    if ( isDirectory(from) || isDirectory(to)  )
    {
	BufferString errstr;
	const bool res = File::copyDir( from, to, &errstr );
	if ( errmsg && !errstr.isEmpty() )
	    errmsg->set( errstr );
	return res;
    }

    if ( !File::checkDir(from,true,errmsg) || !File::checkDir(to,false,errmsg) )
	return false;

    if ( exists(to) && !isDirectory(to) )
	remove( to );

    QFile qfile( from.buf() );
    bool ret = qfile.copy( to.buf() );
    if ( !ret && errmsg )
	errmsg->setFrom( qfile.errorString() );

#ifdef __unix__
    const QFileInfo qfi( qfile );
    utimbuf timestamp;
    timestamp.actime = qfi.lastRead().toSecsSinceEpoch();
    timestamp.modtime = qfi.lastModified().toSecsSinceEpoch();
    utime( to.buf(), &timestamp );
#endif

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


StreamData LocalFileSystemAccess::createOStream( const char* uri,
					bool binary, bool editmode ) const
{
    const BufferString fnm = withoutProtocol( uri );
    if ( fnm.isEmpty() )
	return StreamData();

    StreamData res;
    StreamData::StreamDataImpl* impl = new StreamData::StreamDataImpl;
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
	deleteAndZeroPtr( impl->ostrm_ );

    res.setImpl( impl );

    return res;
}


StreamData LocalFileSystemAccess::createIStream(const char* uri,
						      bool binary ) const
{
    BufferString fnm = withoutProtocol( uri );
    if ( fnm.isEmpty() )
	return StreamData();

    StreamData res;
    StreamData::StreamDataImpl* impl = new StreamData::StreamDataImpl;
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

    deleteAndZeroPtr( impl->istrm_ );
#ifdef __msvc__
    impl->istrm_ = new std::winifstream( impl->fname_, openmode );
#else
    impl->istrm_ = new std::ifstream( impl->fname_, openmode );
#endif

    if ( !impl->istrm_ || !impl->istrm_->good() )
	deleteAndZeroPtr( impl->istrm_ );

    res.setImpl( impl );
    return res;
}
