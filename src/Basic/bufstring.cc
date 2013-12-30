/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "bufstring.h"
#include "bufstringset.h"
#include "fixedstring.h"
#include "iopar.h"
#include "general.h"
#include "arrayndimpl.h"
#include "string2.h"
#include "globexpr.h"

#include <QString>



BufferString::BufferString( int sz, bool mknull )
    : minlen_(sz)
    , len_(0)
    , buf_(0)
{
    if ( sz < 1 ) return;

    setBufSize( sz );
    if ( len_ > 0 )
	OD::memZero( buf_, len_ );
}


BufferString::BufferString( const BufferString& bs )
    : minlen_(bs.minlen_)
    , len_(0)
    , buf_(0)
{
    if ( !bs.buf_ || !bs.len_ ) return;

    mTryAlloc( buf_, char [bs.len_] );
    if ( buf_ )
    {
	len_ = bs.len_;
	strcpy( buf_, bs.buf_ );
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
    else
	buf_[0] = 0;
    return *this;
}


BufferString& BufferString::assignTo( const char* s )
{
    if ( buf_ == s ) return *this;

    if ( !s ) s = "";
    setBufSize( (unsigned int)(strlen(s) + 1) );
    char* ptr = buf_;
    while ( *s ) *ptr++ = *s++;
    *ptr = '\0';
    return *this;
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
    const QByteArray qba = qstr.toUtf8();
    return add( qba.constData() );
}


BufferString& BufferString::add( float f, int maxnrchars )
{
    return add( toString( f, maxnrchars ) );
}


BufferString& BufferString::add( double d, int maxnrchars )
{
    return add( toString( d, maxnrchars ) );
}


BufferString& BufferString::addSpace()
{
    return add( " " );
}


BufferString& BufferString::addTab()
{
    return add( "\t" );
}


BufferString& BufferString::addNewLine()
{
    return add( "\n" );
}


void BufferString::setBufSize( unsigned int newlen )
{
    if ( newlen < minlen_ )
	newlen = minlen_;
    else if ( newlen == len_ )
	return;

    if ( minlen_ > 1 )
    {
	int nrminlens = newlen / minlen_;
	if ( newlen % minlen_ ) nrminlens++;
	newlen = nrminlens * minlen_;
    }
    if ( buf_ && newlen == len_ )
	return;

    char* oldbuf = buf_;
    mTryAlloc( buf_, char [newlen] );
    if ( !buf_ )
	{ buf_ = oldbuf; return; }
    else if ( !oldbuf )
	*buf_ = '\0';
    else
    {
	unsigned int newsz = (oldbuf ? strlen( oldbuf ) : 0) + 1;
	if ( newsz > newlen )
	{
	    newsz = newlen;
	    oldbuf[newsz-1] = '\0';
	}
	OD::memCopy( buf_, oldbuf, newsz );
    }

    delete [] oldbuf;
    len_ = newlen;
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
	mPointerOperation( char, buf_+cursz, = ' ', atidx, ++ );
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
	buf_ = 0;
    else
    {
	mTryAlloc( buf_, char[len_] );
	if ( buf_ )
	    *buf_ ='\0';
    }
}


void BufferString::fill( char* output, int maxnrchar ) const
{
    if ( !output || maxnrchar < 1 )
	return;

    if ( !buf_ || maxnrchar < 2 )
	*output = 0;
    else
	strncpy( output, buf_, maxnrchar );
}


BufferStringSet::BufferStringSet( int nelem, const char* s )
{
    for ( int idx=0; idx<nelem; idx++ )
	add( s );
}



BufferStringSet::BufferStringSet( const char* arr[], int len )
{
    if ( len < 0 )
	for ( int idx=0; arr[idx]; idx++ )
	    add( arr[idx] );
    else
	for ( int idx=0; idx<len; idx++ )
	    add( arr[idx] );
}


bool BufferStringSet::operator ==( const BufferStringSet& bss ) const
{
    if ( size() != bss.size() ) return false;

    for ( int idx=0; idx<size(); idx++ )
	if ( get(idx) != bss.get(idx) )
	    return false;

    return true;
}


int BufferStringSet::indexOf( const char* s ) const
{
    if ( !s ) s = "";
    return ::indexOf( *this, s );
}


int BufferStringSet::indexOf( const GlobExpr& ge ) const
{
    const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( ge.matches( (*this)[idx]->str() ) )
	    return idx;
    }
    return -1;
}


// Levenshtein distance
static int getMatchDist( const BufferString& bs, const char* s, bool casesens )
{
    const int len1 = bs.size();
    const int len2 = FixedString(s).size();
    if ( len1 == 0 ) return len2;
    if ( len2 == 0 ) return len1;
    const char* s1 = bs.str();
    const char* s2 = s;

    Array2DImpl<int> d( len1+1, len2+1 );

    for ( int idx1=0; idx1<=len1; idx1++ )
	d.set( idx1, 0, idx1 );
    for ( int idx2=0; idx2<=len2; idx2++ )
	d.set( 0, idx2, idx2 );

    for ( int idx2=1; idx2<=len2; idx2++ )
    {
	for ( int idx1=1; idx1<=len1; idx1++ )
	{
	    const bool iseq = casesens ? s1[idx1-1] == s2[idx2-1]
			: toupper(s1[idx1-1]) == toupper(s2[idx2-1]);
	    if ( iseq )
		d.set( idx1, idx2, d.get(idx1-1,idx2-1) );
	    else
	    {
		const int delval = d.get( idx1-1, idx2 );
		const int insval = d.get( idx1, idx2-1 );
		const int substval = d.get( idx1-1, idx2-1 );
		d.set( idx1, idx2, 1 + (delval < insval
			? (delval<substval ? delval : substval)
		        : (insval<substval ? insval : substval)) );
	    }
	}
    }

    return d.get( len1, len2 );
}


