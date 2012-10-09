/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-12-1995
-*/

static const char* rcsID = "$Id$";

#include "iopar.h"
#include "multiid.h"
#include "keystrs.h"
#include "strmdata.h"
#include "strmprov.h"
#include "globexpr.h"
#include "position.h"
#include "separstr.h"
#include "ascstream.h"
#include "samplingdata.h"
#include "staticstring.h"
#include "bufstringset.h"
#include "color.h"
#include "convert.h"
#include "errh.h"

const int cMaxTypeSetItemsPerLine = 100;


IOPar::IOPar( const char* nm )
	: NamedObject(nm)
	, keys_(*new BufferStringSet)
	, vals_(*new BufferStringSet)
{
}


IOPar::IOPar( ascistream& astream )
	: NamedObject("")
	, keys_(*new BufferStringSet)
	, vals_(*new BufferStringSet)
{
    getFrom( astream );
}


IOPar::IOPar( const IOPar& iop )
	: NamedObject(iop.name())
	, keys_(*new BufferStringSet)
	, vals_(*new BufferStringSet)
{
    for ( int idx=0; idx<iop.size(); idx++ )
	add( iop.keys_.get(idx), iop.vals_.get(idx) );
}


IOPar& IOPar::operator =( const IOPar& iop )
{
    if ( this != &iop )
    {
	setEmpty();
	setName( iop.name() );
	for ( int idx=0; idx<iop.size(); idx++ )
	    add( iop.keys_.get(idx), iop.vals_.get(idx) );
    }
    return *this;
}


bool IOPar::isEqual( const IOPar& iop, bool worder ) const
{
    if ( &iop == this ) return true;
    const int sz = size();
    if ( iop.size() != sz ) return false;

    for ( int idx=0; idx<sz; idx++ )
    {
	if ( worder )
	{
	    if ( iop.keys_.get(idx) != keys_.get(idx)
	      || iop.vals_.get(idx) != vals_.get(idx) )
		return false;
	}
	else
	{
	    const char* res = iop.find( getKey(idx) );
	    if ( !res || strcmp(res,getValue(idx)) )
		return false;
	}
    }

    return true;
}


IOPar::~IOPar()
{
    setEmpty();
    delete &keys_;
    delete &vals_;
}


int IOPar::size() const
{
    return keys_.size();
}


int IOPar::indexOf( const char* key ) const
{
    return keys_.indexOf( key );
}


const char* IOPar::getKey( int nr ) const
{
    if ( nr >= size() ) return "";
    return keys_.get( nr ).buf();
}


const char* IOPar::getValue( int nr ) const
{
    if ( nr >= size() ) return "";
    return vals_.get( nr ).buf();
}


bool IOPar::setKey( int nr, const char* s )
{
    if ( nr >= size() || !s || !*s )
	return false;

    keys_.get(nr) = s;
    return true;
}


void IOPar::setValue( int nr, const char* s )
{
    if ( nr < size() )
	vals_.get(nr) = s;
}


void IOPar::setEmpty()
{
    deepErase( keys_ ); deepErase( vals_ );
}


void IOPar::remove( int idx )
{
    if ( idx >= size() ) return;
    keys_.remove( idx ); vals_.remove( idx );
}


void IOPar::remove( const char* key )
{
    const int idx = keys_.indexOf( key );
    if ( idx<0 )
	return;

    remove( idx );
}


void IOPar::merge( const IOPar& iopar )
{
    if ( &iopar == this ) return;

    for ( int idx=0; idx<iopar.size(); idx++ )
	set( iopar.keys_.get(idx), iopar.vals_.get(idx) );
}


const char* IOPar::compKey( const char* key1, int k2 )
{
    BufferString intstr = ""; intstr += k2;
    return compKey( key1, (const char*)intstr );
}


const char* IOPar::compKey( const char* key1, const char* key2 )
{
    static StaticStringManager stm;
    BufferString& ret = stm.getString();
    ret = key1;
    if ( key1 && key2 && *key1 && *key2 ) ret += ".";
    ret += key2;
    return ret;
}


IOPar* IOPar::subselect( int nr ) const
{
    BufferString s; s+= nr;
    return subselect( s.buf() );
}


