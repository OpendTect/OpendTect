/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/


#include "bufstring.h"
#include "bufstringset.h"
#include "stringview.h"
#include "globexpr.h"
#include "iopar.h"
#include "odmemory.h"
#include "perthreadrepos.h"
#include "separstr.h"
#include "string2.h"
#include "uistring.h"

#ifndef OD_NO_QT
# include <QString>
# include <QStringList>
#endif

#include <string>


BufferString::BufferString( int sz, bool /*mknull*/ )
    : buf_(nullptr)
    , len_(0)
    , minlen_(sz)
{
    if ( sz < 1 ) return;

    setBufSize( sz );
    if ( len_ > 0 )
	OD::sysMemZero( buf_, len_ );
}


BufferString::BufferString( const BufferString& bs )
    : buf_(nullptr)
    , len_(0)
    , minlen_(bs.minlen_)
{
    if ( !bs.buf_ || !bs.len_ ) return;

    mTryAlloc( buf_, char [bs.len_] );
    if ( buf_ )
    {
	len_ = bs.len_;
#ifdef __win__
	strcpy_s( buf_, len_, bs.buf_ );
#else
	strcpy( buf_, bs.buf_ );
#endif
    }
}


BufferString::BufferString( const QString& qstr )
    : mBufferStringSimpConstrInitList
{
    add( qstr );
}


BufferString::~BufferString()
{
    destroy();
}


BufferString& BufferString::operator= ( const std::string& str )
{
    return assignTo( str.c_str() );
}


const char* BufferString::gtBuf() const
{
    if ( !buf_ )
	const_cast<BufferString*>(this)->init();

    return buf_;
}


char* BufferString::find( char c )
{ return firstOcc( getCStr(), c ); }
char* BufferString::findLast( char c )
{ return lastOcc( getCStr(), c ); }
char* BufferString::find( const char* s )
{ return firstOcc( getCStr(), s ); }
char* BufferString::findLast( const char* s )
{ return lastOcc( getCStr(), s ); }


BufferString& BufferString::setEmpty()
{
    if ( len_ != minlen_ )
	{ destroy(); init(); }
    else if ( !buf_ )
	{ pErrMsg("Probably working with deleted string"); }
    else
	buf_[0] = '\0';

    return *this;
}


BufferString& BufferString::assignTo( const char* s )
{
    if ( buf_ == s ) return *this;

    if ( !s ) s = "";
    setBufSize( sCast( unsigned int, (strlen(s) + 1) ) );
    char* ptr = buf_;
    while ( *s ) *ptr++ = *s++;
    *ptr = '\0';
    return *this;
}


BufferString& BufferString::add( char s )
{
    char tmp[2];
    tmp[0] = s; tmp[1] = '\0';
    return add( tmp );
}


BufferString& BufferString::add( const char* s )
{
    if ( s && *s )
    {
	const unsigned int newsize = strlen(s) + (buf_ ? strlen(buf_) : 0) +1;
	setBufSize( newsize );

	char* ptr = buf_;
	while ( *ptr ) ptr++;
	while ( *s ) *ptr++ = *s++;
	*ptr = '\0';
    }
    return *this;
}


BufferString& BufferString::add( const QString& qstr )
{
#ifdef OD_NO_QT
    return *this;
#else
    const QByteArray qba = qstr.toUtf8();
    return add( qba.constData() );
#endif
}


BufferString& BufferString::add( float f, int nrdec )
{
    return add( toString( f, nrdec ) );
}


BufferString& BufferString::add( double d, int nrdec )
{
    return add( toString( d, nrdec ) );
}


BufferString& BufferString::addLim( float f, int maxnrchars )
{
    return add( toStringLim( f, maxnrchars ) );
}


BufferString& BufferString::addLim( double d, int maxnrchars )
{
    return add( toStringLim( d, maxnrchars ) );
}


static const char* spc32 =
"                                ";
static const char* tab32 =
"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
static const char* nl32 =
"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";


BufferString& BufferString::addArr32Chars( const char* carr, int nr )
{
    if ( nr == 1 )
	return add( carr + 31 ); // shortcut for majority of calls
    else if ( nr < 1 )
	return *this;

    const int nr32 = nr / 32;
    for ( int idx=0; idx<nr32; idx++ )
	add( carr );

    return add( carr + 31 - (nr%32) + 1 );
}


