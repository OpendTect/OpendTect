/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-12-1995
-*/

static const char* rcsID = "$Id: iopar.cc,v 1.64 2007-11-16 21:23:41 cvskris Exp $";

#include "iopar.h"
#include "multiid.h"
#include "keystrs.h"
#include "strmdata.h"
#include "strmprov.h"
#include "globexpr.h"
#include "position.h"
#include "separstr.h"
#include "ascstream.h"
#include "bufstringset.h"
#include "color.h"
#include "convert.h"
#include "errh.h"


IOPar::IOPar( const char* nm )
	: NamedObject(nm)
	, keys_(*new BufferStringSet(true))
	, vals_(*new BufferStringSet(true))
{
}


IOPar::IOPar( ascistream& astream )
	: NamedObject("")
	, keys_(*new BufferStringSet(true))
	, vals_(*new BufferStringSet(true))
{
    getFrom( astream );
}


IOPar::IOPar( const IOPar& iop )
	: NamedObject(iop.name())
	, keys_(*new BufferStringSet(true))
	, vals_(*new BufferStringSet(true))
{
    for ( int idx=0; idx<iop.size(); idx++ )
	add( iop.keys_.get(idx), iop.vals_.get(idx) );
}


IOPar& IOPar::operator =( const IOPar& iop )
{
    if ( this != &iop )
    {
	clear();
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
    clear();
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
    if ( nr >= size() || !s || !*s || keys_.indexOf(s) >= 0 )
	return false;

    keys_.get(nr) = s;
    return true;
}


void IOPar::setValue( int nr, const char* s )
{
    if ( nr < size() )
	vals_.get(nr) = s;
}


void IOPar::clear()
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
    for ( int idx=0; idx<iopar.size(); idx++ )
	set( iopar.keys_.get(idx), iopar.vals_.get(idx) );
}


const char* IOPar::compKey( const char* key1, int k2 )
{
    static BufferString intstr;
    intstr = ""; intstr += k2;
    return compKey( key1, (const char*)intstr );
}


const char* IOPar::compKey( const char* key1, const char* key2 )
{
    static BufferString ret;
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


const char* IOPar::operator[]( const char* keyw ) const
{
    const char* res = find( keyw );
    return res ? res : "";
}


const char* IOPar::find( const char* keyw ) const
{
    int idx = keys_.indexOf( keyw );
    return idx < 0 ? 0 : vals_.get(idx).buf();
}


void IOPar::add( const char* nm, const char* val )
{
    keys_.add( nm ); vals_.add( val );
}


#define get1Val( type, convfunc ) \
bool IOPar::get( const char* s, type& res ) const \
{ \
    const char* ptr = find(s); \
    if ( !ptr || !*ptr ) return false; \
\
    char* endptr; \
    type tmpval = convfunc; \
    if ( ptr==endptr) return false; \
    res = tmpval; \
    return true; \
}

get1Val(int,strtol(ptr, &endptr, 0));
get1Val(od_uint32,strtoul(ptr, &endptr, 0));
get1Val(od_int64,strtoll(ptr, &endptr, 0));
get1Val(od_uint64,strtoull(ptr, &endptr, 0));


template <class T>
static bool iopget_typeset( const IOPar& iop, const char* s, TypeSet<T>& res )
{
    const char* ptr = iop.find(s); \
    if ( !ptr || !*ptr ) return false;

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
	BufferString newkey = IOPar::compKey(s,keyidx);
	ptr = iop.find( newkey );
    }
    return true;
}


bool IOPar::get( const char* s, TypeSet<int>& res ) const
{
    return iopget_typeset( *this, s, res );
}


bool IOPar::get( const char* s, TypeSet<od_uint32>& res ) const
{
    return iopget_typeset( *this, s, res );
}


bool IOPar::get( const char* s, TypeSet<od_int64>& res ) const
{
    return iopget_typeset( *this, s, res );
}


bool IOPar::get( const char* s, TypeSet<od_uint64>& res ) const
{
    return iopget_typeset( *this, s, res );
}


bool IOPar::get( const char* s, TypeSet<double>& res ) const
{
    return iopget_typeset( *this, s, res );
}


bool IOPar::get( const char* s, TypeSet<float>& res ) const
{
    return iopget_typeset( *this, s, res );
}