IOPar* IOPar::subselect( const char* key ) const
{
    if ( !key ) return 0;

    IOPar* iopar = new IOPar( name() );
    for ( int idx=0; idx<keys_.size(); idx++ )
    {
	const char* nm = keys_.get(idx).buf();
	if ( !matchString(key,nm) ) continue;
	nm += strlen(key);
	if ( *nm == '.' && *(nm+1) )
	    iopar->add( nm+1, vals_.get(idx) );
    }

    if ( iopar->size() == 0 )
	{ delete iopar; iopar = 0; }
    return iopar;
}


void IOPar::removeSubSelection( int nr )
{
    BufferString s; s+= nr;
    return removeSubSelection( s.buf() );
}


void IOPar::removeSubSelection( const char* key )
{
    if ( !key ) return;

    for ( int idx=0; idx<keys_.size(); idx++ )
    {
	const char* nm = keys_.get(idx).buf();
	if ( !matchString(key,nm) ) continue;
	nm += strlen(key);
	if ( *nm == '.' && *(nm+1) )
	    { remove( idx ); idx--; }
    }
}


void IOPar::mergeComp( const IOPar& iopar, const char* ky )
{
    BufferString key( ky );
    char* ptr = key.buf() + key.size()-1;
    while ( ptr != key.buf() && *ptr == '.' )
	*ptr = '\0';

    const bool havekey = !key.isEmpty();
    if ( !havekey && &iopar == this ) return;

    BufferString buf;
    for ( int idx=0; idx<iopar.size(); idx++ )
    {
	buf = key;
	if ( havekey ) buf += ".";
	buf += iopar.keys_.get(idx);
	set( buf, iopar.vals_.get(idx) );
    }
}


const char* IOPar::findKeyFor( const char* s, int nr ) const
{
    if ( !s ) return 0;

    for ( int idx=0; idx<size(); idx++ )
    {
	if ( vals_.get(idx) == s )
	{
	    if ( nr )	nr--;
	    else	return keys_.get(idx).buf();
	}
    }

    return 0;
}


void IOPar::removeWithKey( const char* key )
{
    GlobExpr ge( key );
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( ge.matches( keys_.get(idx) ) )
	{
	    remove( idx );
	    idx--;
	}
    }
}


FixedString IOPar::operator[]( const char* keyw ) const
{
    FixedString res = find( keyw );
    if ( res )
	return res;

    return sKey::EmptyString;
}


FixedString IOPar::find( const char* keyw ) const
{
    const int idx = keys_.indexOf( keyw );
    return FixedString( keys_.validIdx(idx) ? vals_.get(idx).buf() : 0 );
}


void IOPar::add( const char* nm, const char* val )
{
    keys_.add( nm ); vals_.add( val );
}


#define mDefYNFns(fnnm) \
void IOPar::fnnm##YN( const char* keyw, bool yn ) \
{ \
    fnnm( keyw, getYesNoString(yn) ); \
} \
void IOPar::fnnm##YN( const char* keyw, bool yn1, bool yn2 ) \
{ \
    FileMultiString fms( getYesNoString(yn1) ); \
    fms.add( getYesNoString(yn2) ); \
    fnnm( keyw, fms ); \
} \
void IOPar::fnnm##YN( const char* keyw, bool yn1, bool yn2, bool yn3 ) \
{ \
    FileMultiString fms( getYesNoString(yn1) ); \
    fms.add( getYesNoString(yn2) ); \
    fms.add( getYesNoString(yn3) ); \
    fnnm( keyw, fms ); \
} \
void IOPar::fnnm##YN( const char* keyw, bool yn1, bool yn2, bool yn3, bool yn4 ) \
{ \
    FileMultiString fms( getYesNoString(yn1) ); \
    fms.add( getYesNoString(yn2) ); \
    fms.add( getYesNoString(yn3) ); \
    fms.add( getYesNoString(yn4) ); \
    fnnm( keyw, fms ); \
}

mDefYNFns(set)
mDefYNFns(add)