BufferString& BufferString::addSpace( int nr )
{
    return addArr32Chars( spc32, nr );
}


BufferString& BufferString::addTab( int nr )
{
    return addArr32Chars( tab32, nr );
}


BufferString& BufferString::addNewLine( int nr )
{
    return addArr32Chars( nl32, nr );
}


bool BufferString::setBufSize( unsigned int newlen )
{
    if ( newlen < minlen_ )
	newlen = minlen_;
    else if ( newlen == len_ )
	return true;

    if ( minlen_ > 1 )
    {
	int nrminlens = newlen / minlen_;
	if ( newlen % minlen_ ) nrminlens++;
	newlen = nrminlens * minlen_;
    }
    if ( buf_ && newlen == len_ )
	return true;

    mDeclareAndTryAlloc(char*,newbuf,char [newlen] );
    if ( !newbuf )
	return false;
    else if ( !buf_ )
    {
	*newbuf = '\0';
    }
    else
    {
	int newsz = (buf_ ? strlen( buf_ ) : 0) + 1;
	if ( newsz > newlen )
	{
	    newsz = newlen;
	    buf_[newsz-1] = '\0';
	}
	OD::sysMemCopy( newbuf, buf_, newsz );
    }

    delete [] buf_;
    buf_ = newbuf;
    len_ = newlen;

    return true;
}


void BufferString::setMinBufSize( unsigned int newlen )
{
    const_cast<unsigned int&>(minlen_) = newlen;
    setBufSize( len_ );
}


BufferString& BufferString::replace( char from, char to )
{
    if ( isEmpty() || from == to )
	return *this;

    char* ptr = getCStr();
    while ( *ptr )
    {
	if ( *ptr == from )
	    *ptr = to;
	ptr++;
    }
    return *this;
}


BufferString& BufferString::replace( const char* from, const char* to )
{
    if ( isEmpty() || !from || !*from )
	return *this;

    const int fromlen = strlen( from );

    char* ptrfound = find( from );
    while ( ptrfound )
    {
	BufferString rest( ptrfound + fromlen );
	*ptrfound = '\0';
	add( to );
	const int curpos = size();
	add( rest );
	ptrfound = firstOcc( getCStr()+curpos, from );
    }
    return *this;
}


BufferString& BufferString::remove( char torem )
{
    if ( isEmpty() )
	return *this;

    char* writepos = getCStr();
    const char* chckpos = writepos;

    while ( *chckpos )
    {
	if ( *chckpos != torem )
	{
	    *writepos = *chckpos;
	    writepos++;
	}
	chckpos++;
    }
    *writepos = '\0';
    return *this;
}


BufferString& BufferString::trimBlanks()
{
    if ( isEmpty() )
	return *this;

    char* memstart = getCStr();
    char* ptr = memstart;
    mSkipBlanks( ptr );
    removeTrailingBlanks( ptr );

    if ( ptr == memstart )
	return *this;
    else if ( !*ptr )
	setEmpty();
    else
    {
	BufferString tmp( ptr );
	*this = tmp;
    }
    return *this;
}


BufferString& BufferString::insertAt( int atidx, const char* string )
{
    const int cursz = size();	// Had to do this to avoid weird compiler bug
    if ( atidx >= cursz )	// i.e. do not replace cursz with size() ...!
	{ replaceAt( atidx, string ); return *this; }
    if ( !string || !*string )
	return *this;

    if ( atidx < 0 )
    {
	const int lenstr = strlen( string );
	if ( atidx <= -lenstr )
	    return *this;
	string += -atidx;
	atidx = 0;
    }

    BufferString rest( buf_ + atidx );
    *(buf_ + atidx) = '\0';
    *this += string;
    *this += rest;
    return *this;
}


