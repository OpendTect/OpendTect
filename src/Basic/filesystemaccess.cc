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
#include "winutils.h"
#include "genc.h"

#ifdef __win__
# include <direct.h>
# include "winstreambuf.h"
#else
# include "sys/stat.h"
# include <unistd.h>
#endif

# include <fstream>

#ifndef OD_NO_QT
# include <QDateTime>
# include <QDir>
# include <QFile>
# include <QFileInfo>
# include <QProcess>
#endif


mImplFactory( File::SystemAccess, File::SystemAccess::factory );

static const char* prefixsearch = "://";
static const int searchlen = strlen( prefixsearch );


namespace File
{


mExpClass(Basic) LocalFileSystemAccess : public SystemAccess
{ mODTextTranslationClass(LocalFileSystemAccess);
public:

    virtual const char*	protocol() const    { return "file"; }

    virtual bool	exists(const char*) const;
    virtual bool	isReadable(const char*) const;
    virtual bool	isFile(const char*) const;
    virtual bool	isDirectory(const char*) const;
    virtual bool	isWritable(const char*) const;

    virtual bool	remove(const char*,bool recursive=true) const;
    virtual bool	setWritable(const char*,bool yn,bool recursive) const;
    virtual bool	rename(const char* from,const char*) const;
    virtual bool	copy(const char* from,const char* to,
			     uiString* errmsg=0) const;
    virtual od_int64	getFileSize(const char*, bool followlink) const;
    virtual bool	createDirectory(const char*) const;
    virtual bool	listDirectory(const char*,DirListType,BufferStringSet&,
				      const char* mask) const;

    virtual StreamData	createOStream(const char*,
				    bool binary,bool editmode) const;

    virtual StreamData	createIStream(const char*,bool binary) const;

    static void		initClass();
    static const char*	sFactoryKeyword() { return "file"; }
    static uiString	sFactoryDisplayName() { return tr("Local"); }

    virtual const char* factoryKeyword() const { return sFactoryKeyword(); }
    virtual uiString	factoryDisplayName() const
			{ return sFactoryDisplayName(); }

private:

    static SystemAccess* createInstance() { return new LocalFileSystemAccess; }

};

} // namespace File


BufferString File::SystemAccess::getProtocol( const char* filename,
					      bool acceptnone )
{
    BufferString res = filename;
    char* prefixend = res.find( prefixsearch );
    if ( prefixend )
	*prefixend = 0;
    else
    {
	if ( !acceptnone )
	    res = LocalFileSystemAccess::sFactoryKeyword();
	else
	    res = BufferString::empty();
    }

    return res;
}


BufferString File::SystemAccess::withoutProtocol( const char* uri )
{
    BufferString input( uri );
    char* prefixend = input.find( prefixsearch );
    if ( !prefixend )
	return input;

    return BufferString( prefixend + searchlen );
}


BufferString File::SystemAccess::iconForProtocol( const char* prot )
{
    return BufferString( "fileaccess_", prot );
}


#define mDefFileSystemAccessFn(nm,result) \
bool File::SystemAccess::nm( const char* uri ) const \
{ return result; }

#define mDefFileSystemAccessFn1Arg(nm,arg,result) \
bool File::SystemAccess::nm( const char* uri, arg ) const \
{ return result; }

#define mDefFileSystemAccessFn2Args(nm,arg1,arg2,result) \
bool File::SystemAccess::nm( const char* uri, arg1, arg2 ) const \
{ return result; }

#define mDefFileSystemAccessFn3Args(nm,arg1,arg2,arg3,result) \
bool File::SystemAccess::nm( const char* uri, arg1, arg2, arg3 ) const \
{ return result; }


bool File::SystemAccess::exists( const char* uri ) const
{ return isReadable( uri ); }
bool File::SystemAccess::isFile( const char* uri ) const
{ return isReadable( uri ); }
mDefFileSystemAccessFn(		isDirectory, false )
od_int64 File::SystemAccess::getFileSize( const char* uri, bool ) const
{ return isReadable(uri) ? -1 : 0; }
mDefFileSystemAccessFn1Arg(	remove, bool, false )
mDefFileSystemAccessFn2Args(	setWritable, bool, bool, false )
mDefFileSystemAccessFn(		isWritable, false )
mDefFileSystemAccessFn1Arg(	rename, const char*, false )
mDefFileSystemAccessFn2Args(	copy, const char*, uiString*, false )
mDefFileSystemAccessFn(		createDirectory, false )
mDefFileSystemAccessFn3Args(	listDirectory, DirListType, BufferStringSet&,
						 const char*, false )