#define mDefSet1Val( type ) \
void IOPar::set( const char* keyw, type val ) \
{\
    set( keyw, toString( val ) );\
}
#define mDefSet2Val( type ) \
void IOPar::set( const char* s, type v1, type v2 ) \
{ \
    FileMultiString fms = toString(v1); \
    fms.add( toString(v2) ); \
    set( s, fms ); \
}
#define mDefSet3Val( type ) \
void IOPar::set( const char* s, type v1, type v2, type v3 ) \
{ \
    FileMultiString fms = toString(v1); \
    fms.add( toString(v2) ); \
    fms.add( toString(v3) ); \
    set( s, fms ); \
}
#define mDefSet4Val( type ) \
void IOPar::set( const char* s, type v1, type v2, type v3, type v4 ) \
{ \
    FileMultiString fms = toString(v1); \
    fms.add( toString(v2) ); \
    fms.add( toString(v3) ); \
    fms.add( toString(v4) ); \
    set( s, fms ); \
}

#define mDefAdd1Val(type) \
void IOPar::add( const char* keyw, type val ) \
{\
    add( keyw, toString( val ) ); \
}
#define mDefAdd2Val( type ) \
void IOPar::add( const char* s, type v1, type v2 ) \
{ \
    FileMultiString fms = toString(v1); \
    fms.add( toString(v2) ); \
    add( s, fms ); \
}
#define mDefAdd3Val( type ) \
void IOPar::add( const char* s, type v1, type v2, type v3 ) \
{ \
    FileMultiString fms = toString(v1); \
    fms.add( toString(v2) ); \
    fms.add( toString(v3) ); \
    add( s, fms ); \
}
#define mDefAdd4Val( type ) \
void IOPar::add( const char* s, type v1, type v2, type v3, type v4 ) \
{ \
    FileMultiString fms = toString(v1); \
    fms.add( toString(v2) ); \
    fms.add( toString(v3) ); \
    fms.add( toString(v4) ); \
    add( s, fms ); \
}

mDefSet1Val(int)	mDefSet2Val(int)
mDefSet3Val(int)	mDefSet4Val(int)
mDefSet1Val(od_uint32)	mDefSet2Val(od_uint32)
mDefSet3Val(od_uint32)	mDefSet4Val(od_uint32)
mDefSet1Val(od_int64)	mDefSet2Val(od_int64)
mDefSet3Val(od_int64)	mDefSet4Val(od_int64)
mDefSet1Val(od_uint64)	mDefSet2Val(od_uint64)
mDefSet3Val(od_uint64)	mDefSet4Val(od_uint64)
mDefSet1Val(float)	mDefSet2Val(float)
mDefSet3Val(float)	mDefSet4Val(float)
mDefSet1Val(double)	mDefSet2Val(double)
mDefSet3Val(double)	mDefSet4Val(double)

mDefAdd1Val(int)	mDefAdd2Val(int)
mDefAdd3Val(int)	mDefAdd4Val(int)
mDefAdd1Val(od_uint32)	mDefAdd2Val(od_uint32)
mDefAdd3Val(od_uint32)	mDefAdd4Val(od_uint32)
mDefAdd1Val(od_int64)	mDefAdd2Val(od_int64)
mDefAdd3Val(od_int64)	mDefAdd4Val(od_int64)
mDefAdd1Val(od_uint64)	mDefAdd2Val(od_uint64)
mDefAdd3Val(od_uint64)	mDefAdd4Val(od_uint64)
mDefAdd1Val(float)	mDefAdd2Val(float)
mDefAdd3Val(float)	mDefAdd4Val(float)
mDefAdd1Val(double)	mDefAdd2Val(double)
mDefAdd3Val(double)	mDefAdd4Val(double)


#define mDefGetI1Val( type, convfunc ) \
bool IOPar::get( const char* s, type& v1 ) const \
{ \
    const char* ptr = find(s); \
    if ( !ptr || !*ptr ) return false; \
\
    char* endptr; \
    type tmpval = convfunc; \
    if ( ptr==endptr ) return false; \
    v1 = tmpval; \
    return true; \
}

#define mDefGetI2Val( type, convfunc ) \
bool IOPar::get( const char* s, type& v1, type& v2 ) const \
{ \
    const char* ptr = find(s); \
    if ( !ptr || !*ptr ) return false; \
    FileMultiString fms( ptr ); \
    if ( fms.size() < 2 ) return false; \
    char* endptr; \
\
    ptr = fms[0]; \
    type tmpval = convfunc; \
    if ( ptr == endptr ) return false; \
    v1 = tmpval; \
\
    ptr = fms[1]; tmpval = convfunc; \
    if ( ptr != endptr ) v2 = tmpval; \
\
    return true; \
}