BufferString& BufferString::replaceAt( int atidx, const char* string, bool cut )
{
    const int strsz = string ? strlen(string) : 0;
    int cursz = size();
    const int nrtopad = atidx - cursz;
    if ( nrtopad > 0 )
    {
	const int newsz = cursz + nrtopad + strsz + 1;
	setBufSize( newsz );
	mDoArrayPtrOperation( char, buf_+cursz, = ' ', atidx, ++ );
	buf_[atidx] = '\0';
	cursz = newsz;
    }

    if ( strsz > 0 )
    {
	if ( atidx + strsz >= cursz )
	{
	    cursz = atidx + strsz + 1;
	    setBufSize( cursz );
	    buf_[cursz-1] = '\0';
	}

	for ( int idx=0; idx<strsz; idx++ )
	{
	    const int replidx = atidx + idx;
	    if ( replidx >= 0 )
		buf_[replidx] = *(string + idx);
	}
    }

    if ( cut )
    {
	setBufSize( atidx + strsz + 1 );
	buf_[atidx + strsz] = '\0';
    }

    return *this;
}


BufferString& BufferString::toLower()
{
    char* ptr = getCStr();
    while ( *ptr )
    {
	*ptr = tolower(*ptr);
        ptr++;
    }
    return *this;
}


BufferString& BufferString::toUpper()
{
    char* ptr = getCStr();
    while ( *ptr )
    {
	*ptr = toupper(*ptr);
        ptr++;
    }
    return *this;
}


BufferString& BufferString::embed( char s, char e )
{
    char sbuf[2]; sbuf[0] = s; sbuf[1] = '\0';
    char ebuf[2]; ebuf[0] = e; ebuf[1] = '\0';
    *this = BufferString( sbuf, str(), ebuf );
    return *this;
}


BufferString& BufferString::unEmbed( char s, char e )
{
    if ( isEmpty() )
	return *this;

    char* bufptr = getCStr();
    char* startptr = bufptr;
    if ( *startptr == s )
	startptr++;

    char* endptr = startptr;
    if ( !*endptr )
	setEmpty();
    else
    {
	while ( *endptr ) endptr++;
	endptr--;
	if ( *endptr == e )
	    *endptr = '\0';

	if ( startptr != bufptr )
	{
	    BufferString tmp( startptr );
	    *this = tmp;
	}
    }

    return *this;
}


BufferString& BufferString::clean( BufferString::CleanType ct )
{
    cleanupString( getCStr(), ct > NoSpaces,
			      ct == NoSpaces || ct == NoSpecialChars,
			      ct != OnlyAlphaNum );
    return *this;
}


static BufferString emptybufferstring( 1, true );

const BufferString& BufferString::empty()
{
    return emptybufferstring;
}


void BufferString::init()
{
    len_ = minlen_;
    if ( len_ < 1 )
	buf_ = nullptr;
    else
    {
	mTryAlloc( buf_, char[len_] );
	if ( buf_ )
	    *buf_ = '\0';
    }
}


typedef BufferStringSet::idx_type idx_type;


BufferStringSet::BufferStringSet( size_type nelem, const char* s )
{
    for ( idx_type idx=0; idx<nelem; idx++ )
	add( s );
}


BufferStringSet::BufferStringSet( const char* arr[], size_type len )
{
    add( arr, len );
}


BufferStringSet::BufferStringSet( const char* s )
{
    add( s );
}


BufferStringSet::BufferStringSet( const char* s1, const char* s2 )
{
    add( s1 ).add( s2 );
}


BufferStringSet::BufferStringSet( const char* s1, const char* s2,
				  const char* s3 )
{
    add( s1 ).add( s2 ).add( s3 );
}


bool BufferStringSet::operator ==( const BufferStringSet& oth ) const
{
    if ( &oth == this )
	return true;

    const size_type sz = size();
    if ( sz != oth.size() )
	return false;

    for ( idx_type idx=0; idx<sz; idx++ )
	if ( get(idx) != oth.get(idx) )
	    return false;

    return true;
}


bool BufferStringSet::operator !=( const BufferStringSet& oth ) const
{
    return !(*this == oth);
}


idx_type BufferStringSet::indexOf( const char* str, CaseSensitivity cs ) const
{
    if ( str )
    {
	const size_type sz = size();
	for ( idx_type idx=0; idx<sz; idx++ )
	{
	    const BufferString& bs = get( idx );
	    if ( bs.isEqual(str,cs) )
		return idx;
	}
    }
    return -1;
}


idx_type BufferStringSet::indexOf( const GlobExpr& ge ) const
{
    const size_type sz = size();
    for ( idx_type idx=0; idx<sz; idx++ )
    {
	if ( ge.matches( strs_[idx]->str() ) )
	    return idx;
    }
    return -1;
}


