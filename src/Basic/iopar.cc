/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 21-12-1995
-*/


#include "iopar.h"
#include "multiid.h"
#include "keystrs.h"
#include "od_iostream.h"
#include "globexpr.h"
#include "position.h"
#include "odversion.h"
#include "separstr.h"
#include "ascstream.h"
#include "samplingdata.h"
#include "perthreadrepos.h"
#include "bufstringset.h"
#include "color.h"
#include "convert.h"
#include "timefun.h"
#include "oddirs.h"
#include "odjson.h"
#include "odver.h"
#include <stdio.h>
#include <string.h>
#include <QHash>
#include <QHashIterator>
#include <QRegularExpression>
#include <QString>


const int cMaxTypeSetItemsPerLine = 100;


class ODHashMapIterator : public QHashIterator<QString,QString>
{
public:
ODHashMapIterator(const ODHashMap& map);

};



class ODHashMap : public QHash<QString,QString>
{
public:
bool includes( const ODHashMap& oth )
{
    ODHashMapIterator iter( oth );
    while ( iter.hasNext() )
    {
	iter.next();
	if ( !contains(iter.key()) || iter.value()!=value(iter.key()) )
	   return false;
    }

    return true;
}

bool removeWithKeyPattern( const char* pattern )
{
    ODHashMapIterator iter( *this );
    QRegularExpression expr( pattern );
    bool ret = false;
    while( iter.hasNext() )
    {
	iter.next();
	if ( iter.key().contains(expr) )
	{
	    remove( iter.key() );
	    ret = true;
	}
    }

    return ret;
}

void addFrom( const ODHashMap& oth )
{
    ODHashMapIterator iter( oth );
    while( iter.hasNext() )
    {
	iter.next();
	if ( !contains(iter.key()) )
	    insert( iter.key(), iter.value() );
    }
}

bool subselect( const char* kystr, ODHashMap& subsel )
{
    BufferString subselkey( kystr );
    if ( !subselkey.last() != '.' )
	subselkey.add( "." );

    ODHashMapIterator iter( *this );
    while( iter.hasNext() )
    {
	iter.next();
	if ( !iter.key().startsWith(subselkey.buf()) )
	    continue;

	QString subkey = iter.key();
	subkey.remove( 0, subselkey.size() );
	subsel.insert( subkey, iter.value() );
    }

    return !subsel.isEmpty();
}

bool removeSubSelection( const char* kystr )
{
    BufferString subselkey( kystr );
    if ( !subselkey.last() != '.' )
	subselkey.add( "." );

    ODHashMapIterator iter( *this );
    bool ret = false;
    while( iter.hasNext() )
    {
	iter.next();
	if ( !iter.key().startsWith(subselkey.buf()) )
	    continue;

	ret = true;
	remove( iter.key() );
    }

    return ret;
}

bool mergeComp( const ODHashMap& oth, const char* key )
{
    if ( oth.isEmpty() )
	return false;

    ODHashMapIterator iter( oth );
    while( iter.hasNext() )
    {
	iter.next();
	QString newkey( key );
	newkey.append( "." ).append( iter.key() );
	insert( newkey, iter.value() );
    }

    return true;
}

const char* findKeyFor( const char* val )
{
    mDeclStaticString( ret );
    ret = key( val );
    return ret.str();
}

const char* getValue( const char* key )
{
    mDeclStaticString( ret );
    ret = value( key );
    return ret.str();
}

};


ODHashMapIterator::ODHashMapIterator( const ODHashMap& map )
    : QHashIterator<QString,QString>(map)
{}


IOPar::IOPar( const char* nm )
    : NamedObject(nm)
    , majorversion_(mODMajorVersion)
    , minorversion_(mODMinorVersion)
    , patchversion_(mODPatchVersion)
    , hashmap_(*new ODHashMap)
{
}


IOPar::IOPar( ascistream& astream )
    : IOPar("")
{
    getFrom( astream );
}