#define mDefGetI3Val( type, convfunc ) \
bool IOPar::get( const char* s, type& v1, type& v2, type& v3 ) const \
{ \
    const char* ptr = find(s); \
    if ( !ptr || !*ptr ) return false; \
    FileMultiString fms( ptr ); \
    if ( fms.size() < 3 ) return false; \
    char* endptr; \
\
    ptr = fms[0]; \
    type tmpval = convfunc; \
    if ( ptr == endptr ) return false; \
    v1 = tmpval; \
\
    ptr = fms[1]; tmpval = convfunc; \
    if ( ptr != endptr ) v2 = tmpval; \
\
    ptr = fms[2]; tmpval = convfunc; \
    if ( ptr != endptr ) v3 = tmpval; \
\
    return true; \
}

#define mDefGetI4Val( type, convfunc ) \
bool IOPar::get( const char* s, type& v1, type& v2, type& v3, type& v4 ) const \
{ \
    const char* ptr = find(s); \
    if ( !ptr || !*ptr ) return false; \
    FileMultiString fms( ptr ); \
    if ( fms.size() < 4 ) return false; \
    char* endptr; \
\
    ptr = fms[0]; \
    type tmpval = convfunc; \
    if ( ptr == endptr ) return false; \
    v1 = tmpval; \
\
    ptr = fms[1]; tmpval = convfunc; \
    if ( ptr != endptr ) v2 = tmpval; \
\
    ptr = fms[2]; tmpval = convfunc; \
    if ( ptr != endptr ) v3 = tmpval; \
\
    ptr = fms[3]; tmpval = convfunc; \
    if ( ptr != endptr ) v3 = tmpval; \
\
    return true; \
}

mDefGetI1Val(int,strtol(ptr, &endptr, 0));
mDefGetI2Val(int,strtol(ptr, &endptr, 0));
mDefGetI3Val(int,strtol(ptr, &endptr, 0));
mDefGetI4Val(int,strtol(ptr, &endptr, 0));
mDefGetI1Val(od_uint32,strtoul(ptr, &endptr, 0));
mDefGetI2Val(od_uint32,strtoul(ptr, &endptr, 0));
mDefGetI3Val(od_uint32,strtoul(ptr, &endptr, 0));
mDefGetI4Val(od_uint32,strtoul(ptr, &endptr, 0));
mDefGetI1Val(od_int64,strtoll(ptr, &endptr, 0));
mDefGetI2Val(od_int64,strtoll(ptr, &endptr, 0));
mDefGetI3Val(od_int64,strtoll(ptr, &endptr, 0));
mDefGetI4Val(od_int64,strtoll(ptr, &endptr, 0));
mDefGetI1Val(od_uint64,strtoull(ptr, &endptr, 0));
mDefGetI2Val(od_uint64,strtoull(ptr, &endptr, 0));
mDefGetI3Val(od_uint64,strtoull(ptr, &endptr, 0));
mDefGetI4Val(od_uint64,strtoull(ptr, &endptr, 0));


#define mDefGetFVals(typ) \
bool IOPar::get( const char* k, typ& v ) const \
{ return getScaled(k,v,1,false); } \
bool IOPar::get( const char* k, typ& v1, typ& v2 ) const \
{ return getScaled(k,v1,v2,1,false); } \
bool IOPar::get( const char* k, typ& v1, typ& v2, typ& v3 ) const \
{ return getScaled(k,v1,v2,v3,1,false); } \
bool IOPar::get( const char* k, typ& v1, typ& v2, typ& v3, typ& v4 ) const \
{ return getScaled(k,v1,v2,v3,v4,1,false); }

mDefGetFVals(float)
mDefGetFVals(double)