BufferString BufferStringSet::getDispString( size_type maxnritems,
					     bool quoted ) const
{
    const size_type sz = size();
    BufferString ret;
    if ( sz < 1 )
	{ ret.set( "-" ); return ret; }

    if ( maxnritems < 1 || maxnritems > sz )
	maxnritems = sz;

#   define mAddItm2Str(idx) \
    if ( quoted ) \
	ret.add( BufferString(get(idx)).quote() ); \
    else \
	ret.add( get(idx) )

    mAddItm2Str(0);
    for ( idx_type idx=1; idx<maxnritems; idx++ )
    {
	ret.add( idx == sz-1 ? " and " : ", " );
	mAddItm2Str( idx );
    }

    if ( sz > maxnritems )
	ret.add( ", ..." );

    return ret;
}


idx_type BufferStringSet::nearestMatch( const char* s, bool caseinsens ) const
{
    return nearestMatch( s, caseinsens ? CaseInsensitive : CaseSensitive );
}


idx_type BufferStringSet::nearestMatch( const char* s,
					CaseSensitivity cs ) const
{
    const size_type sz = size();
    if ( sz < 2 )
	return sz - 1;
    if ( !s )
	s = "";

    TypeSet<idx_type> candidates;
    if ( StringView(s).size() > 1 )
    {
	for ( idx_type idx=0; idx<sz; idx++ )
	    if ( get(idx).startsWith(s,cs) )
		candidates += idx;
	if ( candidates.isEmpty() )
	{
	    const BufferString matchstr( "*", s, "*" );
	    for ( idx_type idx=0; idx<sz; idx++ )
		if ( get(idx).matches(matchstr,cs) )
		    candidates += idx;
	}
    }
    if ( candidates.isEmpty() )
	for ( idx_type idx=0; idx<sz; idx++ )
	    candidates += idx;

    unsigned int mindist = mUdf(unsigned int); idx_type minidx = -1;
    for ( idx_type idx=0; idx<candidates.size(); idx++ )
    {
	const idx_type myidx = candidates[idx];
	const unsigned int curdist
		= get(myidx).getLevenshteinDist( s, cs );
	if ( idx == 0 || curdist < mindist  )
	    { mindist = curdist; minidx = myidx; }
    }
    return minidx;
}


TypeSet<idx_type> BufferStringSet::getMatches( const char* inpexpr,
					     CaseSensitivity cs ) const
{
    TypeSet<idx_type> ret;
    if ( inpexpr && *inpexpr )
    {
	const GlobExpr ge( inpexpr, cs );
	for ( idx_type idx=0; idx<size(); idx++ )
	    if ( ge.matches(get(idx).str()) )
		ret += idx;
    }
    return ret;
}


BufferString BufferStringSet::commonStart() const
{
    BufferString ret;
    const size_type sz = size();
    if ( sz < 1 )
	return ret;

    ret.set( get(0) );

    for ( idx_type idx=1; idx<sz; idx++ )
    {
	size_type retsz = ret.size();
	if ( retsz < 1 )
	    return ret;

	const BufferString& cur = get( idx );
	const size_type cursz = cur.size();
	if ( cursz < 1 )
	    { ret.setEmpty(); break; }
	if ( cursz < retsz )
	    { ret[cursz-1] = '\0'; retsz = cursz; }
	for ( idx_type ich=retsz-1; ich>-1; ich-- )
	{
	    if ( ret[ich] != cur[ich] )
		ret[ich] = '\0';
	}
    }

    return ret;
}


bool BufferStringSet::isSubsetOf( const BufferStringSet& bss ) const
{
    for ( idx_type idx=0; idx<size(); idx++ )
    {
	if ( !bss.isPresent(strs_[idx]->buf()) )
	    return false;
    }

    return true;
}


bool BufferStringSet::remove( const char* itm )
{
    const idx_type idx = indexOf( itm );
    if ( idx < 0 )
	return false;

    removeSingle( idx );
    return true;
}


BufferStringSet& BufferStringSet::add( const char* s )
{
    strs_.add( new BufferString(s) );
    return *this;
}


BufferStringSet& BufferStringSet::add( const OD::String& s )
{
    return add( s.str() );
}


