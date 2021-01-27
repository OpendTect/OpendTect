/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 21-12-1995
-*/


#include "iopar.h"
#include "dbkey.h"
#include "keystrs.h"
#include "od_iostream.h"
#include "globexpr.h"
#include "geomid.h"
#include "position.h"
#include "odversion.h"
#include "separstr.h"
#include "ascstream.h"
#include "samplingdata.h"
#include "staticstring.h"
#include "bufstringset.h"
#include "color.h"
#include "convert.h"
#include "timefun.h"
#include "trckey.h"
#include "oddirs.h"
#include "odjson.h"
#include <stdio.h>
#include <string.h>


const int cMaxTypeSetItemsPerLine = 100;


IOPar::IOPar( const char* nm )
	: NamedObject(nm)
	, keys_(*new BufferStringSet)
	, vals_(*new BufferStringSet)
        , minorversion_( mODMinorVersion )
        , majorversion_( mODMajorVersion )
{
}


IOPar::IOPar( ascistream& astream )
	: NamedObject("")
	, keys_(*new BufferStringSet)
	, vals_(*new BufferStringSet)
        , minorversion_( mODMinorVersion )
        , majorversion_( mODMajorVersion )
{
    getFrom( astream );
}


IOPar::IOPar( const IOPar& oth )
	: NamedObject(oth.name())
	, keys_(*new BufferStringSet)
	, vals_(*new BufferStringSet)
        , minorversion_( oth.minorversion_ )
        , majorversion_( oth.majorversion_ )
{
    for ( int idx=0; idx<oth.size(); idx++ )
	add( oth.keys_.get(idx), oth.vals_.get(idx) );
}


IOPar::~IOPar()
{
    setEmpty();
    delete &keys_;
    delete &vals_;
}


IOPar& IOPar::operator =( const IOPar& oth )
{
    if ( this != &oth )
    {
	setEmpty();
	setName( oth.name() );
	for ( int idx=0; idx<oth.size(); idx++ )
	    add( oth.keys_.get(idx), oth.vals_.get(idx) );
    }

    minorversion_ = oth.minorversion_;
    majorversion_ = oth.majorversion_;
    return *this;
}


bool IOPar::isEqual( const IOPar& oth, bool worder ) const
{
    if ( &oth == this )
	return true;
    const int sz = size();
    if ( oth.size() != sz )
	return false;

    for ( int idx=0; idx<sz; idx++ )
    {
	if ( worder )
	{
	    if ( oth.keys_.get(idx) != keys_.get(idx)
	      || oth.vals_.get(idx) != vals_.get(idx) )
		return false;
	}
	else
	{
	    FixedString res = oth.find( getKey(idx) );
	    if ( res != getValue(idx) )
		return false;
	}
    }

    return true;
}


bool IOPar::includes( const IOPar& oth ) const
{
    const int othsz = oth.size();
    if ( &oth == this || othsz == 0 )
	return true;

    for ( int idx=0; idx<othsz; idx++ )
    {
	FixedString res = find( oth.getKey(idx) );
	if ( res != oth.getValue(idx) )
	    return false;
    }

    return true;
}


int IOPar::size() const
{
    return keys_.size();
}


int IOPar::indexOf( const char* key ) const
{
    if ( !key || !*key )
	{ pErrMsg("indexOf empty key requested"); return -1; }
    return keys_.indexOf( key );
}


FixedString IOPar::getKey( int nr ) const
{
    return FixedString( keys_.validIdx(nr) ? keys_.get(nr).buf() : 0 );
}


FixedString IOPar::getValue( int nr ) const
{
    return FixedString( vals_.validIdx(nr) ? vals_.get(nr).buf() : 0 );
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
    keys_.setEmpty();
    vals_.setEmpty();
}


void IOPar::remove( int idx )
{
    if ( keys_.validIdx(idx) )
	{ keys_.removeSingle( idx ); vals_.removeSingle( idx ); }
}


void IOPar::removeWithKey( const char* key )
{
    if ( !key || !*key ) return;

    for ( int idx=0; idx<size(); idx++ )
    {
	if ( keys_.get(idx) == key )
	{
	    remove( idx );
	    idx--;
	}
    }
}


void IOPar::removeWithKeyPattern( const char* pattern )
{
    const GlobExpr ge( pattern );
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( ge.matches( keys_.get(idx) ) )
	{
	    remove( idx );
	    idx--;
	}
    }
}


void IOPar::sortOnKeys()
{
    if ( keys_.size() < 2 )
	return;

    BufferStringSet::size_type* sortidxs = keys_.getSortIndexes();
    keys_.useIndexes( sortidxs );
    vals_.useIndexes( sortidxs );
    delete [] sortidxs;
}


int IOPar::maxContentSize( bool kys ) const
{
    return (kys ? keys_ : vals_).maxLength();
}


