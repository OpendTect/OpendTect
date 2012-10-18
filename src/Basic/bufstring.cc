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
#include "globexpr.h"
#include "arrayndimpl.h"

#include <iostream>
#include <stdlib.h>
#include "string2.h"
#include <string.h>


BufferString::BufferString( const FixedString& s )
    : mBufferStringSimpConstrInitList
{ assignTo( s.str() ); }


BufferString::BufferString( int sz, bool mknull )
    : minlen_(sz)
    , len_(0)
    , buf_(0)
{
    if ( sz < 1 ) return;

    setBufSize( sz );
    if ( len_ > 0 )
	memset( buf_, 0, len_ );
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


BufferString::~BufferString()
{
    destroy();
}


bool BufferString::operator==( const char* s ) const
{
    if ( !s || !(*s) ) return isEmpty();
    if ( isEmpty() ) return false;

    const char* ptr = buf_;
    while ( *s && *ptr )
	if ( *ptr++ != *s++ ) return false;

    return *ptr == *s;
}


char* BufferString::buf()
{
    if ( !buf_ )
	init();

    return buf_;
}


bool BufferString::isEmpty() const
{ return !buf_ || !(*buf_); }


void BufferString::setEmpty()
{
    if ( len_ != minlen_ )
	{ destroy(); init(); }
    else
	buf_[0] = 0;
}


bool BufferString::isEqual( const char* s, bool caseinsens ) const
{
    return caseinsens ? caseInsensitiveEqual(buf(),s,0) : (*this == s);
}


bool BufferString::isStartOf( const char* s, bool caseinsens ) const
{
    return caseinsens ? matchStringCI(buf(),s) : matchString(buf(),s);
}


bool BufferString::matches( const char* s, bool caseinsens ) const
{
    return GlobExpr(s,!caseinsens).matches( buf() );
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


unsigned int BufferString::size() const	
{
    return buf_ ? strlen(buf_) : 0;
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
	memcpy( buf_, oldbuf, newsz );
    }

    delete [] oldbuf;
    len_ = newlen;
}


void BufferString::setMinBufSize( unsigned int newlen )
{
    const_cast<unsigned int&>(minlen_) = newlen;
    setBufSize( len_ );
}


void BufferString::insertAt( int atidx, const char* string )
{
    const int cursz = size();	// Had to do this to avoid weird compiler bug
    if ( atidx >= cursz )	// i.e. do not replace cursz with size() ...!
	{ replaceAt( atidx, string ); return; }
    if ( !string || !*string )
	return;

    if ( atidx < 0 )
    {
	const int lenstr = strlen( string );
	if ( atidx <= -lenstr ) return;
	string += -atidx;
	atidx = 0;
    }

    BufferString rest( buf_ + atidx );
    *(buf_ + atidx) = '\0';
    *this += string;
    *this += rest;
}


void BufferString::replaceAt( int atidx, const char* string, bool cut )
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
}


bool BufferString::operator >( const char* s ) const
{ return s && buf_ ? strcmp(buf_,s) > 0 : (bool) buf_; }


bool BufferString::operator <( const char* s ) const
{ return s && buf_ ? strcmp(buf_,s) < 0 : (bool) s; }


const BufferString& BufferString::empty()
{
    static BufferString* ret = 0;
    if ( !ret )
    {
	ret = new BufferString( "0" );
	*ret->buf_ = '\0';
    }
    return *ret;
}


void BufferString::fill( char* output ) const
{
    if ( !output )
	return;

    if ( !buf_ )
	*output = 0;
    else
	strcpy( output, buf_ );
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


std::ostream& operator <<( std::ostream& s, const BufferString& bs )
{
    s << bs.buf();
    return s;
}

std::istream& operator >>( std::istream& s, BufferString& bs )
{
    std::string stdstr; s >> stdstr;
    bs = stdstr.c_str();
    return s;
}


std::ostream& operator <<( std::ostream& s, const FixedString& fs )
{
    s << fs.str();
    return s;
}


BufferStringSet::BufferStringSet()
    : ManagedObjectSet<BufferString>(false)
{
}



BufferStringSet::BufferStringSet( const char* arr[], int len )
    : ManagedObjectSet<BufferString>(false)
{
    if ( len < 0 )
	for ( int idx=0; arr[idx]; idx++ )
	    add( arr[idx] );
    else
	for ( int idx=0; idx<len; idx++ )
	    add( arr[idx] );
}


BufferStringSet& BufferStringSet::operator =( const BufferStringSet& bss )
{
    ManagedObjectSet<BufferString>::operator =( bss );
    return *this;
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
	if ( ge.matches( (*this)[idx]->buf() ) )
	    return idx;
    }
    return -1;
}


// Levenshtein distance
static int getMatchDist( const BufferString& bs, const char* s, bool casesens )
{
    const char* s1 = bs.buf();
    const char* s2 = s;
    const int len1 = strlen( s1 );
    const int len2 = strlen( s2 );
    if ( len1 == 0 ) return len2;
    if ( len2 == 0 ) return len1;

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
	if ( allowdup || indexOf(s) < 0 )
	    add( s );
    }
    return *this;
}


bool BufferStringSet::addIfNew( const char* s )
{
    if ( indexOf(s) < 0 )
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
	    char* buf = newbs->buf();
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
	key = idx;
	iopar.set( key, *(*this)[idx] );
    }
}


void BufferStringSet::usePar( const IOPar& iopar )
{
    BufferString key;
    for ( int idx=0; ; idx++ )
    { 
	key = idx;
	if ( !iopar.find(key) ) return;
	add( iopar[key] );
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
    char* ptr = str.buf();

    while ( *ptr )
    {
	char* nlptr = strchr( ptr, sepchar );
	if ( nlptr )
	    *nlptr++ = '\0';
	add( ptr );
	ptr = nlptr;
    }

    if ( *ptr )
	add( ptr );
}


bool FixedString::operator==( const char* s ) const
{
    if ( ptr_==s )
	return true;

    if ( !ptr_ || !s )
	return false;

    return !strcmp(ptr_,s);
}


int FixedString::size() const
{
    if ( !ptr_ ) return 0;
    return strlen( ptr_ );
}