BufferStringSet& BufferStringSet::add( const mQtclass(QString)& qstr )
{
    return add( BufferString(qstr) );
}


BufferStringSet& BufferStringSet::add( const BufferStringSet& bss,
				       bool allowdup )
{
    for ( idx_type idx=0; idx<bss.size(); idx++ )
    {
	const char* s = bss.get( idx );
	if ( allowdup || !isPresent(s) )
	    add( s );
    }
    return *this;
}


BufferStringSet& BufferStringSet::add( const char* arr[], size_type len )
{
    if ( len < 0 )
	for ( idx_type idx=0; arr[idx]; idx++ )
	    add( arr[idx] );
    else
	for ( idx_type idx=0; idx<len; idx++ )
	    add( arr[idx] );

    return *this;
}


BufferStringSet& BufferStringSet::addWordsFrom( const char* inp )
{
    if ( !inp || !*inp )
	return *this;
    mSkipBlanks( inp );
    if ( !*inp )
	return *this;

    const auto bufsz = StringView( inp ).size() + 1;
    char* buf = new char [bufsz];

    while ( true )
    {
	inp = getNextWordElem( inp, buf );
	if ( !inp )
	    break;
	add( buf );
    }

    delete [] buf;
    return *this;
}


bool BufferStringSet::addIfNew( const char* s )
{
    if ( !isPresent(s) )
	{ add( s ); return true; }
    return false;
}


bool BufferStringSet::addIfNew( const OD::String& s )
{
    return addIfNew( s.buf() );
}


BufferStringSet& BufferStringSet::addToAll( const char* str, bool infront )
{
    if ( !str || !*str )
	return *this;

    for ( idx_type idx=0; idx<size(); idx++ )
    {
	BufferString& itm = get( idx );
	if ( infront )
	    itm.insertAt( 0, str );
	else
	    itm.add( str );
    }

    return *this;
}


BufferStringSet::size_type BufferStringSet::maxLength() const
{
    size_type ret = 0;
    for ( size_type idx=0; idx<size(); idx++ )
    {
	const size_type len = get(idx).size();
	if ( len > ret )
	    ret = len;
    }
    return ret;
}


void BufferStringSet::sort( bool caseinsens, bool asc )
{
    idx_type* idxs = getSortIndexes( caseinsens, asc );
    useIndexes( idxs );
    delete [] idxs;
}


idx_type* BufferStringSet::getSortIndexes( bool caseinsens, bool asc ) const
{
    const size_type sz = size();
    if ( sz < 1 )
	return 0;

    mGetIdxArr( idx_type, idxs, sz );
    if ( !idxs || sz < 2 )
	{ if ( idxs ) *idxs = 0; return idxs; }

    BufferStringSet uppercaseset;
    const BufferStringSet* tosort = this;
    if ( caseinsens )
    {
	tosort = &uppercaseset;
	for ( idx_type idx=0; idx<sz; idx++ )
	{
	    BufferString newbs( get(idx) );
	    const idx_type len = newbs.size();
	    char* buf = newbs.getCStr();
	    for ( idx_type ich=0; ich<len; ich++ )
		buf[ich] = (char)toupper(buf[ich]);
	    uppercaseset.add( newbs );
	}
    }

    for ( idx_type d=sz/2; d>0; d=d/2 )
	for ( idx_type i=d; i<sz; i++ )
	    for ( idx_type j=i-d;
		  j>=0 && tosort->get(idxs[j]) > tosort->get(idxs[j+d]); j-=d )
		std::swap( idxs[j+d], idxs[j] );

    if ( !asc )
	std::reverse( idxs, idxs+sz );

    return idxs;
}


bool BufferStringSet::hasUniqueNames( CaseSensitivity sens ) const
{
    const idx_type lastidx = size() - 1;
    for ( idx_type idx=0; idx<lastidx; idx++ )
	if ( firstDuplicateOf(idx,sens,idx+1) >= 0 )
	    return false;
    return true;
}