IOPar::IOPar( const IOPar& iop )
    : NamedObject(iop.name())
    , majorversion_(iop.majorversion_)
    , minorversion_(iop.minorversion_)
    , patchversion_(iop.patchversion_)
    , hashmap_(*new ODHashMap(iop.hashmap_))
{
}


IOPar::~IOPar()
{
    delete &hashmap_;
}


IOPar& IOPar::operator =( const IOPar& iop )
{
    if ( this == &iop )
	return *this;

    setEmpty();
    setName( iop.name() );
    hashmap_ = iop.hashmap_;

    majorversion_ = iop.majorversion_;
    minorversion_ = iop.minorversion_;
    patchversion_ = iop.patchversion_;
    return *this;
}


bool IOPar::isEqual( const IOPar& iop ) const
{
    if ( &iop == this )
	return true;

    return hashmap_ == iop.hashmap_;
}


bool IOPar::includes( const IOPar& oth ) const
{
    return hashmap_.includes( oth.hashmap_ );
}


bool IOPar::isPresent( const char* keyw ) const
{
    return hashmap_.contains( keyw );
}


int IOPar::size() const
{
    return hashmap_.size();
}


bool IOPar::isEmpty() const
{
    return hashmap_.isEmpty();
}


void IOPar::setEmpty()
{
    hashmap_.clear();
}


bool IOPar::remove( const char* key )
{
    return hashmap_.remove( key );
}


void IOPar::removeWithKey( const char* key )
{
    remove( key );
}


bool IOPar::removeWithKeyPattern( const char* pattern )
{
    return hashmap_.removeWithKeyPattern( pattern );
}


void IOPar::merge( const IOPar& iopar )
{
    if ( &iopar == this )
	return;

    hashmap_.insert( iopar.hashmap_ );
}


void IOPar::addFrom( const IOPar& iopar )
{
    if ( &iopar == this )
	return;

    hashmap_.addFrom( iopar.hashmap_ );
}


const char* IOPar::compKey( const char* key1, int k2 )
{
    BufferString intstr = ""; intstr += k2;
    return compKey( key1, intstr.buf() );
}