template <class T>
static bool iopget_typeset( const IOPar& iop, const char* s, TypeSet<T>& res )
{
    const char* ptr = iop.find(s); \
    if ( !ptr || !*ptr ) return false;

    res.erase();
    int keyidx = 0;
    while ( ptr && *ptr )
    {
	FileMultiString fms( ptr );
	const int len = fms.size();
	for ( int idx=0; idx<len; idx++ )
	{
	    const char* valstr = fms[idx];
	    const T newval = Conv::to<T>( valstr ); 
	    res += newval;
	}

	keyidx++;
	FixedString newkey = IOPar::compKey(s,keyidx);
	ptr = iop.find( newkey );
    }
    return true;
}


template <class T>
static void iopset_typeset( IOPar& iop, const char* keyw, 
			    const TypeSet<T>& vals )
{
    const int nrvals = vals.size();

    int validx = 0; 
    int keyidx = 0;

    while ( validx != nrvals )
    {
	T val = vals[ validx++ ];
	FileMultiString fms( toString(val) );

	for ( int cnt=1; cnt<cMaxTypeSetItemsPerLine; cnt++ )
	{
	    if ( validx == nrvals ) break;
	    
	    val = vals[ validx++ ];
	    fms += toString( val );
	}
	
	FixedString newkey = keyidx ? IOPar::compKey(keyw,keyidx) : keyw;
	iop.set( newkey, fms );
	keyidx++;
    }
}

#define mDefTSFns(typ) \
bool IOPar::get( const char* s, TypeSet<typ>& res ) const \
{ \
    return iopget_typeset( *this, s, res ); \
} \
\
void IOPar::set( const char* keyw, const TypeSet<typ>& vals ) \
{ \
    iopset_typeset( *this, keyw, vals ); \
}

mDefTSFns(int)
mDefTSFns(od_uint32)
mDefTSFns(od_int64)
mDefTSFns(od_uint64)
mDefTSFns(float)
mDefTSFns(double)


template <class T>
static bool iopget_scaled( const IOPar& iop, const char* s,
			   T** vptrs, int nrvals, T sc, bool setudf )
{
    const char* ptr = iop.find( s );
    bool havedata = false;
    if ( setudf || (ptr && *ptr) )
    {
	FileMultiString fms = ptr;
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    ptr = fms[idx];
	    T& f( *(vptrs[idx]) );
	    if ( *ptr )
	    {
		havedata = true;
		Conv::udfset( f, ptr );
		if ( !mIsUdf(f) ) f *= sc;
	    }
	    else if ( setudf )
		Values::setUdf(f);
	}
    }
    return havedata;
}


bool IOPar::getScaled( const char* s, float& f, float sc, bool udf ) const
{
    float* vptrs[1]; vptrs[0] = &f;
    return iopget_scaled( *this, s, vptrs, 1, sc, udf );
}
bool IOPar::getScaled( const char* s, double& f, double sc, bool udf ) const
{
    double* vptrs[1]; vptrs[0] = &f;
    return iopget_scaled( *this, s, vptrs, 1, sc, udf );
}
bool IOPar::getScaled( const char* s, float& f1, float& f2, float sc,
		       bool udf ) const
{
    float* vptrs[2]; vptrs[0] = &f1; vptrs[1] = &f2;
    return iopget_scaled( *this, s, vptrs, 2, sc, udf );
}
bool IOPar::getScaled( const char* s, double& f1, double& f2, double sc,
		       bool udf ) const
{
    double* vptrs[2]; vptrs[0] = &f1; vptrs[1] = &f2;
    return iopget_scaled( *this, s, vptrs, 2, sc, udf );
}
bool IOPar::getScaled( const char* s, float& f1, float& f2, float& f3, float sc,
		       bool udf ) const
{
    float* vptrs[3]; vptrs[0] = &f1; vptrs[1] = &f2; vptrs[2] = &f3;
    return iopget_scaled( *this, s, vptrs, 3, sc, udf );
}
bool IOPar::getScaled( const char* s, double& f1, double& f2, double& f3,
			double sc, bool udf ) const
{
    double* vptrs[3]; vptrs[0] = &f1; vptrs[1] = &f2; vptrs[2] = &f3;
    return iopget_scaled( *this, s, vptrs, 3, sc, udf );
}