void IOPar::merge( const IOPar& oth )
{
    if ( &oth == this ) return;

    for ( int idx=0; idx<oth.size(); idx++ )
	set( oth.keys_.get(idx), oth.vals_.get(idx) );
}


void IOPar::addFrom( const IOPar& oth )
{
    if ( &oth == this ) return;

    for ( int idx=0; idx<oth.size(); idx++ )
    {
	const char* ky = oth.keys_.get( idx ).str();
	if ( !isPresent(ky) )
	    add( ky, oth.vals_.get(idx) );
    }
}


const char* IOPar::compKey( const char* key1, int k2 )
{
    BufferString intstr = ""; intstr += k2;
    return compKey( key1, (const char*)intstr );
}


const char* IOPar::compKey( const char* key1, const char* key2 )
{
    mDeclStaticString( ret );
    ret = key1;
    if ( key1 && key2 && *key1 && *key2 ) ret += ".";
    ret += key2;
    return ret;
}


#define mGetKeyPfx(ky) \
    BufferString pfx( ky ); \
    if ( !pfx.endsWith(".") ) \
	pfx.add( '.' ); \

bool IOPar::hasSubSelection( int nr ) const
{
    BufferString s; s+= nr;
    return hasSubSelection( s.str() );
}


bool IOPar::hasSubSelection( const char* kystr ) const
{
    mGetKeyPfx( kystr );
    for ( int idx=0; idx<keys_.size(); idx++ )
    {
	const BufferString& curky = keys_.get( idx );
	if ( curky.startsWith( pfx ) )
	    return true;
    }
    return false;
}


IOPar* IOPar::subselect( int nr ) const
{
    BufferString s; s+= nr;
    return subselect( s.str() );
}


IOPar* IOPar::subselect( const char* kystr ) const
{
    mGetKeyPfx( kystr );
    const int pfxlen = pfx.size();

    IOPar* iopar = new IOPar( name() );
    for ( int idx=0; idx<keys_.size(); idx++ )
    {
	const BufferString& curky = keys_.get( idx );
	if ( curky.startsWith( pfx ) )
	{
	    const char* ky = curky.str() + pfxlen;
	    iopar->add( ky, vals_.get(idx) );
	}
    }

    if ( iopar->isEmpty() )
	{ delete iopar; return 0; }

    iopar->majorversion_ = majorversion_;
    iopar->minorversion_ = minorversion_;
    return iopar;
}


void IOPar::removeSubSelection( int nr )
{
    BufferString s; s+= nr;
    return removeSubSelection( s.buf() );
}


void IOPar::removeSubSelection( const char* ky )
{
    mGetKeyPfx( ky );
    if ( pfx.isEmpty() )
	return;

    for ( int idx=0; idx<keys_.size(); idx++ )
    {
	if ( keys_.get(idx).startsWith(pfx) )
	    { remove( idx ); idx--; }
    }
}


void IOPar::updateComp( const IOPar& oth, const char* ky )
{
    if ( &oth != this )
    {
	removeSubSelection( ky );
	doMergeComp( oth, ky, true );
    }
}


void IOPar::mergeComp( const IOPar& oth, const char* ky )
{
    if ( &oth != this )
	doMergeComp( oth, ky, false );
}


void IOPar::doMergeComp( const IOPar& oth, const char* ky, bool doupd )
{
    mGetKeyPfx( ky );

    for ( int idx=0; idx<oth.size(); idx++ )
    {
	const BufferString entryky( pfx, oth.keys_.get(idx) );
	const char* valstr = oth.vals_.get( idx );
	if ( doupd )
	    update( entryky, valstr );
	else
	    set( entryky, valstr );
    }
}


const char* IOPar::findKeyFor( const char* val, int nr ) const
{
    if ( !val ) return 0;

    for ( int idx=0; idx<size(); idx++ )
    {
	if ( vals_.get(idx) == val )
	{
	    if ( nr )	nr--;
	    else	return keys_.get(idx).buf();
	}
    }

    return 0;
}


const char* IOPar::find( const char* keyw ) const
{
    const int idx = keys_.indexOf( keyw );
    return vals_.validIdx(idx) ? vals_.get(idx).buf() : 0;
}


void IOPar::set( const char* keyw, const char* val )
{
    if ( !keyw ) return;

    const int idxof = indexOf( keyw );
    if ( idxof < 0 )
	add( keyw, val );
    else
	vals_.get( idxof ) = val;
}


void IOPar::add( const char* keyw, const char* val )
{
    if ( !keyw ) return;

    keys_.add( keyw );
    vals_.add( val );
}