const char* IOPar::compKey( const char* key1, const char* key2 )
{
    mDeclStaticString( ret );
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


IOPar* IOPar::subselect( const char* kystr ) const
{
    if ( !kystr || !*kystr )
	return nullptr;

    IOPar* iopar = new IOPar( name() );
    hashmap_.subselect( kystr, iopar->hashmap_ );
    if ( iopar->isEmpty() )
    {
	delete iopar;
	return nullptr;
    }

    iopar->majorversion_ = majorversion_;
    iopar->minorversion_ = minorversion_;
    iopar->patchversion_ = patchversion_;
    return iopar;
}


bool IOPar::removeSubSelection( int nr )
{
    BufferString s; s+= nr;
    return removeSubSelection( s.buf() );
}


bool IOPar::removeSubSelection( const char* kystr )
{
    if ( !kystr || !*kystr )
	return false;

    return hashmap_.removeSubSelection( kystr );
}


void IOPar::mergeComp( const IOPar& iopar, const char* ky )
{
    BufferString key( ky );
    char* ptr = key.getCStr() + key.size()-1;
    while ( ptr != key.buf() && *ptr == '.' )
	*ptr = '\0';

    const bool havekey = !key.isEmpty();
    if ( !havekey )
    {
	if ( &iopar != this )
	    merge( iopar );

	return;
    }

    hashmap_.mergeComp( iopar.hashmap_, key.buf() );
}


const char* IOPar::findKeyFor( const char* val ) const
{
    if ( !val || !*val )
	return nullptr;

    return hashmap_.findKeyFor( val );
}


const char* IOPar::find( const char* keyw ) const
{
    return hashmap_.getValue( keyw );
}


void IOPar::set( const char* keyw, const char* val )
{
    if ( !keyw || !*keyw )
	return;

    hashmap_.insert( keyw, val );
}


void IOPar::add( const char* keyw, const char* val )
{
    set( keyw, val );
}


void IOPar::addVal( const char* keyw, const char* valtoadd )
{
    BufferStringSet vals;
    get( keyw, vals );
    vals.addIfNew( valtoadd );
    set( keyw, vals );
}


void IOPar::update( const char* keyw, const char* val )
{
    if ( !keyw || !*keyw )
	return;

    if ( !val || !*val )
    {
	if ( isPresent(keyw) )
	{
	    remove( keyw );
	    return;
	}
    }

    set( keyw, val );
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
void IOPar::fnnm##YN( const char* keyw, bool yn1, bool yn2, \
		      bool yn3, bool yn4 ) \
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
void IOPar::set( const char* keyw, type v1, type v2 ) \
{ \
    FileMultiString fms = toString(v1); \
    fms.add( toString(v2) ); \
    set( keyw, fms ); \
}
#define mDefSet3Val( type ) \
void IOPar::set( const char* keyw, type v1, type v2, type v3 ) \
{ \
    FileMultiString fms = toString(v1); \
    fms.add( toString(v2) ); \
    fms.add( toString(v3) ); \
    set( keyw, fms ); \
}
#define mDefSet4Val( type ) \
void IOPar::set( const char* keyw, type v1, type v2, type v3, type v4 ) \
{ \
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
void IOPar::add( const char* keyw, type v1, type v2 ) \
{ \
    FileMultiString fms = toString(v1); \
    fms.add( toString(v2) ); \
    add( keyw, fms ); \
}
#define mDefAdd3Val( type ) \
void IOPar::add( const char* keyw, type v1, type v2, type v3 ) \
{ \
    FileMultiString fms = toString(v1); \
    fms.add( toString(v2) ); \
    fms.add( toString(v3) ); \
    add( keyw, fms ); \
}
#define mDefAdd4Val( type ) \
void IOPar::add( const char* keyw, type v1, type v2, type v3, type v4 ) \
{ \
    FileMultiString fms = toString(v1); \
    fms.add( toString(v2) ); \
    fms.add( toString(v3) ); \
    fms.add( toString(v4) ); \
    add( keyw, fms ); \
}

mDefSet1Val(od_int16)	mDefSet2Val(od_int16)
mDefSet3Val(od_int16)	mDefSet4Val(od_int16)
mDefSet1Val(od_uint16)	mDefSet2Val(od_uint16)
mDefSet3Val(od_uint16)	mDefSet4Val(od_uint16)
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

mDefAdd1Val(od_int16)	mDefAdd2Val(od_int16)
mDefAdd3Val(od_int16)	mDefAdd4Val(od_int16)
mDefAdd1Val(od_uint16)	mDefAdd2Val(od_uint16)
mDefAdd3Val(od_uint16)	mDefAdd4Val(od_uint16)
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


#define mDefGetI1Val( type, convfunc ) \
bool IOPar::get( const char* keyw, type& v1 ) const \
{ \
    mGetStartNotEmpty(pval); \
    char* endptr; \
    type valfound = convfunc; \
    if ( pval==endptr ) return false; \
    v1 = valfound; \
    return true; \
}

#define mDefGetI2Val( type, convfunc ) \
bool IOPar::get( const char* keyw, type& v1, type& v2 ) const \
{ \
    mGetStartNotEmpty(pval); \
    FileMultiString fms( pval ); \
    if ( fms.size() < 2 ) return false; \
    char* endptr; \
\
    pval = fms[0]; \
    type valfound = convfunc; \
    if ( pval == endptr ) return false; \
    v1 = valfound; \
\
    pval = fms[1]; valfound = convfunc; \
    if ( pval != endptr ) v2 = valfound; \
\
    return true; \
}

#define mDefGetI3Val( type, convfunc ) \
bool IOPar::get( const char* keyw, type& v1, type& v2, type& v3 ) const \
{ \
    mGetStartNotEmpty(pval); \
    FileMultiString fms( pval ); \
    if ( fms.size() < 3 ) return false; \
    char* endptr; \
\
    pval = fms[0]; \
    type valfound = convfunc; \
    if ( pval == endptr ) return false; \
    v1 = valfound; \
\
    pval = fms[1]; valfound = convfunc; \
    if ( pval != endptr ) v2 = valfound; \
\
    pval = fms[2]; valfound = convfunc; \
    if ( pval != endptr ) v3 = valfound; \
\
    return true; \
}

#define mDefGetI4Val( type, convfunc ) \
bool IOPar::get( const char* keyw, type& v1,type& v2,type& v3,type& v4 ) const \
{ \
    mGetStartNotEmpty(pval); \
    FileMultiString fms( pval ); \
    if ( fms.size() < 4 ) return false; \
    char* endptr; \
\
    pval = fms[0]; \
    type valfound = convfunc; \
    if ( pval == endptr ) return false; \
    v1 = valfound; \
\
    pval = fms[1]; valfound = convfunc; \
    if ( pval != endptr ) v2 = valfound; \
\
    pval = fms[2]; valfound = convfunc; \
    if ( pval != endptr ) v3 = valfound; \
\
    pval = fms[3]; valfound = convfunc; \
    if ( pval != endptr ) v4 = valfound; \
\
    return true; \
}

#define mDefGetIVal( tp, fn ) \
mDefGetI1Val( tp, fn ); \
mDefGetI2Val( tp, fn ); \
mDefGetI3Val( tp, fn ); \
mDefGetI4Val( tp, fn );

mDefGetIVal(od_int16,strtol(pval, &endptr, 0));
mDefGetIVal(od_uint16,strtoul(pval, &endptr, 0));
mDefGetIVal(int,strtol(pval, &endptr, 0));
mDefGetIVal(od_uint32,strtoul(pval, &endptr, 0));
mDefGetIVal(od_int64,strtoll(pval, &endptr, 0));
mDefGetIVal(od_uint64,strtoull(pval, &endptr, 0));


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

	StringView newkey = keyidx ? IOPar::compKey(keyw,keyidx) : keyw;
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
mDefTSFns(int)
mDefTSFns(od_uint32)
mDefTSFns(od_int64)
mDefTSFns(od_uint64)
mDefTSFns(float)
mDefTSFns(double)


template <class T>
static bool iopget_scaled( const IOPar& iop, const char* keyw,
			   T** vptrs, int nrvals, T sc, bool setudf )
{
    StringView fs = iop.find( keyw );
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
    return od_sscanf( pval, "%p", &res ) > 0;
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
    od_sprintf( buf, 80, "%p", ptr );
    set( keyw, buf );
}


bool IOPar::get( const char* keyw, Coord& crd ) const
{ return get( keyw, crd.x, crd.y ); }
void IOPar::set( const char* keyw, const Coord& crd )
{ set( keyw, crd.x, crd.y ); }

bool IOPar::get( const char* keyw, Coord3& crd ) const
{ return get( keyw, crd.x, crd.y, crd.z ); }
void IOPar::set( const char* keyw, const Coord3& crd )
{ set( keyw, crd.x, crd.y, crd.z ); }

bool IOPar::get( const char* keyw, BinID& binid ) const
{ return get( keyw, binid.inl(), binid.crl() ); }
void IOPar::set( const char* keyw, const BinID& binid )
{ set( keyw, binid.inl(), binid.crl() ); }


bool IOPar::get( const char* keyw, OD::GeomSystem& survid ) const
{
    int sidnr;
    if ( !get(keyw,sidnr) )
	return false;

    survid = mIsUdf(sidnr) ?  OD::Geom3D
			   : (sidnr >= (int)OD::Geom2D
				? OD::Geom2D
				: (sidnr < int(OD::GeomSynth)
					? OD::Geom3D
					: OD::GeomSystem(sidnr) ));
    return true;
}


bool IOPar::get( const char* keyw, TrcKey& tk ) const
{
    OD::GeomSystem gs;
    int sidnr, linenr, trcnr;
    if ( !get(keyw,gs) || !get(keyw,sidnr,linenr,trcnr) )
	return false;

    tk.setGeomSystem( gs );
    if ( tk.is2D() )
	tk.setGeomID( Pos::GeomID(linenr) ).setTrcNr( trcnr );
    else if ( tk.is3D() )
	tk.setLineNr( linenr ).setTrcNr( trcnr );
    else
	tk.setTrcNr( trcnr );

    return true;
}


void IOPar::set( const char* keyw, const TrcKey& tk )
{
   set( keyw, int(tk.geomSystem()), tk.lineNr(), tk.trcNr() );
}


void IOPar::set( const char* keyw, const OD::GeomSystem& gs )
{
    set( keyw, int(gs) );
}


bool IOPar::get( const char* keyw, SeparString& ss ) const
{
    mGetStartAllowEmpty(pval);
    ss = pval;
    return true;
}


bool IOPar::get( const char* keyw, BufferString& bs ) const
{
    mGetStartAllowEmpty(pval);
    bs = pval;
    return true;
}


bool IOPar::get( const char* keyw, uiString& uis ) const
{
    BufferString hex;
    uiString res;
    if ( !get( keyw, hex ) || !res.setFromHexEncoded( hex.buf() ) )
	return false;

    if ( hex.size() && res.isEmpty() )
	return false;

    uis = res;
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


bool IOPar::get( const char* keyw, TypeSet<MultiID>& keys ) const
{
    BufferStringSet strs;
    if ( !get(keyw,strs) )
	return false;

    for ( auto str : strs )
	keys.add( MultiID(str->str()) );
    return true;
}


void IOPar::set( const char* keyw, const TypeSet<MultiID>& keys )
{
    BufferStringSet strs;
    for ( int idx=0; idx<keys.size(); idx++ )
	strs.add( keys[idx].toString() );
    set( keyw, strs );
}


bool IOPar::get( const char* keyw, DBKeySet& keys ) const
{
    BufferStringSet strs;
    if ( !get(keyw,strs) )
	return false;

    keys.erase();
    for ( auto str : strs )
	keys.add( DBKey(str->str()) );
    return true;
}


void IOPar::set( const char* keyw, const DBKeySet& keys )
{
    BufferStringSet strs;
    for ( int idx=0; idx<keys.size(); idx++ )
	strs.add( keys[idx].toString(false) );
    set( keyw, strs );
}


bool IOPar::get( const char* key, TypeSet<Pos::GeomID>& ids ) const
{
    TypeSet<int> intvals;
    if ( !get(key,intvals) )
	return false;

    for ( const auto& val : intvals )
	ids.add( Pos::GeomID(val) );

    return !ids.isEmpty();
}


void IOPar::set( const char* key, const TypeSet<Pos::GeomID>& ids )
{
    TypeSet<int> intvals;
    for ( const auto& id : ids )
	intvals += id.asInt();

    set( key, intvals );
}


void IOPar::set( const char* keyw, const SeparString& ss )
{
    set( keyw, ss.buf() );
}


void IOPar::set( const char* keyw, const uiString& uis )
{
    BufferString buf;
    uis.getHexEncoded( buf );
    set( keyw, buf );
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


bool IOPar::get( const char* keyw, MultiID& mid ) const
{
    mGetStartNotEmpty(pval);
    mid.fromString( pval );
    return true;
}


bool IOPar::get( const char* keyw, DBKey& key ) const
{
    mGetStartNotEmpty(pval);
    key.fromString( pval );
    return true;
}


void IOPar::set( const char* keyw, const MultiID& mid )
{
    set( keyw, mid.toString() );
}


void IOPar::set( const char* keyw, const DBKey& key )
{
    set( keyw, key.toString(true) );
}


bool IOPar::get( const char* keyw, OD::Color& c ) const
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

void IOPar::set( const char* keyw, const OD::Color& c )
{
    BufferString bs; c.fill( bs );
    set( keyw, bs );
}


void IOPar::setToDateTime( const char* keyw )
{
    if ( !keyw || !*keyw )
	keyw = sKey::DateTime();
    set( keyw, Time::getISODateTimeString() );
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
    set( sKey::Version(), GetFullODVersion() );
    setToDateTime( sKey::CrAt() );
    setToUser( sKey::CrBy() );
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

    majorversion_ = strm.majorVersion();
    minorversion_ = strm.minorVersion();
    patchversion_ = strm.patchVersion();
}


void IOPar::putTo( ascostream& strm ) const
{
    if ( !name().isEmpty() )
	strm.stream() << name() << od_endl;

    IOParIterator iter( *this );
    BufferString key, val;
    while ( iter.next(key,val) )
	strm.put( key, val );

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
    IOParIterator iter( *this );
    BufferString key, val, buf;
    while ( iter.next(key,val) )
    {
	buf = startsep;
	buf += key;
	buf += midsep;
	buf += val;
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
    char* ptrstart = buf.getCStr();
    char* ptr = ptrstart;

    mAdvanceSep( ptr, startsep )
    setName( ptrstart );

    BufferString key, val;
    while ( *ptr )
    {
	ptrstart = ptr; mAdvanceSep( ptr, midsep )
	key = ptrstart;

	ptrstart = ptr; mAdvanceSep( ptr, endsep )
	val = ptrstart;

	ptrstart = ptr; mAdvanceSep( ptr, startsep )
	add( key, val );
    }
}


void IOPar::getParsFrom( const char* str )
{
    setEmpty();

    BufferString buf = str;
    char* ptrstart = buf.getCStr();
    char* ptr = ptrstart;
    mAdvanceSep( ptr, startsep )

    BufferString key, val;
    while ( *ptr )
    {
	ptrstart = ptr; mAdvanceSep( ptr, midsep )
	key = ptrstart;
	ptrstart = ptr; mAdvanceSep( ptr, endsep )
	val = ptrstart;
	ptrstart = ptr; mAdvanceSep( ptr, startsep )
	add( key, val );
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

    if ( typ && StringView(typ)==sKeyDumpPretty() )
	dumpPretty( strm );
    else
    {
	ascostream astream( strm );
	if ( typ && *typ )
	    astream.putHeader( typ );
	putTo( astream );
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
    if ( !name().isEmpty() )
    {
	res += "> ";
	res += name();
	res += " <\n";
    }

    unsigned int maxkeylen = 0;
    bool haveval = false;
    IOParIterator iter( *this );
    BufferString key, val, buf;
    while ( iter.next(key,val) )
    {
	if ( key.size() > maxkeylen )
	    maxkeylen = key.size();
	if ( !haveval && !val.isEmpty() )
	    haveval = true;
    }

    if ( maxkeylen == 0 )
	return;

    const int valpos = haveval ? maxkeylen + 3 : 0;
    BufferString valposstr( valpos + 1, true );
    for ( int ispc=0; ispc<valpos; ispc++ )
	valposstr[ispc] = ' ';

    iter.reset();
    while ( iter.next(key,val) )
    {
	if ( key == sKeyHdr() )
	{
	    res += "\n\n* ";
	    res += val;
	    res += " *\n\n";
	    continue;
	}
	else if ( key == sKeySubHdr() )
	{
	    res += "\n	- ";
	    res += val;
	    res += "\n\n";
	    continue;
	}

	BufferString keyprint( maxkeylen + 1, true );
	const int extra = maxkeylen - key.size();
	for ( int ispc=0; ispc<extra; ispc++ )
	    keyprint[ispc] = ' ';
	keyprint += key;
	res += keyprint;
	res += (haveval ? " : " : "");

	char* startptr = val.getCStr();
	while ( startptr && *startptr )
	{
	    char* nlptr = firstOcc( startptr, '\n' );
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


int IOPar::odVersion() const
{
    return 100*majorversion_ + 10*minorversion_ + patchVersion();
}


bool IOPar::areSubParsIndexed() const
{
    int indexedparsize = 0;
    for ( int idx=0; idx<1024; idx++ )
    {
	PtrMan<IOPar> subpar = subselect( idx );
	if ( subpar )
	    indexedparsize += subpar->size();
	else
	    break;
    }

    return indexedparsize == size();
}


void IOPar::fillJSON( OD::JSON::Object& jsonobj, const BufferStringSet& keys,
		      const BufferStringSet& vals, const char* subkey,
		      int& startidx ) const
{
    for ( int idx=startidx; idx<keys.size(); idx++ )
    {
	BufferString key = keys.get( idx );
	char* keystr = key.getCStr();
	if ( subkey )
	{
	    if ( !key.startsWith(subkey) )
	    {
		startidx = idx - 1;
		break;
	    }

	    keystr += FixedString(subkey).size();
	    key = keystr;
	}

	char* dotptr = key.find( '.' );
	if ( !dotptr )
	{
	    const BufferString& val = vals.get( idx );
	    FileMultiString fms( val );
	    if ( fms.isEmpty() )
		continue;

	    bool isbool = true, isint = true, isnum = true;
	    for ( int idy=0; idy<fms.size(); idy++ )
	    {
		if ( !isBoolString(fms[idy]) )
		    isbool = false;

		if ( !isNumberString(fms[idy],false) )
		    isnum = isint = false;
		else if ( !isNumberString(fms[idy],true) )
		    isint = false;
	    }

	    if ( fms.size() == 1 )
	    {
		if ( isbool )
		    jsonobj.set( key, toBool(val) );
		else if ( isint )
		    jsonobj.set( key, toInt(val) );
		else if ( isnum )
		    jsonobj.set( key, toDouble(val) );
		else
		{
		    BufferString str = val;
		    str.replace( "\"", "\\\"" );
		    jsonobj.set( key, str );
		}

		continue;
	    }

	    // Make JSON Array
	    OD::JSON::DataType datatype = isbool ? OD::JSON::Boolean
			: (isnum ? OD::JSON::Number : OD::JSON::String);
	    OD::JSON::Array* arr = new OD::JSON::Array( datatype );
	    for ( int idy=0; idy<fms.size(); idy++ )
	    {
		if ( isbool )
		    arr->add( toBool(fms[idy]) );
		else if ( isint )
		    arr->add( toInt(fms[idy]) );
		else if ( isnum )
		    arr->add( toDouble(fms[idy]) );
		else
		    arr->add( fms[idy] );
	    }

	    jsonobj.set( key, arr );
	    continue;
	}

	*dotptr = '\0';
	bool aresubparsindexed = true;
	BufferString basekey = compKey( subkey, key );
	int index=0, count=0;
	int idy = idx;
	for ( ; idy<keys.size(); idy++ )
	{
	    const BufferString indexkey = compKey( basekey, index );
	    if ( keys.get(idy).startsWith(indexkey) )
	    {
		count++;
		continue;
	    }

	    if ( !keys.get(idy).startsWith(basekey) )
	    {
		if ( count == 0 )
		    index--;

		break;
	    }

	    if ( count == 0 )
	    {
		aresubparsindexed = false;
		break;
	    }

	    count = 0;
	    index++;
	}

	PtrMan<IOPar> subpar = subselect( key );
	if ( !subpar || !subpar->size() )
	    continue;

	const bool shouldmakearray = aresubparsindexed;
	if ( shouldmakearray )
	{
	    const int lastindex = index;
	    auto* subarray = new OD::JSON::Array( true );
	    for ( index=0; index<=lastindex; index++ )
	    {
		const BufferString indexkey = compKey( basekey, index );
		auto* indexsubobj = new OD::JSON::Object;
		fillJSON( *indexsubobj, keys, vals, indexkey, startidx );
		subarray->add( indexsubobj );
	    }

	    jsonobj.set( key, subarray );
	}
	else
	{
	    auto* subobj = new OD::JSON::Object;
	    basekey.add( "." );
	    fillJSON( *subobj, keys, vals, basekey, startidx );
	    jsonobj.set( key, subobj );
	}
    }
}


void IOPar::fillJSON( OD::JSON::Object& jsonobj, bool simple ) const
{
    IOParIterator iter( *this );
    BufferString key, val, buf;
    if ( simple )
    {
	while ( iter.next(key,val) )
	    jsonobj.set( key, val );

	return;
    }

    BufferStringSet keys, vals;
    while ( iter.next(key,val) )
    {
	keys.add( key );
	vals.add( val );
    }

    ArrPtrMan<int> sortedidxs = keys.getSortIndexes( false );
    if ( !sortedidxs )
	return;

    keys.useIndexes( sortedidxs );
    vals.useIndexes( sortedidxs );
    int startidx = 0;
    fillJSON( jsonobj, keys, vals, nullptr, startidx );
}


bool IOPar::useJSON( const char* key, const OD::JSON::Array& jsonarr )
{
    const OD::JSON::ValueSet::ValueType valtype = jsonarr.valType();
    if ( valtype == OD::JSON::ValueSet::Data )
    {
	FileMultiString fms;
	for ( int idx=0; idx<jsonarr.size(); idx++ )
	    fms.add( jsonarr.getStringValue(idx) );

	set( key, fms );
	return true;
    }

    IOPar subpar;
    for ( int idx=0; idx<jsonarr.size(); idx++ )
    {
	const BufferString idxkey = toString( idx );
	if ( valtype == OD::JSON::ValueSet::SubArray )
	{
	    subpar.useJSON( idxkey, jsonarr.array(idx) );
	    continue;
	}

	const OD::JSON::Object& subobj = jsonarr.object( idx );
	IOPar objpar;
	if ( objpar.useJSON(subobj) )
	    subpar.mergeComp( objpar, idxkey );
    }

    if ( subpar.size() )
	mergeComp( subpar, key );

    return !isEmpty();
}


bool IOPar::useJSON( const OD::JSON::Object& jsonobj )
{
    for ( int idx=0; idx<jsonobj.size(); idx++ )
    {
	const BufferString& key = jsonobj.key( idx );
	const OD::JSON::ValueSet::ValueType valtype = jsonobj.valueType( idx );
	if ( valtype == OD::JSON::ValueSet::Data )
	{
	    if ( !key.isEmpty() )
		set( key, jsonobj.getStringValue(idx) );

	    continue;
	}

	if ( valtype == OD::JSON::ValueSet::SubArray )
	{
	    useJSON( key, jsonobj.array(idx) );
	    continue;
	}

	const OD::JSON::Object& subobj = jsonobj.object( idx );
	IOPar subpar;
	if ( subpar.useJSON(subobj) )
	    mergeComp( subpar, key );
    }

    return !isEmpty();
}



IOParIterator::IOParIterator( const IOPar& iop )
    : iter_(*new ODHashMapIterator(iop.hashmap_))
{}


IOParIterator::~IOParIterator()
{
    delete &iter_;
}


bool IOParIterator::next( BufferString& key, BufferString& val )
{
    if ( !iter_.hasNext() )
	return false;

    iter_.next();
    key = iter_.key();
    val = iter_.value();
    return true;
}


void IOParIterator::reset()
{
    iter_.toFront();
}