bool IOPar::getScaled( const char* s, float& f1, float& f2, float& f3,
			float& f4, float sc, bool udf ) const
{
    float* vptrs[4];
    vptrs[0] = &f1; vptrs[1] = &f2; vptrs[2] = &f3; vptrs[3] = &f4;
    return iopget_scaled( *this, s, vptrs, 4, sc, udf );
}
bool IOPar::getScaled( const char* s, double& f1, double& f2, double& f3,
		       double& f4, double sc, bool udf ) const
{
    double* vptrs[4];
    vptrs[0] = &f1; vptrs[1] = &f2; vptrs[2] = &f3; vptrs[3] = &f4;
    return iopget_scaled( *this, s, vptrs, 4, sc, udf );
}


bool IOPar::get( const char* s, int& i1, int& i2, float& f ) const
{
    const char* ptr = find( s );
    bool havedata = false;
    if ( ptr && *ptr )
    {
	FileMultiString fms = ptr;
	ptr = fms[0];
	if ( *ptr ) { i1 = toInt( ptr ); havedata = true; }
	ptr = fms[1];
	if ( *ptr ) { i2 = toInt( ptr ); havedata = true; }
	ptr = fms[2];
	if ( *ptr ) { f = toFloat( ptr ); havedata = true; }
    }
    return havedata;
}


bool IOPar::getYN( const char* s, bool& i ) const
{
    const char* ptr = find( s );
    if ( !ptr ) return false;
    mSkipBlanks(ptr);
    if ( !*ptr ) return false;

    i = toBool(ptr,true);
    return true;
}


bool IOPar::getYN( const char* s, bool& i1, bool& i2 ) const
{
    const char* ptr = find( s );
    if ( !ptr || !*ptr ) return false;

    FileMultiString fms( ptr );
    i1 = toBool(fms[0],false);
    i2 = toBool(fms[1],false);
    return true;
}


bool IOPar::getYN( const char* s, bool& i1, bool& i2, bool& i3 ) const
{
    const char* ptr = find( s );
    if ( !ptr || !*ptr ) return false;

    FileMultiString fms( ptr );
    i1 = toBool(fms[0],false);
    i2 = toBool(fms[1],false);
    i3 = toBool(fms[2],false);
    return true;
}


bool IOPar::getYN( const char* s, bool& i1, bool& i2, bool& i3, bool& i4 ) const
{
    const char* ptr = find( s );
    if ( !ptr || !*ptr ) return false;

    FileMultiString fms( ptr );
    i1 = toBool(fms[0],false);
    i2 = toBool(fms[1],false);
    i3 = toBool(fms[2],false);
    i4 = toBool(fms[3],false);
    return true;
}


bool IOPar::getPtr( const char* s, void*& res ) const
{
    const char* ptr = find( s );
    if ( !ptr || !*ptr ) return false;

    return sscanf( ptr, "%p", &res ) > 0;
}


void IOPar::set( const char* keyw, const char* vals )
{
    int idx = keys_.indexOf( keyw );
    if ( idx < 0 )
	add( keyw, vals );
    else
	setValue( idx, vals );
}


void IOPar::set( const char* keyw, const char* vals1, const char* vals2 )
{
    FileMultiString fms( vals1 ); fms += vals2;
    int idx = keys_.indexOf( keyw );
    if ( idx < 0 )
	add( keyw, fms );
    else
	setValue( idx, fms );
}


void IOPar::set( const char* s, int i1, int i2, float f )
{
    FileMultiString fms = toString( i1 );
    fms.add( toString(i2) );
    fms.add( toString(f) );
    set( s, fms );
}


void IOPar::setPtr( const char* keyw, void* ptr )
{
    char buf[80]; sprintf( buf, "%p", ptr );
    set( keyw, buf );
}


bool IOPar::get( const char* s, Coord& crd ) const
{ return get( s, crd.x, crd.y ); }
void IOPar::set( const char* s, const Coord& crd )
{ set( s, crd.x, crd.y ); }

bool IOPar::get( const char* s, Coord3& crd ) const
{ return get( s, crd.x, crd.y, crd.z ); }
void IOPar::set( const char* s, const Coord3& crd )
{ set( s, crd.x, crd.y, crd.z ); }

bool IOPar::get( const char* s, BinID& binid ) const
{ return get( s, binid.inl, binid.crl ); }
void IOPar::set( const char* s, const BinID& binid )
{ set( s, binid.inl, binid.crl ); }