// Set up a mechanism to run initClass once. Since this will likely be done
// while initialising the static file and global variables, we cannot
// put this in initbasic.cc.

#define mLocalFileSystemNotInited	0
#define mLocalFileSystemIniting		1
#define mLocalFileSystemInited		2
static Threads::Atomic<int> lfsinitstate_ = mLocalFileSystemNotInited;
static const File::SystemAccess* lfsinst_ = 0;
static ObjectSet<const File::SystemAccess> systemaccesses_;


void File::LocalFileSystemAccess::initClass()
{
    while ( lfsinitstate_!=mLocalFileSystemInited )
    {
	if ( lfsinitstate_.setIfValueIs( mLocalFileSystemNotInited,
				   mLocalFileSystemIniting ) )
	{
	    // first thread to get here ...
	    File::SystemAccess::factory().addCreator(createInstance,
						     sFactoryKeyword(),
						     sFactoryDisplayName());

	    lfsinst_ = new File::LocalFileSystemAccess;
	    systemaccesses_ += lfsinst_;
	    lfsinitstate_ = mLocalFileSystemInited;
	}
    }
}


const File::SystemAccess& File::SystemAccess::get( const char* fnm )
{
    BufferString protocol = getProtocol( fnm, false );
    return gtByProt( protocol );
}


const File::SystemAccess& File::SystemAccess::getByProtocol( const char* prot )
{
    BufferString protocol( prot );
    return gtByProt( protocol );
}


