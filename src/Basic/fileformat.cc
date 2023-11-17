/*+
________________________________________________________________________

 Copyright:	(C) 1995-2023 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "fileformat.h"
#include "filepath.h"


const FileFormat& FileFormat::allFiles()
{
    static FileFormat* fmt = nullptr;
    if ( !fmt )
	fmt = new FileFormat( tr("All files"), "" );

    return *fmt;
}

const FileFormat& FileFormat::parFiles()
{
    static FileFormat* fmt = nullptr;
    if ( !fmt )
	fmt = new FileFormat( tr("Parameter files"), sParFileExtension() );

    return *fmt;
}

const FileFormat& FileFormat::zipFiles()
{
    static FileFormat* fmt = nullptr;
    if ( !fmt )
	fmt = new FileFormat( tr("Zip files"), "zip" );

    return *fmt;
}

const FileFormat& FileFormat::hdf5Files()
{
    static FileFormat* fmt = nullptr;
    if ( !fmt )
	fmt = new FileFormat( tr("HDF5 files"), "h5", "hdf5", "hdf" );

    return *fmt;
}

const FileFormat& FileFormat::textFiles()
{
    static FileFormat* fmt = nullptr;
    if ( !fmt )
	fmt = new FileFormat( tr("Text files"), "txt", "dat" );

    return *fmt;
}

const FileFormat& FileFormat::imageFiles()
{
    static FileFormat* fmt = nullptr;
    if ( !fmt )
	fmt = new FileFormat( tr("Image files"), "png", "jpg", "jpeg" );

    return *fmt;
}

const FileFormat& FileFormat::shlibFiles()
{
    static FileFormat* winfmt = nullptr;
    static FileFormat* macfmt = nullptr;
    static FileFormat* othfmt = nullptr;
    if ( !winfmt )
	winfmt = new FileFormat( tr("DLL files"), "dll" );
    if ( !macfmt )
	macfmt = new FileFormat( tr("Dynamic Libs"), "dylib" );
    if ( !othfmt )
	othfmt = new FileFormat( tr("Shared Libraries"), "so" );
#ifdef __win__
    return *winfmt;
#else
# ifdef __mac__
    return *macfmt;
# else
    return *othfmt;
# endif
#endif
}


// tradstr ('traditional') would be either a 'filter', or just an extension

FileFormat::FileFormat( const char* tradstr )
{
    BufferString bs( tradstr );
    if ( bs.isEmpty() )
	return;

    char* startptr = bs.getCStr();
    mTrimBlanks( startptr );
    char* ptr = firstOcc( startptr, '(' );
    if ( !ptr )
	ptr = startptr;
    else
    {
	*ptr++ = '\0';
	usrdesc_ = toUiString( startptr );
	if ( !ptr || !*ptr )
	    return;
    }

    startptr = ptr;
    ptr = firstOcc( startptr, ')' );
    if ( ptr )
	*ptr = '\0';
    while ( true )
    {
	mSkipBlanks( startptr );
	if ( !*startptr )
	    break;
	ptr = startptr;
	mSkipNonBlanks( ptr );
	if ( *ptr )
	    *ptr++ = '\0';

	// startptr could now be pointing to a "*.xx" of just an extension
	if ( *startptr == '*' )
	    startptr += 2;
	addExtension( startptr );
	if ( usrdesc_.isEmpty() )
	    usrdesc_ = toUiString( BufferString( startptr, " files" ) );
	startptr = ptr;
    }
}


FileFormat::FileFormat( const uiString& ud, const char* ext, const char* ext2,
		      const char* ext3 )
    : usrdesc_(ud)
{
    if ( ext )
	addExtension( ext );
    if ( ext2 )
	addExtension( ext2 );
    if ( ext3 )
	addExtension( ext3 );
}


#define mGetCleanExt() \
    if ( !ext ) \
	ext = ""; \
    while ( *ext && *ext == '.' ) \
	ext++


void FileFormat::addExtension( const char* ext )
{
    mGetCleanExt();
    exts_.addIfNew( ext );
}


bool FileFormat::hasExtension( const char* ext ) const
{
    mGetCleanExt();
    return exts_.isPresent( ext, OD::CaseInsensitive );
}


const char* FileFormat::extension( int idx ) const
{
    if ( !exts_.validIdx(idx) )
    {
	pErrMsg("Invalid idx (ext)");
	return "";
    }

    return exts_.get( idx ).str();
}


BufferString FileFormat::getFileFilter() const
{
    BufferString ret( toString(usrdesc_), " (" );
    for ( int iext=0; iext<exts_.size(); iext++ )
    {
	if ( iext > 0 )
	    ret.addSpace();

	const BufferString& ext = exts_.get( iext );
	if ( !ext.isEmpty() )
	    ret.add( "*." ).add( ext );
	else
	{
	    ret.add( "*" );
#ifdef __win__
	    ret.add( " *.*" );
#endif
	}
    }

    ret.add( ')' );
    return ret;
}


FileFormatList::FileFormatList( const FileFormat& fmt )
{
    addFormat( fmt );
}


FileFormatList::FileFormatList( const char* trad_str )
{
    BufferString workstr( trad_str );
    if ( workstr.isEmpty() )
	return;

    char* startptr = workstr.getCStr();
    mTrimBlanks( startptr );
    if ( !*startptr )
	return;

    while ( true )
    {
	mSkipBlanks( startptr );
	if ( !*startptr )
	    break;

	char* ptr = firstOcc( startptr, ';' );
	if ( ptr )
	{
	    *ptr = '\0';
	    if ( *ptr++ == ';' )
		*ptr++ = '\0';
	}

	addFormat( startptr );

	if ( !ptr )
	    break;

	startptr = ptr;
    }
}


FileFormatList::FileFormatList( const uiString& ud, const char* ext,
				const char* ext2, const char* ext3 )
{
    addFormat( FileFormat(ud,ext,ext2,ext3) );
}


FileFormatList::FileFormatList( const FileFormatList& oth )
{
    *this = oth;
}


FileFormatList& FileFormatList::operator =( const FileFormatList& oth )
{
    if ( this != &oth )
    {
	deepErase( fmts_ );
	deepCopy( fmts_, oth.fmts_ );
    }

    return *this;
}


int FileFormatList::indexOf( const char* ext ) const
{
    for ( int ifmt=0; ifmt<fmts_.size(); ifmt++ )
	if ( fmts_[ifmt]->exts_.isPresent(ext) )
	    return ifmt;

    for ( int ifmt=0; ifmt<fmts_.size(); ifmt++ )
	if ( fmts_[ifmt]->hasExtension(ext) )
	    return ifmt;

    return false;
}


FileFormat FileFormatList::format( int ifmt ) const
{
    if ( !fmts_.validIdx(ifmt) )
    {
	pErrMsg("Invalid list idx (fmt)");
	return FileFormat::allFiles();
    }

    return *fmts_[ifmt];
}


uiString FileFormatList::userDesc( int ifmt ) const
{
    if ( !fmts_.validIdx(ifmt) )
    {
	pErrMsg("Invalid list idx (udesc)");
	return uiString::empty();
    }

    return fmts_[ifmt]->userDesc();
}


void FileFormatList::addFormat( const uiString& ud, const char* ext )
{
    fmts_ += new FileFormat( ud, ext );
}


void FileFormatList::addFormats( const FileFormatList& oth )
{
    for ( int ifmt=0; ifmt<oth.fmts_.size(); ifmt++ )
    {
	const FileFormat* fmt = oth.fmts_[ifmt];
	for ( int iext=0; iext<oth.size(); iext++ )
	{
	    if ( isPresent(fmt->extension(iext)) )
	    {
		fmt = nullptr;
		break;
	    }
	}

	if ( fmt )
	    fmts_ += new FileFormat( *fmt );
    }
}


void FileFormatList::setEmpty()
{
    deepErase( fmts_ );
}


void FileFormatList::removeFormat( int idx )
{
    if ( fmts_.validIdx(idx) )
	delete fmts_.removeSingle( idx );
}


BufferString FileFormatList::getFileFilter( int idx ) const
{
    if ( !fmts_.validIdx(idx) )
    {
	pErrMsg("Invalid list idx (filefilt)");
	return BufferString();
    }

    return fmts_[idx]->getFileFilter();
}


BufferString FileFormatList::getFileFilters() const
{
    BufferString ret;
    for ( int ifmt=0; ifmt<fmts_.size(); ifmt++ )
    {
	if ( ifmt > 0 )
	    ret.add( ";; " );
	ret.add( fmts_[ifmt]->getFileFilter() );
    }

    return ret;
}