bool IOPar::get( const char* s, int& i1, int& i2 ) const
{
    const char* ptr = find( s );
    bool havedata = false;
    if ( ptr && *ptr )
    {
	FileMultiString fms = ptr;
	ptr = fms[0];
	if ( *ptr ) { i1 = atoi( ptr ); havedata = true; }
	ptr = fms[1];
	if ( *ptr ) { i2 = atoi( ptr ); havedata = true; }
    }
    return havedata;
}


bool IOPar::getSc( const char* s, float& f, float sc, bool udf ) const
{
    const char* ptr = find( s );
    if ( ptr && *ptr )
    {
	Conv::udfset( f, ptr );
	if ( !mIsUdf(f) ) f *= sc;
	return true;
    }
    else if ( udf )
	Values::setUdf(f);

    return false;
}


bool IOPar::getSc( const char* s, double& d, double sc, bool udf ) const
{
    const char* ptr = find( s );
    if ( ptr && *ptr )
    {
	d = atof( ptr );
	if ( !mIsUdf(d) ) d *= sc;
	return true;
    }
    else if ( udf )
	Values::setUdf(d);

    return false;
}


bool IOPar::getSc( const char* s, float& f1, float& f2, float sc,
		   bool udf ) const
{
    double d1, d2;
    if ( getSc( s, d1, d2, sc, udf ) )
	{ f1 = (float)d1; f2 = (float)d2; return true; }
    return false;
}


bool IOPar::getSc( const char* s, float& f1, float& f2, float& f3, float sc,
		   bool udf ) const
{
    double d1, d2, d3;
    if ( getSc( s, d1, d2, d3, sc, udf ) )
	{ f1 = (float)d1; f2 = (float)d2; f3 = (float)d3; return true; }
    return false;
}


bool IOPar::getSc( const char* s, float& f1, float& f2, float& f3, float& f4,
		   float sc, bool udf ) const
{
    double d1, d2, d3, d4;
    if ( getSc( s, d1, d2, d3, d4, sc, udf ) )
    {
	f1 = (float)d1; f2 = (float)d2; f3 = (float)d3; f4 = (float)d4;
	return true;
    }
    return false;

}


bool IOPar::getSc( const char* s, double& d1, double& d2, double sc,
		 bool udf ) const
{
    const char* ptr = find( s );
    bool havedata = false;
    if ( udf || (ptr && *ptr) )
    {
	FileMultiString fms = ptr;
	ptr = fms[0];
	if ( *ptr )
	{
	    havedata = true;
	    d1 = atof( ptr );
	    if ( !mIsUdf(d1) ) d1 *= sc;
	}
	else if ( udf )
	    Values::setUdf(d1);

	ptr = fms[1];
	if ( *ptr )
	{
	    havedata = true;
	    d2 = atof( ptr );
	    if ( !mIsUdf(d2) ) d2 *= sc;
	}
	else if ( udf )
	    Values::setUdf(d2);
    }
    return havedata;
}


bool IOPar::getSc( const char* s, double& d1, double& d2, double& d3, double sc,
		 bool udf ) const
{
    const char* ptr = find( s );
    bool havedata = false;
    if ( udf || (ptr && *ptr) )
    {
	FileMultiString fms = ptr;
	ptr = fms[0];
	if ( *ptr )
	{
	    d1 = atof( ptr );
	    if ( !mIsUdf(d1) ) d1 *= sc;
	    havedata = true;
	}
	else if ( udf )
	    Values::setUdf(d1);

	ptr = fms[1];
	if ( *ptr )
	{
	    d2 = atof( ptr );
	    if ( !mIsUdf(d2) ) d2 *= sc;
	    havedata = true;
	}
	else if ( udf )
	    Values::setUdf(d2);

	ptr = fms[2];
	if ( *ptr )
	{
	    d3 = atof( ptr );
	    if ( !mIsUdf(d3) ) d3 *= sc;
	    havedata = true;
	}
	else if ( udf )
	    Values::setUdf(d3);
    }
    return havedata;
}