bool IOPar::get( const char* s, SeparString& ss ) const
{
    const char* res = find( s );
    if ( !res ) return false;
    ss = res;
    return true;
}


bool IOPar::get( const char* s, BufferString& bs ) const
{
    const char* res = find( s );
    if ( !res ) return false;
    bs = res;
    return true;
}


bool IOPar::get( const char* s, BufferString& bs1, BufferString& bs2 ) const
{
    const char* res = find( s );
    if ( !res ) return false;
    FileMultiString fms( res );
    bs1 = fms[0]; bs2 = fms[1];
    return true;
}


bool IOPar::get( const char* s, BufferStringSet& bss ) const
{
    const char* res = find( s );
    if ( !res ) return false;

    bss.erase();
    FileMultiString fms( res );
    const int sz = fms.size();
    for ( int idx=0; idx<sz; idx++ )
	bss.add( fms[idx] );
    return true;
}


void IOPar::set( const char* s, const SeparString& ss )
{
    set( s, ss.buf() );
}


void IOPar::set( const char* s, const BufferString& bs )
{
    set( s, bs.buf() );
}


void IOPar::set( const char* s, const FixedString& bs )
{
    set( s, bs.str() );
}


void IOPar::set( const char* s, const BufferString& bs1,
				const BufferString& bs2 )
{
    set( s, bs1.buf(), bs2.buf() );
}


void IOPar::set( const char* s, const BufferStringSet& bss )
{
    FileMultiString fms;
    for ( int idx=0; idx<bss.size(); idx++ )
	fms += bss.get( idx );
    set( s, fms.buf() );
}


bool IOPar::get( const char* s, MultiID& mid ) const
{
    const char* ptr = find( s );
    if ( !ptr || !*ptr ) return false;
    mid = ptr;
    return true;
}


void IOPar::set( const char* s, const MultiID& mid )
{
    set( s, mid.buf() );
}


bool IOPar::get( const char* s, Color& c ) const
{
    const char* ptr = find( s );
    if ( !ptr || !*ptr ) return false;

    return c.use( ptr );
}

void IOPar::set( const char* s, const Color& c )
{
    BufferString bs; c.fill( bs.buf() );
    set( s, bs );
}


void IOPar::getFrom( ascistream& strm )
{
    if ( atEndOfSection(strm) )
	strm.next();
    if ( strm.type() == ascistream::Keyword )
    {
	setName( strm.keyWord() );
	strm.next();
    }

    while ( !atEndOfSection(strm) )
    {
	add( strm.keyWord(), strm.value() );
	strm.next();
    }
}


void IOPar::putTo( ascostream& strm ) const
{
    if ( !name().isEmpty() )
	strm.put( name() );
    for ( int idx=0; idx<size(); idx++ )
	strm.put( keys_.get(idx), vals_.get(idx) );
    strm.newParagraph();
}


static const char* startsep	= "+## [";
static const char* midsep	= "] ## [";
static const char* endsep	= "] ##.";

void IOPar::putTo( BufferString& str ) const
{
    str = name();
    putParsTo( str );
}


void IOPar::putParsTo( BufferString& str ) const
{
    BufferString buf;
    for ( int idx=0; idx<size(); idx++ )
    {
	buf = startsep;
	buf += keys_.get(idx);
	buf += midsep;
	buf += vals_.get(idx);
	buf += endsep;
	str += buf;
    }
}

#define mAdvanceSep( ptr, sep ) \
    while ( *ptr && ( *ptr!=*sep || strncmp(ptr,sep,strlen(sep)) ) ) \
	{ ptr++; } \
\
    if( *ptr && !strncmp(ptr,sep,strlen(sep) ) ) \
    { \
	*ptr++ = '\0'; \
\
	for ( unsigned int idx=1; idx<strlen(sep); idx++ ) \
	    { if( *ptr ) ptr++; } \
    }

void IOPar::getFrom( const char* str )
{
    setEmpty();

    BufferString buf = str;
    char* ptrstart = buf.buf();
    char* ptr = ptrstart;

    mAdvanceSep( ptr, startsep )
    setName( ptrstart );

    while ( *ptr )
    {
	ptrstart = ptr; mAdvanceSep( ptr, midsep )

	keys_.add( ptrstart );

	ptrstart = ptr; mAdvanceSep( ptr, endsep )

	vals_.add( ptrstart );

	ptrstart = ptr; mAdvanceSep( ptr, startsep )
    }
}