void IOPar::update( const char* keyw, const char* val )
{
    if ( !keyw ) return;

    const int idxof = indexOf( keyw );
    if ( idxof < 0 )
    {
	if ( val && *val )
	    add( keyw, val );
    }
    else if ( !val || !*val )
    {
	keys_.removeSingle( idxof );
	vals_.removeSingle( idxof );
    }
    else
	vals_.get( idxof ) = val;
}


#define mDefYNFns(fnnm) \
void IOPar::fnnm##YN( const char* kw, bool yn ) \
{ \
    const BufferString keyw( kw ); \
    fnnm( keyw, getYesNoString(yn) ); \
} \
void IOPar::fnnm##YN( const char* kw, bool yn1, bool yn2 ) \
{ \
    const BufferString keyw( kw ); \
    FileMultiString fms( getYesNoString(yn1) ); \
    fms.add( getYesNoString(yn2) ); \
    fnnm( keyw, fms ); \
} \
void IOPar::fnnm##YN( const char* kw, bool yn1, bool yn2, bool yn3 ) \
{ \
    const BufferString keyw( kw ); \
    FileMultiString fms( getYesNoString(yn1) ); \
    fms.add( getYesNoString(yn2) ); \
    fms.add( getYesNoString(yn3) ); \
    fnnm( keyw, fms ); \
} \
void IOPar::fnnm##YN( const char* kw, bool yn1, bool yn2, \
		      bool yn3, bool yn4 ) \
{ \
    const BufferString keyw( kw ); \
    FileMultiString fms( getYesNoString(yn1) ); \
    fms.add( getYesNoString(yn2) ); \
    fms.add( getYesNoString(yn3) ); \
    fms.add( getYesNoString(yn4) ); \
    fnnm( keyw, fms ); \
}

mDefYNFns(set)
mDefYNFns(add)


#define mDefSet1Val( type ) \
void IOPar::set( const char* kw, type val ) \
{\
    const BufferString keyw( kw ); \
    set( keyw, toString( val ) );\
}
#define mDefSet2Val( type ) \
void IOPar::set( const char* kw, type v1, type v2 ) \
{ \
    const BufferString keyw( kw ); \
    FileMultiString fms = toString(v1); \
    fms.add( toString(v2) ); \
    set( keyw, fms ); \
}
#define mDefSet3Val( type ) \
void IOPar::set( const char* kw, type v1, type v2, type v3 ) \
{ \
    const BufferString keyw( kw ); \
    FileMultiString fms = toString(v1); \
    fms.add( toString(v2) ); \
    fms.add( toString(v3) ); \
    set( keyw, fms ); \
}
#define mDefSet4Val( type ) \
void IOPar::set( const char* kw, type v1, type v2, type v3, type v4 ) \
{ \
    const BufferString keyw( kw ); \
    FileMultiString fms = toString(v1); \
    fms.add( toString(v2) ); \
    fms.add( toString(v3) ); \
    fms.add( toString(v4) ); \
    set( keyw, fms ); \
}

#define mDefAdd1Val(type) \
void IOPar::add( const char* keyw, type val ) \
{\
    add( keyw, toString( val ) ); \
}
#define mDefAdd2Val( type ) \
void IOPar::add( const char* kw, type v1, type v2 ) \
{ \
    const BufferString keyw( kw ); \
    FileMultiString fms = toString(v1); \
    fms.add( toString(v2) ); \
    add( keyw, fms ); \
}
#define mDefAdd3Val( type ) \
void IOPar::add( const char* kw, type v1, type v2, type v3 ) \
{ \
    const BufferString keyw( kw ); \
    FileMultiString fms = toString(v1); \
    fms.add( toString(v2) ); \
    fms.add( toString(v3) ); \
    add( keyw, fms ); \
}
#define mDefAdd4Val( type ) \
void IOPar::add( const char* kw, type v1, type v2, type v3, type v4 ) \
{ \
    const BufferString keyw( kw ); \
    FileMultiString fms = toString(v1); \
    fms.add( toString(v2) ); \
    fms.add( toString(v3) ); \
    fms.add( toString(v4) ); \
    add( keyw, fms ); \
}

#define mDefAddSetIUpto4Vals( typ ) \
    mDefSet1Val(typ) mDefAdd1Val(typ) \
    mDefSet2Val(typ) mDefAdd2Val(typ) \
    mDefSet3Val(typ) mDefAdd3Val(typ) \
    mDefSet4Val(typ) mDefAdd4Val(typ) \

mDefAddSetIUpto4Vals(od_int16)
mDefAddSetIUpto4Vals(od_uint16)
mDefAddSetIUpto4Vals(od_int32)
mDefAddSetIUpto4Vals(od_uint32)
mDefAddSetIUpto4Vals(od_int64)
mDefAddSetIUpto4Vals(od_uint64)
mDefAddSetIUpto4Vals(float)
mDefAddSetIUpto4Vals(double)