bool IOPar::getSc( const char* s, double& d1, double& d2, double& d3,
		   double& d4, double sc, bool udf ) const
{
    const char* ptr = find( s );
    bool havedata = false;
    if ( udf || (ptr && *ptr) )
    {
	FileMultiString fms = ptr;
	ptr = fms[0];
	if ( *ptr )
	{
	    havedata = true;
	    d1 = atof( ptr );
	    if ( !mIsUdf(d1) ) d1 *= sc;
	}
	else if ( udf )
	    Values::setUdf(d1);

	ptr = fms[1];
	if ( *ptr )
	{
	    havedata = true;
	    d2 = atof( ptr );
	    if ( !mIsUdf(d2) ) d2 *= sc;
	}
	else if ( udf )
	    Values::setUdf(d2);

	ptr = fms[2];
	if ( *ptr )
	{
	    havedata = true;
	    d3 = atof( ptr );
	    if ( !mIsUdf(d3) ) d3 *= sc;
	}
	else if ( udf )
	    Values::setUdf(d3);

	ptr = fms[3];
	if ( *ptr )
	{
	    havedata = true;
	    d4 = atof( ptr );
	    if ( !mIsUdf(d4) ) d4 *= sc;
	}
	else if ( udf )
	    Values::setUdf(d4);
    }
    return havedata;
}


bool IOPar::get( const char* s, int& i1, int& i2, int& i3 ) const
{
    const char* ptr = find( s );
    bool havedata = false;
    if ( ptr && *ptr )
    {
	FileMultiString fms = ptr;
	ptr = fms[0];
	if ( *ptr ) { i1 = atoi( ptr ); havedata = true; }
	ptr = fms[1];
	if ( *ptr ) { i2 = atoi( ptr ); havedata = true; }
	ptr = fms[2];
	if ( *ptr ) { i3 = atoi( ptr ); havedata = true; }
    }
    return havedata;
}


bool IOPar::get( const char* s, int& i1, int& i2, float& f ) const
{
    const char* ptr = find( s );
    bool havedata = false;
    if ( ptr && *ptr )
    {
	FileMultiString fms = ptr;
	ptr = fms[0];
	if ( *ptr ) { i1 = atoi( ptr ); havedata = true; }
	ptr = fms[1];
	if ( *ptr ) { i2 = atoi( ptr ); havedata = true; }
	ptr = fms[2];
	if ( *ptr ) { f = atof( ptr ); havedata = true; }
    }
    return havedata;
}


bool IOPar::getYN( const char* s, bool& i ) const
{
    const char* ptr = find( s );
    if ( !ptr || !*ptr ) return false;

    i = yesNoFromString(ptr);
    return true;
}


bool IOPar::getYN( const char* s, bool& i1, bool& i2 ) const
{
    const char* ptr = find( s );
    if ( !ptr || !*ptr ) return false;

    FileMultiString fms( ptr );
    i1 = yesNoFromString(fms[0]);
    i2 = yesNoFromString(fms[1]);
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


const int mMaxFMSLineSize = 100;

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
	FileMultiString fms( Conv::to<const char*>(val) );

	for ( int cnt=1; cnt<mMaxFMSLineSize; cnt++ )
	{
	    if ( validx == nrvals ) break;
	    
	    val = vals[ validx++ ];
	    fms += Conv::to<const char*>( val );
	}
	
	BufferString newkey = keyidx ? IOPar::compKey(keyw,keyidx) : keyw;
	iop.set( newkey, fms );
	keyidx++;
    }
}


void IOPar::set( const char* keyw, const TypeSet<int>& vals ) 
{ return iopset_typeset( *this, keyw, vals ); }


void IOPar::set( const char* keyw, const TypeSet<od_uint32>& vals ) 
{ return iopset_typeset( *this, keyw, vals ); }


void IOPar::set( const char* keyw, const TypeSet<od_int64>& vals )
{ return iopset_typeset( *this, keyw, vals ); }


void IOPar::set( const char* keyw, const TypeSet<od_uint64>& vals )
{ return iopset_typeset( *this, keyw, vals ); }


void IOPar::set( const char* keyw, const TypeSet<float>& vals )
{ return iopset_typeset( *this, keyw, vals ); }


void IOPar::set( const char* keyw, const TypeSet<double>& vals )
{ return iopset_typeset( *this, keyw, vals ); }


#define mSet1Val( type ) \
void IOPar::set( const char* keyw, type val ) \
{\
    set( keyw, Conv::to<const char*>(val) );\
}

mSet1Val( int );
mSet1Val( od_uint32 );
mSet1Val( od_int64 );
mSet1Val( od_uint64 );
mSet1Val( float );
mSet1Val( double );

#define mSet2Val( type ) \
void IOPar::set( const char* s, type v1, type v2 ) \
{ \
    FileMultiString fms = Conv::to<const char*>(v1); \
    fms.add( Conv::to<const char*>(v2) ); \
    set( s, fms ); \
}