void IOPar::getParsFrom( const char* str )
{
    setEmpty();

    BufferString buf = str;
    char* ptrstart = buf.buf();
    char* ptr = ptrstart;
    mAdvanceSep( ptr, startsep )

    while ( *ptr )
    {
	ptrstart = ptr;	mAdvanceSep( ptr, midsep )
	keys_.add( ptrstart );
	ptrstart = ptr;	mAdvanceSep( ptr, endsep )
	vals_.add( ptrstart );
	ptrstart = ptr; mAdvanceSep( ptr, startsep )
    }
}


bool IOPar::read( const char* fnm, const char* typ, bool chktyp )
{
    StreamData sd = StreamProvider(fnm).makeIStream();
    if ( !sd.usable() ) return false;
    const bool res = read( *sd.istrm, typ, chktyp );
    sd.close();
    return res;
}


bool IOPar::read( std::istream& strm, const char* typ, bool chktyp )
{
    const bool havetyp = typ && *typ;
    ascistream astream( strm, havetyp ? true : false );
    if ( havetyp && chktyp && !astream.isOfFileType(typ) )
    {
	BufferString msg( "File has wrong file type: '" );
	msg += astream.fileType();
	msg += "' should be: '"; msg += typ; msg += "'";
	ErrMsg( msg );
	return false;
    }
    else
	getFrom( astream );

    return true;
}


bool IOPar::write( const char* fnm, const char* typ ) const
{
    StreamData sd = StreamProvider(fnm).makeOStream();
    if ( !sd.usable() ) return false;
    bool ret = write( *sd.ostrm, typ );
    sd.close();
    return ret;
}


bool IOPar::write( std::ostream& strm, const char* typ ) const
{

    if ( typ && !strcmp(typ,sKeyDumpPretty()) )
	dumpPretty( strm );
    else
    {
	ascostream astream( strm );
	if ( typ && *typ )
	    astream.putHeader( typ );
	putTo( astream );
    }
    return true;
}


void IOPar::dumpPretty( std::ostream& strm ) const
{
    BufferString res;
    dumpPretty( res );
    strm << res.buf();
    strm.flush();
}


void IOPar::dumpPretty( BufferString& res ) const
{
    if ( !name().isEmpty() )
    {
	res += "> ";
	res += name();
	res += " <\n";
    }

    unsigned int maxkeylen = 0;
    bool haveval = false;
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( keys_[idx]->size() > maxkeylen )
	    maxkeylen = keys_[idx]->size();
	if ( !haveval && !vals_[idx]->isEmpty() )
	    haveval = true;
    }
    if ( maxkeylen == 0 ) return;

    const int valpos = haveval ? maxkeylen + 3 : 0;
    BufferString valposstr( valpos + 1, true );
    for ( int ispc=0; ispc<valpos; ispc++ )
	valposstr[ispc] = ' ';

    for ( int idx=0; idx<size(); idx++ )
    {
	const BufferString& ky = *keys_[idx];
	if ( ky == sKeyHdr() )
	{
	    res += "\n\n* ";
	    res += vals_.get(idx);
	    res += " *\n\n";
	    continue;
	}
	else if ( ky == sKeySubHdr() )
	{
	    res += "\n  - ";
	    res += vals_.get(idx);
	    res += "\n\n";
	    continue;
	}

	BufferString keyprint( maxkeylen + 1, true );
	const int extra = maxkeylen - ky.size();
	for ( int ispc=0; ispc<extra; ispc++ )
	    keyprint[ispc] = ' ';
	keyprint += ky;
	res += keyprint;
	res += (haveval ? " : " : "");

	BufferString valstr( vals_.get(idx) );
	char* startptr = valstr.buf();
	while ( startptr && *startptr )
	{
	    char* nlptr = strchr( startptr, '\n' );
	    if ( nlptr )
		*nlptr = '\0';
	    res += startptr;
	    if ( !nlptr ) break;

	    startptr = nlptr + 1;
	    if ( *startptr )
	    {
		res += "\n";
		res += valposstr;
	    }
	}
	res += "\n";
    }
}