#define mGetStart(pval) \
    if ( !pval ) return false; \
    mSkipBlanks(pval)

#define mGetStartAllowEmptyOn(iop,pval) \
    const char* pval = iop.find( keyw ); \
    mGetStart(pval)

#define mGetStartAllowEmpty(pval) \
    const char* pval = find( keyw ); \
    mGetStart(pval)

#define mGetStartNotEmptyOn(iop,pval) \
    mGetStartAllowEmptyOn(iop,pval); \
    if ( !*pval ) return false

#define mGetStartNotEmpty(pval) \
    mGetStartAllowEmpty(pval); \
    if ( !*pval ) return false


#define mDefGetI1Val( type, fncall ) \
bool IOPar::get( const char* keyw, type& v1 ) const \
{ \
    mGetStartNotEmpty(pval); \
    char* endptr; \
    type valfound = fncall; \
    if ( pval==endptr ) return false; \
    v1 = valfound; \
    return true; \
}

#define mDefGetI2Val( type, fncall ) \
bool IOPar::get( const char* keyw, type& v1, type& v2 ) const \
{ \
    mGetStartNotEmpty(pval); \
    FileMultiString fms( pval ); \
    if ( fms.size() < 2 ) return false; \
    char* endptr; \
\
    pval = fms[0]; \
    type valfound = fncall; \
    if ( pval == endptr ) return false; \
    v1 = valfound; \
\
    pval = fms[1]; valfound = fncall; \
    if ( pval != endptr ) v2 = valfound; \
\
    return true; \
}

#define mDefGetI3Val( type, fncall ) \
bool IOPar::get( const char* keyw, type& v1, type& v2, type& v3 ) const \
{ \
    mGetStartNotEmpty(pval); \
    FileMultiString fms( pval ); \
    if ( fms.size() < 3 ) return false; \
    char* endptr; \
\
    pval = fms[0]; \
    type valfound = fncall; \
    if ( pval == endptr ) return false; \
    v1 = valfound; \
\
    pval = fms[1]; valfound = fncall; \
    if ( pval != endptr ) v2 = valfound; \
\
    pval = fms[2]; valfound = fncall; \
    if ( pval != endptr ) v3 = valfound; \
\
    return true; \
}

#define mDefGetI4Val( type, fncall ) \
bool IOPar::get( const char* keyw, type& v1,type& v2,type& v3,type& v4 ) const \
{ \
    mGetStartNotEmpty(pval); \
    FileMultiString fms( pval ); \
    if ( fms.size() < 4 ) return false; \
    char* endptr; \
\
    pval = fms[0]; \
    type valfound = fncall; \
    if ( pval == endptr ) return false; \
    v1 = valfound; \
\
    pval = fms[1]; valfound = fncall; \
    if ( pval != endptr ) v2 = valfound; \
\
    pval = fms[2]; valfound = fncall; \
    if ( pval != endptr ) v3 = valfound; \
\
    pval = fms[3]; valfound = fncall; \
    if ( pval != endptr ) v4 = valfound; \
\
    return true; \
}

#define mDefGetIUpto4Vals( typ, fncall ) \
mDefGetI1Val( typ, (typ)fncall ); \
mDefGetI2Val( typ, (typ)fncall ); \
mDefGetI3Val( typ, (typ)fncall ); \
mDefGetI4Val( typ, (typ)fncall )

mDefGetIUpto4Vals( od_int16, strtol( pval, &endptr, 0 ) )
mDefGetIUpto4Vals( od_uint16, strtoul( pval, &endptr, 0 ) )
mDefGetIUpto4Vals( od_int32, strtol( pval, &endptr, 0 ) )
mDefGetIUpto4Vals( od_uint32, strtoul( pval, &endptr, 0 ) )
mDefGetIUpto4Vals( od_int64, strtoll( pval, &endptr, 0 ) )
mDefGetIUpto4Vals( od_uint64, strtoull( pval, &endptr, 0 ) )


#define mDefGetFVals(typ) \
bool IOPar::get( const char* k, typ& v ) const \
{ return getScaled(k,v,(typ)1,false); } \
bool IOPar::get( const char* k, typ& v1, typ& v2 ) const \
{ return getScaled(k,v1,v2,(typ)1,false); } \
bool IOPar::get( const char* k, typ& v1, typ& v2, typ& v3 ) const \
{ return getScaled(k,v1,v2,v3,(typ)1,false); } \
bool IOPar::get( const char* k, typ& v1, typ& v2, typ& v3, typ& v4 ) const \
{ return getScaled(k,v1,v2,v3,v4,(typ)1,false); }

mDefGetFVals(float)
mDefGetFVals(double)