const File::SystemAccess& File::SystemAccess::gtByProt( BufferString& protocol )
{
    if ( lfsinitstate_ != mLocalFileSystemInited )
	LocalFileSystemAccess::initClass();

    if ( protocol.isEmpty() )
	return *lfsinst_;

    // search for previously used instance. First exact match (e.g. "http")
    for ( int idx=0; idx<systemaccesses_.size(); idx++ )
    {
	const SystemAccess& item = *systemaccesses_[idx];
	if ( protocol == item.factoryKeyword() )
	    return item;
    }
    // maybe we've been passed a variant (e.g. "https" for "http")
    for ( int idx=0; idx<systemaccesses_.size(); idx++ )
    {
	const SystemAccess& item = *systemaccesses_[idx];
	if ( protocol.startsWith(item.factoryKeyword()) )
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

    SystemAccess* res = factory().create( protocol );
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


bool File::LocalFileSystemAccess::exists( const char* uri ) const
{
    mGetFileNameAndRetFalseIfEmpty();
    return QFile::exists( fnm.buf() );
}


bool File::LocalFileSystemAccess::isReadable( const char* uri ) const
{
    mGetFileNameAndRetFalseIfEmpty();
    QFileInfo qfi( fnm.buf() );
    return qfi.isReadable();
}


bool File::LocalFileSystemAccess::isFile( const char* uri ) const
{
    mGetFileNameAndRetFalseIfEmpty();
    QFileInfo qfi( fnm.buf() );
    return qfi.isFile();
}


bool File::LocalFileSystemAccess::createDirectory( const char* uri ) const
{
    mGetFileNameAndRetFalseIfEmpty();
    QDir qdir;
    return qdir.mkpath( fnm.str() );
}


bool File::LocalFileSystemAccess::listDirectory( const char* uri,
	DirListType dlt, BufferStringSet& filenames, const char* mask ) const
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
    if ( dlt == FilesInDir )
	dirfilters = QDir::Files | QDir::Hidden;
    else if ( dlt == DirsInDir )
	dirfilters = QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden;
    else
	dirfilters = QDir::Dirs | QDir::NoDotAndDotDot | QDir::Files
				| QDir::Hidden;

    QStringList qlist = qdir.entryList( dirfilters );
    for ( int idx=0; idx<qlist.size(); idx++ )
	filenames.add( qlist[idx] );

    return true;
}


bool File::LocalFileSystemAccess::isDirectory( const char* uri ) const
{
    mGetFileNameAndRetFalseIfEmpty();
    QFileInfo qfi( fnm.str() );
    if ( qfi.isDir() )
	return true;

    BufferString lnkfnm( fnm, ".lnk" );
    qfi.setFile( lnkfnm.str() );
    return qfi.isDir();
}



bool File::LocalFileSystemAccess::remove( const char* uri,
					  bool recursive ) const
{
    mGetFileNameAndRetFalseIfEmpty();
    if ( isFile(fnm) || isLink(fnm) )
	return QFile::remove( fnm.str() );

    if ( recursive && isDirectory( fnm ) )
    {
#if QT_VERSION >= 0x050000
	QDir dir( fnm.buf() );
	return dir.removeRecursively();
#else
# ifdef __win__
	return winRemoveDir( fnm );
# else
	BufferString cmd;
	cmd = "/bin/rm -rf";
	cmd.add(" \"").add(fnm).add("\"");
	bool res = QProcess::execute( QString(cmd.buf()) ) >= 0;
	if ( res ) res = !exists(fnm);
	return res;
# endif
#endif
    }

    return false;
}


bool File::LocalFileSystemAccess::setWritable( const char* uri, bool yn,
					       bool recursive ) const
{
    mGetFileNameAndRetFalseIfEmpty();

#ifdef OD_NO_QT
    return false;
#else
    BufferString cmd;
# ifdef __win__
    cmd = "attrib"; cmd += yn ? " -R " : " +R ";
    cmd.add("\"").add(fnm).add("\"");
    if ( recursive && isDirectory(fnm) )
	cmd += "\\*.* /S ";
# else
    cmd = "chmod";
    if ( recursive && isDirectory(fnm) )
	cmd += " -R ";
    cmd.add(yn ? " ug+w \"" : " a-w \"").add(fnm).add("\"");
# endif

    return QProcess::execute( QString(cmd.buf()) ) >= 0;
#endif


}


bool File::LocalFileSystemAccess::isWritable( const char* uri ) const
{
    mGetFileNameAndRetFalseIfEmpty();
    const QFileInfo qfi( fnm.buf() );
    return qfi.isWritable();
}


bool File::LocalFileSystemAccess::rename( const char* fromuri,
					  const char* touri ) const
{
    const BufferString from = withoutProtocol( fromuri );
    const BufferString to = withoutProtocol( touri );
    if ( from.isEmpty() || to.isEmpty() )
	return false;

    return QFile::rename( from.buf(), to.buf() );
}


bool File::LocalFileSystemAccess::copy( const char* fromuri,
					const char* touri,
					uiString* errmsg ) const
{
    const BufferString from = withoutProtocol( fromuri );
    const BufferString to = withoutProtocol( touri );
    if ( from.isEmpty() || to.isEmpty() )
	return false;

    if ( isDirectory(from) || isDirectory(to)  )
	return copyDir( from, to, errmsg );

    uiString errmsgloc;
    if ( !File::checkDirectory(from,true,errmsg ? *errmsg : errmsgloc) ||
	 !File::checkDirectory(to,false,errmsg ? *errmsg : errmsgloc) )
	return false;

    if ( exists(to) && !isDirectory(to) )
	remove( to );

    QFile qfile( from.buf() );
    const bool ret = qfile.copy( to.buf() );
    if ( !ret && errmsg )
	errmsg->setFrom( qfile.errorString() );

    return ret;
}


od_int64 File::LocalFileSystemAccess::getFileSize( const char* uri,
						   bool followlink ) const
{
    const BufferString fnm = withoutProtocol( uri );
    if ( fnm.isEmpty() )
	return 0;

    if ( !followlink && isLink(fnm) )
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



StreamData File::LocalFileSystemAccess::createOStream(const char* uri,
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
    if ( isHidden(fnm.buf() ) )
	hide( fnm.buf(), false );

    impl->ostrm_ = new std::winofstream( fnm.buf(), openmode );
#else
    impl->ostrm_ = new std::ofstream( fnm.buf(), openmode );
#endif

    if ( !impl->ostrm_ || !impl->ostrm_->good() )
	deleteAndZeroPtr( impl->ostrm_ );

    res.setImpl( impl );

    return res;
}


StreamData File::LocalFileSystemAccess::createIStream(const char* uri,
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
	File::Path fp( fnm );
	BufferString fullpath = fp.fullPath( File::Path::Local, true );
	if ( !exists(fullpath) )
	    fullpath = fp.fullPath( File::Path::Local, false );
	// Sometimes the filename _is_ weird, and the cleanup is wrong
	if ( exists( fullpath ) )
	    impl->fname_ = fullpath;
    }

    std::ios_base::openmode openmode = std::ios_base::in;
    if ( binary )
	openmode = openmode | std::ios_base::binary;


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