int BufferStringSet::nearestMatch( const char* s, bool caseinsens ) const
{
    if ( isEmpty() ) return -1;
    const int sz = size();
    if ( sz < 2 ) return 0;
    if ( !s ) s = "";

    int mindist = -1; int minidx = -1;
    for ( int idx=0; idx<sz; idx++ )
    {
	const int curdist = getMatchDist( get(idx), s, !caseinsens );
	if ( idx == 0 || curdist < mindist  )
	    { mindist = curdist; minidx = idx; }
    }
    return minidx;
}


bool BufferStringSet::isSubsetOf( const BufferStringSet& bss ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( !bss.isPresent((*this)[idx]->buf()) )
	    return false;
    }

    return true;
}



BufferStringSet& BufferStringSet::add( const char* s )
{
    *this += new BufferString(s);
    return *this;
}


BufferStringSet& BufferStringSet::add( const FixedString& s )
{
    return add( s.str() );
}


BufferStringSet& BufferStringSet::add( const BufferString& s )
{
    *this += new BufferString(s);
    return *this;
}


BufferStringSet& BufferStringSet::add( const BufferStringSet& bss,
				       bool allowdup )
{
    for ( int idx=0; idx<bss.size(); idx++ )
    {
	const char* s = bss.get( idx );
	if ( allowdup || !isPresent(s) )
	    add( s );
    }
    return *this;
}


bool BufferStringSet::addIfNew( const char* s )
{
    if ( !isPresent(s) )
	{ add( s ); return true; }
    return false;
}


bool BufferStringSet::addIfNew( const BufferString& s )
{
    return addIfNew( s.buf() );
}


int BufferStringSet::maxLength() const
{
    int ret = 0;
    for ( int idx=0; idx<size(); idx++ )
    {
	const int len = get(idx).size();
	if ( len > ret )
	    ret = len;
    }
    return ret;
}


void BufferStringSet::sort( bool caseinsens, bool asc )
{
    int* idxs = getSortIndexes( caseinsens, asc );
    useIndexes( idxs );
    delete [] idxs;
}


void BufferStringSet::useIndexes( const int* idxs )
{
    const int sz = size();
    if ( !idxs || sz < 2 ) return;

    ObjectSet<BufferString> tmp;
    for ( int idx=0; idx<sz; idx++ )
	tmp += (*this)[idx];
    ObjectSet<BufferString>::erase();

    for ( int idx=0; idx<sz; idx++ )
	*this += tmp[ idxs[idx] ];
}


int* BufferStringSet::getSortIndexes( bool caseinsens, bool asc ) const
{
    const int sz = size();
    if ( sz < 1 ) return 0;

    mGetIdxArr(int,idxs,sz)
    if ( !idxs || sz < 2 )
	return idxs;

    BufferStringSet uppcasebss;
    const BufferStringSet* bss = this;
    if ( caseinsens )
    {
	bss = &uppcasebss;
	for ( int idx=0; idx<sz; idx++ )
	{
	    BufferString* newbs = new BufferString( get(idx) );
	    const int len = newbs->size();
	    char* buf = newbs->getCStr();
	    for ( int ich=0; ich<len; ich++ )
		buf[ich] = (char) toupper(buf[ich]);
	    uppcasebss += newbs;
	}
    }

    int tmp;
    for ( int d=sz/2; d>0; d=d/2 )
	for ( int i=d; i<sz; i++ )
	    for ( int j=i-d;
		  j>=0 && bss->get(idxs[j]) > bss->get(idxs[j+d]); j-=d )
		mSWAP( idxs[j+d], idxs[j], tmp )

    if ( !asc )
    {
	const int hsz = sz/2;
	for ( int idx=0; idx<hsz; idx++ )
	    mSWAP( idxs[idx], idxs[sz-idx-1], tmp )
    }
    return idxs;
}


void BufferStringSet::fillPar( IOPar& iopar ) const
{
    BufferString key;
    for ( int idx=0; idx<size(); idx++ )
    {
	key.set( idx );
	iopar.set( key, get(idx) );
    }
}


void BufferStringSet::usePar( const IOPar& iopar )
{
    BufferString key;
    for ( int idx=0; ; idx++ )
    {
	key.set( idx );
	const int idxof = iopar.indexOf( key );
	if ( idxof < 0 )
	    break;

	add( iopar.getValue(idx) );
    }
}


BufferString BufferStringSet::cat( char sepchar ) const
{
    BufferString ret;
    char sepstr[2]; sepstr[0] = sepchar; sepstr[1] = '\0';
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( idx )
	    ret.add( sepstr );
	ret.add( get(idx) );
    }
    return ret;
}


void BufferStringSet::unCat( const char* inpstr, char sepchar )
{
    BufferString str( inpstr );
    char* ptr = str.getCStr();

    while ( *ptr )
    {
	char* nlptr = ::firstOcc( ptr, sepchar );
	if ( nlptr )
	    *nlptr++ = '\0';
	add( ptr );
	ptr = nlptr;
    }

    if ( *ptr )
	add( ptr );
}