template <class T>
static bool iopget_typeset( const IOPar& iop, const char* keyw, TypeSet<T>& res)
{
    mGetStartNotEmptyOn(iop,pval);

    res.erase();
    int keyidx = 0;
    while ( pval && *pval )
    {
	FileMultiString fms( pval );
	const int len = fms.size();
	for ( int idx=0; idx<len; idx++ )
	{
	    const char* valstr = fms[idx];
	    const T newval = Conv::to<T>( valstr );
	    res += newval;
	}

	keyidx++;
	pval = iop.find( IOPar::compKey(keyw,keyidx) );
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
bool IOPar::get( const char* keyw, TypeSet<typ>& res ) const \
{ \
    return iopget_typeset( *this, keyw, res ); \
} \
\
void IOPar::set( const char* keyw, const TypeSet<typ>& vals ) \
{ \
    iopset_typeset( *this, keyw, vals ); \
}

mDefTSFns(od_int16)
mDefTSFns(od_uint16)
mDefTSFns(od_int32)
mDefTSFns(od_uint32)
mDefTSFns(od_int64)
mDefTSFns(od_uint64)
mDefTSFns(float)
mDefTSFns(double)


template <class T>
static bool iopget_scaled( const IOPar& iop, const char* keyw,
			   T** vptrs, int nrvals, T sc, bool setudf )
{
    FixedString fs = iop.find( keyw );
    bool havedata = false;
    if ( setudf || !fs.isEmpty() )
    {
	FileMultiString fms = fs;
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    fs = fms[idx];
	    T& f( *(vptrs[idx]) );
	    if ( !fs.isEmpty() )
	    {
		havedata = true;
		Conv::udfset( f, fs );
		if ( !mIsUdf(f) ) f *= sc;
	    }
	    else if ( setudf )
		Values::setUdf(f);
	}
    }
    return havedata;
}


bool IOPar::getScaled( const char* keyw, float& f, float sc, bool udf ) const
{
    float* vptrs[1]; vptrs[0] = &f;
    return iopget_scaled( *this, keyw, vptrs, 1, sc, udf );
}
bool IOPar::getScaled( const char* keyw, double& f, double sc, bool udf ) const
{
    double* vptrs[1]; vptrs[0] = &f;
    return iopget_scaled( *this, keyw, vptrs, 1, sc, udf );
}
bool IOPar::getScaled( const char* keyw, float& f1, float& f2, float sc,
		       bool udf ) const
{
    float* vptrs[2]; vptrs[0] = &f1; vptrs[1] = &f2;
    return iopget_scaled( *this, keyw, vptrs, 2, sc, udf );
}
bool IOPar::getScaled( const char* keyw, double& f1, double& f2, double sc,
		       bool udf ) const
{
    double* vptrs[2]; vptrs[0] = &f1; vptrs[1] = &f2;
    return iopget_scaled( *this, keyw, vptrs, 2, sc, udf );
}
bool IOPar::getScaled( const char* keyw, float& f1, float& f2, float& f3,
			float sc, bool udf ) const
{
    float* vptrs[3]; vptrs[0] = &f1; vptrs[1] = &f2; vptrs[2] = &f3;
    return iopget_scaled( *this, keyw, vptrs, 3, sc, udf );
}
bool IOPar::getScaled( const char* keyw, double& f1, double& f2, double& f3,
			double sc, bool udf ) const
{
    double* vptrs[3]; vptrs[0] = &f1; vptrs[1] = &f2; vptrs[2] = &f3;
    return iopget_scaled( *this, keyw, vptrs, 3, sc, udf );
}

bool IOPar::getScaled( const char* keyw, float& f1, float& f2, float& f3,
			float& f4, float sc, bool udf ) const
{
    float* vptrs[4];
    vptrs[0] = &f1; vptrs[1] = &f2; vptrs[2] = &f3; vptrs[3] = &f4;
    return iopget_scaled( *this, keyw, vptrs, 4, sc, udf );
}
bool IOPar::getScaled( const char* keyw, double& f1, double& f2, double& f3,
		       double& f4, double sc, bool udf ) const
{
    double* vptrs[4];
    vptrs[0] = &f1; vptrs[1] = &f2; vptrs[2] = &f3; vptrs[3] = &f4;
    return iopget_scaled( *this, keyw, vptrs, 4, sc, udf );
}


bool IOPar::get( const char* keyw, int& i1, int& i2, float& f ) const
{
    mGetStartNotEmpty(pval);
    bool havedata = false;
    if ( pval && *pval )
    {
	FileMultiString fms = pval;
	pval = fms[0];
	if ( *pval ) { i1 = toInt( pval ); havedata = true; }
	pval = fms[1];
	if ( *pval ) { i2 = toInt( pval ); havedata = true; }
	pval = fms[2];
	if ( *pval ) { f = toFloat( pval ); havedata = true; }
    }
    return havedata;
}


bool IOPar::getYN( const char* keyw, bool& i ) const
{
    mGetStartNotEmpty(pval);
    i = toBool(pval,true);
    return true;
}


bool IOPar::getYN( const char* keyw, bool& i1, bool& i2 ) const
{
    mGetStartNotEmpty(pval);
    FileMultiString fms( pval );
    i1 = toBool(fms[0],false);
    i2 = toBool(fms[1],false);
    return true;
}


bool IOPar::getYN( const char* keyw, bool& i1, bool& i2, bool& i3 ) const
{
    mGetStartNotEmpty(pval);
    FileMultiString fms( pval );
    i1 = toBool(fms[0],false);
    i2 = toBool(fms[1],false);
    i3 = toBool(fms[2],false);
    return true;
}


bool IOPar::getYN( const char* keyw, bool& i1, bool& i2, bool& i3,
		   bool& i4 ) const
{
    mGetStartNotEmpty(pval);
    FileMultiString fms( pval );
    i1 = toBool(fms[0],false);
    i2 = toBool(fms[1],false);
    i3 = toBool(fms[2],false);
    i4 = toBool(fms[3],false);
    return true;
}


bool IOPar::getPtr( const char* keyw, void*& res ) const
{
    mGetStartNotEmpty(pval);
#ifdef __win__
    return sscanf_s( pval, "%p", &res ) > 0;
#else
    return sscanf( pval, "%p", &res ) > 0;
#endif
}


void IOPar::set( const char* keyw, const char* v1, const char* v2 )
{
    FileMultiString fms( v1 ); fms += v2;
    set( keyw, fms.buf() );
}


void IOPar::set( const char* keyw, const char* v1, const char* v2,
		 const char* v3 )
{
    FileMultiString fms( v1 ); fms += v2; fms += v3;
    set( keyw, fms.buf() );
}


void IOPar::set( const char* keyw, int i1, int i2, float f )
{
    FileMultiString fms = toString( i1 );
    fms.add( toString(i2) );
    fms.add( toString(f) );
    set( keyw, fms.buf() );
}


void IOPar::setPtr( const char* keyw, void* ptr )
{
    char buf[80];
#ifdef __win__
    sprintf_s( buf, 80, "%p", ptr );
#else
    sprintf( buf, "%p", ptr );
#endif
    set( keyw, buf );
}


bool IOPar::get( const char* keyw, SeparString& ss ) const
{
    mGetStartAllowEmpty(pval);
    ss = pval;
    return true;
}


bool IOPar::getUiString( const char* keyw, uiString& uis ) const
{
    BufferString valstr;
    if ( !get( keyw, valstr ) )
	return false;

    if ( valstr.startsWith(mStoreduiStringPreamble) )
	return uis.useEncodedStorageString( valstr.buf() ) >= 0;

    uis = toUiString( valstr );
    return true;
}


bool IOPar::get( const char* keyw, BufferString& bs ) const
{
    mGetStartAllowEmpty(pval);
    bs = pval;
    return true;
}


bool IOPar::get( const char* keyw, BufferString& bs1, BufferString& bs2 ) const
{
    mGetStartAllowEmpty(pval);
    FileMultiString fms( pval );
    bs1 = fms[0]; bs2 = fms[1];
    return true;
}


bool IOPar::get( const char* keyw, BufferString& bs1, BufferString& bs2,
		 BufferString& bs3 ) const
{
    mGetStartAllowEmpty(pval);
    FileMultiString fms( pval );
    bs1 = fms[0]; bs2 = fms[1]; bs3 = fms[2];
    return true;
}


bool IOPar::get( const char* keyw, BufferStringSet& bss ) const
{
    mGetStartAllowEmpty(pval);
    bss.erase();
    FileMultiString fms( pval );
    const int sz = fms.size();
    for ( int idx=0; idx<sz; idx++ )
	bss.add( fms[idx] );
    return true;
}


bool IOPar::get( const char* keyw, BoolTypeSet& bools ) const
{
    mGetStartAllowEmpty(pval);
    bools.erase();
    FileMultiString fms( pval );
    const int sz = fms.size();
    for ( int idx=0; idx<sz; idx++ )
	bools.add( yesNoFromString(fms[idx]) );
    return true;
}


bool IOPar::get( const char* keyw, DBKeySet& dbkys ) const
{
    BufferStringSet strs;
    if ( !get(keyw,strs) )
	return false;

    for ( auto str : strs )
	dbkys.add( DBKey(str->str()) );
    return true;
}


void IOPar::set( const char* keyw, const SeparString& ss )
{
    set( keyw, ss.buf() );
}


void IOPar::setUiString( const char* keyw, const uiString& uis )
{
    if ( uis.isPlainAscii() )
	set( keyw, toString(uis) );
    else
    {
	BufferString buf;
	uis.encodeStorageString( buf );
	set( keyw, buf );
    }
}


void IOPar::set( const char* keyw, const OD::String& fs )
{
    set( keyw, fs.buf() );
}


void IOPar::set( const char* keyw, const OD::String& fs1,
				   const OD::String& fs2 )
{
    set( keyw, fs1.buf(), fs2.buf() );
}


void IOPar::set( const char* keyw, const OD::String& fs1,
				   const OD::String& fs2,
				   const OD::String& fs3 )
{
    set( keyw, fs1.buf(), fs2.buf(), fs3.buf() );
}


void IOPar::set( const char* keyw, const BoolTypeSet& bools )
{
    FileMultiString fms;
    for ( int idx=0; idx<bools.size(); idx++ )
	fms += bools.get( idx ) ? "Y" : "N";
    set( keyw, fms.buf() );
}


void IOPar::set( const char* keyw, const BufferStringSet& bss )
{
    FileMultiString fms;
    for ( int idx=0; idx<bss.size(); idx++ )
	fms += bss.get( idx );
    set( keyw, fms.buf() );
}


void IOPar::set( const char* keyw, const DBKeySet& dbkys )
{
    BufferStringSet strs;
    for ( auto dbky : dbkys )
	strs.add( dbky->toString() );
    set( keyw, strs );
}


bool IOPar::get( const char* keyw, TrcKey& tk ) const
{
    int sid, trcnr, linenr;
    if ( !get( keyw, sid, linenr, trcnr ) )
	return false;
    if ( sid > 0 )
	sid = 0;
    if ( sid < -2 )
	sid = -1;
    tk.setGeomSystem( (OD::GeomSystem)sid );
    tk.setLineNr( linenr ).setTrcNr( trcnr );
    return true;
}

void IOPar::set( const char* keyw, const TrcKey& tk )
{
    set( keyw, (int)tk.geomSystem(), tk.lineNr(), tk.trcNr() );
}


bool IOPar::get( const char* keyw, DBKey& dbky ) const
{
    mGetStartNotEmpty(pval);
    dbky.fromString( pval );
    return true;
}


void IOPar::set( const char* keyw, const DBKey& dbky )
{
    set( keyw, dbky.toString() );
}


void IOPar::update( const char* keyw, const DBKey& dbky )
{
    if ( dbky.isValid() )
	update( keyw, dbky.toString() );
    else
	update( keyw, 0 );
}


bool IOPar::get( const char* keyw, Pos::GeomID& gid ) const
{
    auto ival = gid.getI();
    bool rv = get( keyw, ival );
    if ( rv )
	gid.setI( ival );
    return rv;
}


void IOPar::update( const char* keyw, const Pos::GeomID& gid )
{
    if ( gid.isValid() )
	update( keyw, toString(gid) );
    else
	update( keyw, 0 );
}


void IOPar::set( const char* keyw, const GeomIDSet& geomids )
{
    GeomIDSet::IntSet ints;
    geomids.getIntSet( ints );
    set( keyw, ints );
}


bool IOPar::get( const char* keyw, GeomIDSet& geomids ) const
{
    GeomIDSet::IntSet ints;
    if ( !get(keyw,ints) )
	return false;

    geomids = GeomIDSet( ints );
    return true;
}


void IOPar::set( const char* keyw, const Pos::GeomID& gid )
{
    set( keyw, gid.getI() );
}


bool IOPar::get( const char* keyw, Color& c ) const
{
    mGetStartNotEmpty(pval);
    const FileMultiString fms( pval );
    const int sz = fms.size();
    if ( sz==1 && isNumberString(pval,true) )
    {
	c.setRgb( fms.getIValue(0) );
	return true;
    }

    return c.use( pval );
}

void IOPar::set( const char* keyw, const Color& c )
{
    BufferString bs; c.fill( bs );
    set( keyw, bs );
}


void IOPar::setToDateTime( const char* keyw )
{
    if ( !keyw || !*keyw )
	keyw = sKey::DateTime();
    set( keyw, Time::getISOUTCDateTimeString() );
}


void IOPar::setToUser( const char* keyw )
{
    if ( !keyw || !*keyw )
	keyw = sKey::User();

    FileMultiString fms; fms.add( GetUserNm() );
    const char* odusrnm = GetSoftwareUser();
    if ( odusrnm ) fms.add( odusrnm );

    set( keyw, fms.buf() );
}


void IOPar::setStdCreationEntries()
{
    setToDateTime( sKey::CrAt() );
    setToUser( sKey::CrBy() );
}


void IOPar::getFrom( ascistream& strm )
{
    if ( atEndOfSection(strm) )
	strm.next();
    if ( strm.type() == ascistream::Keyword )
    {
	BufferString nm( strm.keyWord() );
	if ( nm.lastChar() == ':' )
	    nm.last() = '\0';
	setName( strm.keyWord() );
	strm.next();
    }

    while ( !atEndOfSection(strm) )
    {
	add( strm.keyWord(), strm.value() );
	strm.next();
    }

    majorversion_ = strm.majorVersion();
    minorversion_ = strm.minorVersion();
}


void IOPar::putTo( ascostream& strm, bool endparagraph ) const
{
    if ( !name().isEmpty() )
	strm.stream() << name() << od_endl;
    for ( int idx=0; idx<size(); idx++ )
	strm.put( keys_.get(idx), vals_.get(idx) );
    if ( endparagraph )
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
    while ( *ptr && ( *ptr!=*sep || strncmp(ptr,sep,strLength(sep)) ) ) \
	{ ptr++; } \
\
    if( *ptr && !strncmp(ptr,sep,strLength(sep) ) ) \
    { \
	*ptr++ = '\0'; \
\
	for ( unsigned int idx=1; idx<strLength(sep); idx++ ) \
	    { if( *ptr ) ptr++; } \
    }

void IOPar::getFrom( const char* str )
{
    setEmpty();

    BufferString buf = str;
    char* ptrstart = buf.getCStr();
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
    char* ptrstart = buf.getCStr();
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
    od_istream strm( fnm );
    return read( strm, typ, chktyp );
}


bool IOPar::read( od_istream& strm, const char* typ, bool chktyp )
{
    if ( !strm.isOK() )
	return false;

    const bool havetyp = typ && *typ;
    ascistream astream( strm, havetyp );
    if ( havetyp && chktyp && !astream.isOfFileType(typ) )
    {
	BufferString msg( "File has wrong file type: '" );
	msg += astream.fileType();
	msg += "' should be: '"; msg += typ; msg += "'";
	ErrMsg( msg );
	return false;
    }
    else if ( havetyp && !astream.hasStandardHeader() )
	return false;
    else
	getFrom( astream );

    return !strm.isBad();
}


bool IOPar::write( const char* fnm, const char* typ ) const
{
    od_ostream strm( fnm );
    return write( strm, typ );
}


bool IOPar::write( od_ostream& strm, const char* typ ) const
{
    if ( !strm.isOK() )
	return false;

    if ( typ && FixedString(typ)==sKeyDumpPretty() )
	dumpPretty( strm );
    else
    {
	ascostream astream( strm );
	if ( typ && *typ )
	    astream.putHeader( typ );
	putTo( astream, true );
    }

    return strm.isOK();
}


void IOPar::dumpPretty( od_ostream& strm ) const
{
    BufferString res;
    dumpPretty( res );
    strm << res.buf() << od_endl;
}


void IOPar::dumpPretty( BufferString& res ) const
{
    StringBuilder sb;
    if ( !name().isEmpty() )
	sb.add( "> " ).add( name() ).add( " <\n" );

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
	const BufferString& ky = keys_.get( idx );
	if ( ky == sKeyHdr() )
	{
	    sb.add( "\n\n* " ).add( vals_.get(idx) ).add( " *\n\n" );
	    continue;
	}
	else if ( ky == sKeySubHdr() )
	{
	    sb.add( "\n  - " ).add( vals_.get(idx) ).add( "\n\n" );
	    continue;
	}

	BufferString keyprint( maxkeylen + 1, true );
	const int extra = maxkeylen - ky.size();
	for ( int ispc=0; ispc<extra; ispc++ )
	    keyprint[ispc] = ' ';
	keyprint += ky;
	sb.add( keyprint );
	sb.add( (haveval ? " : " : "") );

	BufferString valstr( vals_.get(idx) );
	char* startptr = valstr.getCStr();
	while ( startptr && *startptr )
	{
	    char* nlptr = firstOcc( startptr, '\n' );
	    if ( nlptr )
		*nlptr = '\0';
	    sb.add( startptr );
	    if ( !nlptr ) break;

	    startptr = nlptr + 1;
	    if ( *startptr )
	    {
		sb.add( "\n" );
		sb.add( valposstr );
	    }
	}
	sb.add( "\n" );
    }

    res = sb.result();
}


void IOPar::collectIDs( TypeSet<int>& ids ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	BufferString idstr( keys_.get(idx) );
	char* firstdotptr = idstr.find( '.' );
	if ( !firstdotptr )
	    continue;

	*firstdotptr = '\0';
	if ( idstr.isNumber(true) )
	{
	    int id = idstr.toInt();
	    if ( !ids.isPresent(id) )
		ids += id;
	}
    }
}


int IOPar::odVersion() const
{
    return 100*majorversion_ + 10*minorversion_;
}


void IOPar::fillJSON( OD::JSON::Object& obj )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	auto key = getKey( idx );
	auto val = getValue( idx );

	obj.set( key, val );
    }
}

