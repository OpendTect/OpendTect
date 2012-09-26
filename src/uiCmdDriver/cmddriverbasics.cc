/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          January 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "cmddriverbasics.h"

#include "uimainwin.h"
#include "errh.h"
#include "filepath.h"


namespace CmdDrive
{


#define mReturnIfReadOnly( retval ) \
    if ( !bufstr_ ) \
    { \
	pErrMsg( "StringProcessor: Attempt to write read-only string" ); \
	return retval; \
    }

const char* StringProcessor::parseDQuoted( BufferString& unquotedstr ) const
{
    return parseBracketed( unquotedstr, '"', '"' );
}


const char* StringProcessor::parseBracketed( BufferString& unbracketedstr,
				       char beginchar, char endchar ) const
{
    unbracketedstr = "";
    const char* start = constptr_;
    mSkipBlanks( start );
    if ( *start != beginchar )
	return 0;

    start += 1;
    const char* stop = start;
    int nestdepth = 1;
    while ( true )
    {
	if ( !*stop )
	    return 0;

	if ( *stop==endchar && !isEscapedSymbol(stop) )
	    nestdepth--;
	else if ( *stop==beginchar && !isEscapedSymbol(stop) )
	    nestdepth++;

	if ( !nestdepth )
	    break;

	stop++;
    }

    unbracketedstr.setBufSize( stop - start + 1 );
    char* ptrbuf = unbracketedstr.buf();
    while ( start != stop )
	*ptrbuf++ = *start++;
    *ptrbuf = '\0';

    return *stop ? stop + 1 : stop;
}


int StringProcessor::consecutiveDQuoted( const char** remnant ) const
{
    if ( remnant )
	*remnant = constptr_;
    BufferString dummy;
    const char* tailstr = parseDQuoted(dummy);
    return tailstr ? StringProcessor(tailstr).consecutiveDQuoted(remnant)+1 : 0;
}


int StringProcessor::nrDQuotes() const
{
    int nrdquotes = 0;
    const char* ptr = constptr_;
    while ( *ptr )
    {
	if ( *ptr=='"' && !isEscapedSymbol(ptr) )
	    nrdquotes++;

	ptr++;
    }
    return nrdquotes;
}


int StringProcessor::removeNumAppendix() 
{
    mReturnIfReadOnly( 0 );

    char* wicketptr = bufstr_->buf() + bufstr_->size();

    while ( *wicketptr!='#' || isEscapedSymbol(wicketptr) )
    {
	if ( wicketptr == bufstr_->buf() )
	    return 0;
	wicketptr--;
    }
    
    char* endptr;
    double num = strtod( wicketptr+1, &endptr ); 
    
    if ( endptr == wicketptr+1 )
	return 0;
    mSkipBlanks( endptr );
    if ( *endptr != '\0' )
	return 0;
    
    *wicketptr = '\0';
    return num && num==mNINT32(num) ? mNINT32(num) : mUdf(int);
}


bool StringProcessor::removeTokenAppendix( char token )
{
    mReturnIfReadOnly( 0 );

    char* tokenptr = bufstr_->buf() + bufstr_->size() - 1;

    if ( !*bufstr_->buf() || *tokenptr!=token || isEscapedSymbol(tokenptr) )
	return false;

    *tokenptr = '\0';
    return true;
}


const char* StringProcessor::nextAction( BufferString& action ) const
{
    action = constptr_;
    const char* ptr = constptr_;

    mSkipBlanks( ptr );
    if ( *ptr == '#' )
	while ( *ptr ) ptr++;

    while ( *ptr && *ptr!=';' )
    {
	BufferString dummy;
	const char* gotoptr = StringProcessor(ptr).parseDQuoted( dummy );
	if ( !gotoptr )
	    gotoptr = StringProcessor(ptr).parseBracketed( dummy );

	if ( gotoptr )
	    ptr = gotoptr;
	else if ( *ptr!='"' && *ptr!='[' )
	    ptr++;
	else
	    while ( *ptr ) ptr++; 
    }

    *( action.buf() + (ptr-constptr_) ) = '\0';
    return *ptr ? ptr+1 : ptr;
}


const char* StringProcessor::findAssignment( const char* tokens ) const
{
    const char* endptr = constptr_;
    mSkipBlanks( endptr );
    mSkipNonBlanks( endptr );
    mSkipBlanks( endptr );

    const char* ptr = constptr_;
    while ( *ptr && ptr<=endptr )
    {
	if ( strchr(tokens, *ptr) )
	    return ptr;

	ptr++;
    }

    return 0;
}


const char* StringProcessor::parseIdentifier( BufferString& name ) const
{
    name = "";
    const char* ptr = constptr_;
    mSkipBlanks( ptr );
    mUnscope( ptr, unscopedptr );

    if ( !isalpha(*unscopedptr) )
	return 0;

    while ( ptr<unscopedptr || isalnum(*ptr) || *ptr=='_' )
	mAddCharToBufStr( name, *ptr++ );

    return ptr;
}


bool StringProcessor::convertToInt( int* val ) const
{
    if ( !constptr_ || isspace(*constptr_) )
	return false;

    BufferString unescapedstr( constptr_ );
    StringProcessor(unescapedstr).removeCmdFileEscapes();

    char* remnant = 0;
    const int num = strtol( unescapedstr.buf(), &remnant, 0 );

    if ( !remnant || *remnant || remnant==unescapedstr.buf() )
	return false;

    if ( val )
	*val = num;

    return true;
}


bool StringProcessor::convertToDouble( double* val ) const
{
    int intval;
    if ( convertToInt(&intval) )
    {
	if ( val )
	    *val = intval;

	return true;
    }

    if ( !constptr_ || isspace(*constptr_) )
	return false;

    BufferString unescapedstr( constptr_ );
    StringProcessor(unescapedstr).removeCmdFileEscapes();

    char* remnant = 0;
    const double num = strtod( unescapedstr.buf(), &remnant );

    if ( !remnant || *remnant || remnant==unescapedstr.buf() )
	return false;

    if ( val )
	*val = num;

    return true;
}


#define mAddToFMS( fms, startptr, endptr, acceptempty ) \
{ \
    *endptr = '\0'; \
    mTrimBlanks( startptr ); \
    if (*(startptr) || (acceptempty)) fms += startptr; \
    startptr = endptr+1; \
}


char StringProcessor::preParseProcedure( FileMultiString& keyfms,
					 FileMultiString& parfms ) const
{
    keyfms.setEmpty();
    parfms.setEmpty();

    BufferString bufcopy = constptr_;
    char* startptr = bufcopy.buf();
    char* endptr = bufcopy.buf();

    int nestdepth = 0;
    int stagenr = 0;

    while  ( *endptr )
    {
	if ( stagenr == 0 )
	{
	    if ( *endptr == '?' )
	    {
		mAddToFMS( parfms, startptr, endptr, true );
		stagenr++;
	    }
	    else if ( *endptr == '(' )
	    {
		parfms += "_dummyvar";
		endptr = startptr;
		stagenr++;
	    }
	}
	else if ( stagenr == 1 )
	{
	    if ( *endptr == '(' )
	    {
		mAddToFMS( keyfms, startptr, endptr, true );
		stagenr++;
	    }
	}
	else if ( stagenr == 2 )
	{
	    if ( *endptr == '(' )
		nestdepth++;
	    else if ( nestdepth && *endptr == ')' )
		nestdepth--;
	    else if ( !nestdepth && (*endptr==',' || *endptr==')') )
	    {
		if ( *endptr == ')' )
		{
		    keyfms += parfms.size();
		    stagenr++;
		}
		mAddToFMS( parfms, startptr, endptr, true );
	    }
	    else if ( *endptr == '"' )
	    {
		BufferString dummy;
		const char* ptr = endptr;
		ptr = StringProcessor(ptr).parseDQuoted( dummy );
		if ( !ptr )
		    return '"';

		endptr = const_cast<char*>( ptr );
		continue;
	    }
	}
	else if ( stagenr == 3 )
	{
	    if ( isspace(*endptr) )
		mAddToFMS( parfms, startptr, endptr, false );
	}

	endptr++;
    }

    if ( stagenr == 3 )
    {
	keyfms += parfms.size() - atoi(keyfms[2]);
	mAddToFMS( parfms, startptr, endptr, false );
	return 0;
    }

    return stagenr<2 ? '(' : ')';
}


void StringProcessor::makeDirSepIndep( int startpos )
{
    mReturnIfReadOnly();

    const char* winsep = FilePath::dirSep( FilePath::Windows );
    const char* unixsep = FilePath::dirSep( FilePath::Unix );

    char* ptr = bufstr_->buf();
    if ( startpos>0 && startpos<bufstr_->size() )
	ptr += startpos;

    while ( ptr>=bufstr_->buf() && (*ptr!='"' || isEscapedSymbol(ptr)) )
	ptr--;

    while ( *(++ptr) && (*ptr!='"' || isEscapedSymbol(ptr)) )
    {
	if ( *ptr==*winsep || *ptr==*unixsep )
	   *ptr = *FilePath::dirSep(FilePath::Local); 
    }
}


void StringProcessor::addCmdFileEscapes( const char* extraescapesymbols )
{
    mReturnIfReadOnly();

    bufstr_->setBufSize( 2*bufstr_->size()+1 );

    char* writeptr = bufstr_->buf();
    BufferString readcopy( writeptr );
    const char* readptr = readcopy.buf();

    while ( *readptr )
    {
	if ( *readptr == '@' )
	{
	    if ( mIsSymbol(*(readptr+1)) )
		*writeptr++ = '@';
	}
	else if ( strchr(extraescapesymbols, *readptr) )
	{
	    if ( *readptr == '<' )
	    {
		if ( !*(readptr+1) )
		    *writeptr++ = '@';
	    }
	    else
		*writeptr++ = '@';
	}
	*writeptr++ = *readptr++;
    }
    *writeptr = '\0';
}


bool StringProcessor::isEscapedSymbol( const char* charptr ) const
{
    if ( !mIsSymbol(*charptr) || constptr_>charptr-1 || *(charptr-1)!='@' )
	return false;
    if ( constptr_>charptr-2 || *(charptr-2)!='@' )
	return true;

    return isEscapedSymbol( charptr-2 );
}


void StringProcessor::removeCmdFileEscapes()
{
    mReturnIfReadOnly();

    char* writeptr = bufstr_->buf();
    const char* readptr = writeptr;

    while ( *readptr )
    {
	if ( *readptr=='@' && mIsSymbol(*(readptr+1)) )
	    readptr++;

	*writeptr++ = *readptr++;
    }
    *writeptr = '\0';
}


const char* StringProcessor::getCharElement( int idx ) const
{
    const char* ptr = constptr_;
    static char res[3] = "";

    if ( idx<0 )
	return res;

    while ( *ptr++ )
    {
	if ( *ptr && isEscapedSymbol(ptr) )
	{
	    if ( !idx-- )
	    {
		res[0] = '@'; res[1] = *ptr; res[2] = '\0';
		return res;
	    }
	    ptr++;
	}
	else if ( !idx-- )
	{
	    res[0] = *(ptr-1); res[1] = '\0';
	}
    }

    return res;
}


int StringProcessor::nrCharElements() const
{
    const char* ptr = constptr_;
    int nrcharelems = 0;
    while ( *ptr++ )
    {
	nrcharelems++;
	if ( *ptr && isEscapedSymbol(ptr) )
	    ptr++;
    }
    return nrcharelems;
}


void StringProcessor::appendCharElements( const char* tail )
{
    mReturnIfReadOnly();

    const char* last = bufstr_->buf() + bufstr_->size() - 1;
    if ( *bufstr_->buf() && *last=='@' && !isEscapedSymbol(last) )
	bufstr_->add( mIsSymbol(*tail) ? "@" : "" );

    bufstr_->add( tail );
}


void StringProcessor::filterAmpersands()
{
    mReturnIfReadOnly();

    replaceString( bufstr_->buf(), "&&", "\f" );
    removeCharacter( bufstr_->buf(), '&' );
    replaceCharacter( bufstr_->buf(), '\f', '&' );
}


void StringProcessor::addFileMultiStrEscapes()
{
    mReturnIfReadOnly();

    bufstr_->setBufSize( 2*bufstr_->size()+1 );
    replaceString( bufstr_->buf(), "\\", "\\\\" );
    replaceString( bufstr_->buf(), "@@", "\f" );
    replaceString( bufstr_->buf(), "@`", "@\\`" );
    replaceString( bufstr_->buf(), "\f", "@@" );
}


void StringProcessor::removeExtraSpace()
{
    mReturnIfReadOnly();

    char* writeptr = bufstr_->buf();
    const char* readptr = writeptr;

    while ( *readptr ) 
    {
	if ( isspace(*readptr) )
	{
	    readptr++;
	    if ( !isspace(*readptr) && writeptr!=bufstr_->buf() && *readptr )
		*writeptr++ = ' ';
	    continue;
	}
	*writeptr++ = *readptr++;
    }
    *writeptr = '\0';
}


void StringProcessor::capitalize( bool yn )
{
    mReturnIfReadOnly();

    char* ptr = bufstr_->buf();

    while ( *ptr )
    {
	*ptr = char( yn ? toupper(*ptr) : tolower(*ptr) );
	ptr++;
    }
}


void StringProcessor::removeTrailingDots()
{
    mReturnIfReadOnly();

    char* ptr = bufstr_->buf() + bufstr_->size();
    while ( ptr != bufstr_->buf() )
    {
	ptr--;
	if ( *ptr!='.' && !isspace(*ptr) )
	    return;

	*ptr = '\0';
    }
}


void StringProcessor::cleanUp()
{
    removeTrailingDots();
    removeExtraSpace();
}


char StringProcessor::stripOuterBrackets( const char* beginsymbols,
					  const char* endsymbols )
{
    mReturnIfReadOnly( '\0' );

    BufferString bufcopy = bufstr_->buf();
    char* firstptr = bufcopy.buf();
    mTrimBlanks( firstptr );

    const char* bracketptr = strchr( beginsymbols, *firstptr );
    if ( !bracketptr )
	return '\0';

    int bracketidx = (int) (bracketptr - beginsymbols);
    char* lastptr = firstptr + strlen(firstptr) - 1;
    if ( bracketidx>=strlen(endsymbols) || *lastptr!=endsymbols[bracketidx] )
	return '\0';

    *firstptr = ' ';
    *lastptr = ' '; 
    mTrimBlanks( firstptr );
    *bufstr_ = firstptr;
    return *bracketptr;
}


const char* windowTitle( const uiMainWin* applwin, const uiMainWin* uimw,
			 int aliasnr )
{
    if ( applwin && uimw==applwin )
    {
	if ( aliasnr == 0 )
	    return uimw->name();
	if ( aliasnr == 1 )
	    return uimw->caption( true );
    }

    if ( aliasnr > 0 )
	return 0;

    if ( !uimw )
    {
	if ( uiMainWin::activeModalType() == uiMainWin::Main )
	    return 0;
	return uiMainWin::activeModalQDlgTitle();
    }

    return uimw->caption( true );
}


bool isCmdDriverWindow( const uiMainWin* uimw )
{
    if ( !uimw )
	return false;

    if ( !strcmp(uimw->name(), controllerTitle()) )
	return true;

    uiParent* parent = const_cast<uiParent*>( uimw->parent() );
    return isCmdDriverWindow( parent ? parent->mainwin() : 0 );
}


const char* controllerTitle()
{ return "Command controller (Ctrl-R)"; }


const uiMainWin* WindowStack::topWin( bool ignorecmdddriverwins ) const
{
    const_cast<WindowStack*>(this)->synchronize();

    for ( int idx=0; idx<winstack_.size(); idx++ )
    {
	if ( !ignorecmdddriverwins || !isCmdDriverWindow(winstack_[idx]) )
	    return winstack_[idx];
    }
    return 0;
}


bool WindowStack::moveToTop( const uiMainWin* curwin )
{
    synchronize();
    return moveToTopWithoutSync( curwin );
}


bool WindowStack::moveFrameToTop( const uiMainWin* framewin )
{
    synchronize();

    int stackidx = winstack_.indexOf( framewin );
    while ( stackidx>0 && winstack_[stackidx-1]->isModal() )
	stackidx--;

    return moveToTopWithoutSync( winstack_[stackidx] );
}


bool WindowStack::moveToTopWithoutSync( const uiMainWin* curwin )
{
    const int stackidx = winstack_.indexOf( curwin );

    if ( stackidx==0 )
	return true;
    if ( stackidx<0 || winstack_[stackidx-1]->isModal() )
	return false;

    int framecnt = 0;
    while ( true )
    {
	const uiMainWin* uimw = winstack_.remove( stackidx+framecnt );
	winstack_.insertAt( uimw, framecnt );
	if ( !uimw->isModal() )
	    break;

	framecnt++;
    }

    return true;
}


void WindowStack::synchronize()
{
    ObjectSet<uiMainWin> windowlist;
    uiMainWin::getTopLevelWindows( windowlist, false );

    for ( int idx=winstack_.size()-1; idx>=0; idx-- )
    {
	if ( windowlist.indexOf(winstack_[idx]) < 0 )
	    winstack_.remove( idx );
    }

    for ( int idx=0; idx<windowlist.size(); idx++ )
    {
	uiMainWin* curwin = windowlist[idx];

	if ( winstack_.indexOf(curwin) >= 0 )
	    continue;

	if ( !curwin->isModal() )
	    winstack_ += curwin;
	else
	{
	    if ( curwin->parent() && curwin->parent()->mainwin() )
	    {
		const uiMainWin* parentwin = curwin->parent()->mainwin();
		int stackidx = winstack_.indexOf( parentwin );
		if ( stackidx >= 0 )
		{
		    while ( stackidx && winstack_[stackidx-1]->isModal() )
			stackidx--;

		    winstack_.insertAt( curwin, stackidx );
		    moveToTopWithoutSync( curwin );
		    continue;
		}
	    }
	    pErrMsg( "CmdDriver found broken window branch" );
	    winstack_.insertAt( curwin, 0 );	// Let's hope for the best
	}
    }

    for ( int idx=winstack_.size()-1; idx>=0; idx-- )
    {
	if ( winstack_[idx]->isHidden() )
	    winstack_.remove( idx );
    }
}


}; // namespace CmdDrive