mSet2Val( int );
mSet2Val( od_uint32 );
mSet2Val( od_int64 );
mSet2Val( od_uint64 );
mSet2Val( float );
mSet2Val( double );

#define mSet3Val( type ) \
void IOPar::set( const char* s, type v1, type v2, type v3 ) \
{ \
    FileMultiString fms = Conv::to<const char*>(v1); \
    fms.add( Conv::to<const char*>(v2) ); \
    fms.add( Conv::to<const char*>(v3) ); \
    set( s, fms ); \
}

mSet3Val( int );
mSet3Val( od_uint32 );
mSet3Val( od_int64 );
mSet3Val( od_uint64 );
mSet3Val( float );
mSet3Val( double );

#define mSet4Val( type ) \
void IOPar::set( const char* s, type v1, type v2, type v3, type v4 ) \
{ \
    FileMultiString fms = Conv::to<const char*>(v1); \
    fms.add( Conv::to<const char*>(v2) ); \
    fms.add( Conv::to<const char*>(v3) ); \
    fms.add( Conv::to<const char*>(v4) ); \
    set( s, fms ); \
}

mSet4Val( int );
mSet4Val( od_uint32 );
mSet4Val( od_int64 );
mSet4Val( od_uint64 );
mSet4Val( float );
mSet4Val( double );


void IOPar::set( const char* s, int i1, int i2, float f )
{
    FileMultiString fms = Conv::to<const char*>( i1 );
    fms.add( Conv::to<const char*>(i2) );
    fms.add( Conv::to<const char*>(f) );
    set( s, fms );
}


void IOPar::setYN( const char* keyw, bool i )
{
    set( keyw, getYesNoString(i) );
}


void IOPar::setYN( const char* keyw, bool i1, bool i2 )
{
    FileMultiString fms( getYesNoString(i1) );
    fms.add( getYesNoString(i2) );
    set( keyw, fms );
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
    if ( sz )
	bss.setIsOwner( true );
    for ( int idx=0; idx<sz; idx++ )
	bss.add( fms[idx] );
    return true;
}


void IOPar::set( const char* s, const BufferString& bs )
{
    set( s, (const char*)bs );
}


void IOPar::set( const char* s, const BufferString& bs1,
				const BufferString& bs2 )
{
    set( s, (const char*)bs1, (const char*)bs2 );
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
    set( s, (const char*)mid );
}

bool IOPar::get( const char* s, Interval<float>& rg ) const
{
    return get( s, rg.start, rg.stop );
}

void IOPar::set( const char* s, const Interval<float>& rg )
{
    set( s, rg.start, rg.stop );
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
	set( strm.keyWord(), strm.value() );
	strm.next();
    }
}


void IOPar::putTo( ascostream& strm ) const
{
    strm.tabsOff();
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
	for ( int idx=1; idx<strlen(sep); idx++ ) \
	    { if( *ptr ) ptr++; } \
    }

void IOPar::getFrom( const char* str )
{
    clear();

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
    clear();

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
    read( *sd.istrm, typ, chktyp );
    sd.close();
    return true;
}


void IOPar::read( std::istream& strm, const char* typ, bool chktyp )
{
    const bool havetyp = typ && *typ;
    ascistream astream( strm, havetyp ? YES : NO );
    if ( havetyp && chktyp && !astream.isOfFileType(typ) )
    {
	BufferString msg( "File has wrong file type: '" );
	msg += astream.fileType();
	msg += "' should be: '"; msg += typ; msg += "'";
	ErrMsg( msg );
    }
    else
	getFrom( astream );
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

    if ( typ && !strcmp(typ,"_pretty") )
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
    if ( !name().isEmpty() )
	strm << "> " << name() << " <\n";

    int maxlen = 0;
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( keys_[idx]->size() > maxlen )
	    maxlen = keys_[idx]->size();
    }
    if ( maxlen == 0 ) return;

    for ( int idx=0; idx<size(); idx++ )
    {
	const BufferString& ky = *keys_[idx];
	if ( ky == "->" )
	    { strm << "\n\n* " << vals_.get(idx) << " *\n\n"; continue; }

	int extra = maxlen - ky.size();
	BufferString toprint;
	for ( int ispc=0; ispc<extra; ispc++ )
	    toprint += " ";
	toprint += ky;
	toprint += " : ";
	toprint += vals_.get(idx);
	strm << toprint << '\n';
    }
}