idx_type BufferStringSet::firstDuplicateOf( idx_type idx2find,
			CaseSensitivity sens, idx_type startat ) const
{
    const size_type sz = size();
    if ( idx2find < 0 || idx2find >= sz )
	return -1;

    const BufferString& tofind = get( idx2find );
    for ( idx_type idx=startat; idx<sz; idx++ )
    {
	if ( idx == idx2find )
	    continue;

	if ( sens == CaseInsensitive )
	{
	    if ( caseInsensitiveEqual(tofind.str(),get(idx).str()) )
		return idx;
	}
	else if ( get(idx) == tofind )
	    return idx;

    }

    return -1;
}


void BufferStringSet::fillPar( IOPar& iopar ) const
{
    BufferString key;
    for ( idx_type idx=0; idx<size(); idx++ )
    {
	key.set( idx );
	iopar.set( key, get(idx) );
    }
}


void BufferStringSet::usePar( const IOPar& iopar )
{
    BufferString key;
    for ( idx_type idx=0; ; idx++ )
    {
	key.set( idx );
	if ( !iopar.isPresent(key) )
	    break;

	add( iopar.find(key) );
    }
}


void BufferStringSet::fill( uiStringSet& res ) const
{
    res.setEmpty();

    for ( idx_type idx=0; idx<size(); idx++ )
	res += toUiString( get(idx) );
}


void BufferStringSet::use( const uiStringSet& from )
{
    setEmpty();

    for ( idx_type idx=0; idx<from.size(); idx++ )
	add( from[idx].getFullString() );
}


void BufferStringSet::fill( QList<QString>& res ) const
{
#ifndef OD_NO_QT
    res.clear();

    for ( idx_type idx=0; idx<size(); idx++ )
	res.append( get(idx).str() );
#endif
}


void BufferStringSet::use( const QList<QString>& from )
{
#ifndef OD_NO_QT
    setEmpty();

    for ( QList<QString>::const_iterator iter = from.constBegin();
				      iter != from.constEnd(); ++iter )
	add( (*iter).toLocal8Bit().constData() );
#endif
}


BufferString BufferStringSet::cat( const char* sepstr ) const
{
    BufferString ret;
    for ( idx_type idx=0; idx<size(); idx++ )
    {
	if ( idx )
	    ret.add( sepstr );
	ret.add( get(idx) );
    }
    return ret;
}


void BufferStringSet::unCat( const char* inpstr, const char* sepstr )
{
    const size_type sepstrsz = StringView(sepstr).size();
    if ( sepstrsz < 1 )
	{ add( inpstr ); return; }

    BufferString str( inpstr );
    char* ptr = str.getCStr();

    while ( ptr && *ptr )
    {
	char* sepptr = ::firstOcc( ptr, sepstr );
	if ( sepptr )
	{
	    *sepptr = '\0';
	    sepptr += sepstrsz;
	}
	add( ptr );
	ptr = sepptr;
    }

    if ( ptr && *ptr )
	add( ptr );
}


uiStringSet BufferStringSet::getUiStringSet() const
{
    uiStringSet uistrset;
    for ( idx_type idx=0; idx<size(); idx++ )
        uistrset.add( toUiString(get(idx)) );
    return uistrset;
}


BufferStringSet& BufferStringSet::operator=( const char* arr[] )
{
    for ( idx_type idx=0; arr[idx]; idx++ )
	add( arr[idx] );
    return *this;
}


BufferStringSet& BufferStringSet::copy( const BufferStringSet& oth )
{
    *this = oth;
    return *this;
}


void deepErase( BufferStringSet& bss )
{
    bss.setEmpty();
}


void deepCopy( BufferStringSet& bss,const BufferStringSet& oth )
{
    bss = oth;
}


void sort( BufferStringSet& bss )
{
    bss.sort();
}


const BufferString* find( const BufferStringSet& bss, const char* nm )
{
    return find( bss.getStringSet(), nm );
}



// StringPair
StringPair::StringPair( const char* compstr )
{
    SeparString sepstr( compstr, separator() );
    first() = sepstr[0];
    second() = sepstr[1];
}


const OD::String& StringPair::getCompString() const
{
    return getCompString( false );
}


const OD::String& StringPair::getCompString( bool withwhitespace ) const
{
    mDeclStaticString( ret );
    ret = first();
    if ( !second().isEmpty() )
    {
	if ( withwhitespace )
	    ret.addSpace();
	ret.add( separator() );
	if ( withwhitespace )
	    ret.addSpace();
	ret.add( second() );
    }
    return ret;
}
