/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-12-1995
-*/

static const char* rcsID = "$Id: iopar.cc,v 1.9 2000-11-27 15:25:27 bert Exp $";

#include "iopar.h"
#include "ascstream.h"
#include "aobset.h"
#include "position.h"
#include "separstr.h"
#include <ctype.h>

static FileMultiString fms;


IOPar::IOPar( const char* nm )
	: UserIDObject(nm)
	, pars_(*new AliasObjectSet(this))
{
}


IOPar::IOPar( UserIDObject* u )
	: UserIDObject(u)
	, pars_(*new AliasObjectSet(this))
{
}


IOPar::IOPar( const IOPar& iop )
	: UserIDObject(iop.name())
	, pars_(*new AliasObjectSet(this))
{
    for ( int idx=0; idx<iop.pars_.size(); idx++ )
	add( iop.pars_[idx]->name(), iop.pars_[idx]->obj->name() );
}


IOPar& IOPar::operator=( const IOPar& iop )
{
    if ( this != &iop )
    {
	clear();
	setName( iop.name() );
	for ( int idx=0; idx<iop.pars_.size(); idx++ )
	    add( iop.pars_[idx]->name(), iop.pars_[idx]->obj->name() );
    }
    return *this;
}


IOPar::~IOPar()
{
    clear();
    delete &pars_;
}


int IOPar::size() const
{
    return pars_.size();
}


const char* IOPar::getKey( int nr ) const
{
    if ( nr >= size() ) return "";
    return pars_[nr]->name();
}


const char* IOPar::getValue( int nr ) const
{
    if ( nr >= size() ) return "";
    return pars_[nr]->obj->name();
}


bool IOPar::setKey( int nr, const char* s )
{
    if ( nr >= size() || !s || !*s || pars_.indexOf(s) >= 0 )
	return false;

    pars_[nr]->setName( s );
    return true;
}


void IOPar::setValue( int nr, const char* s )
{
    if ( nr < size() ) pars_[nr]->obj->setName( s );
}


void IOPar::clear()
{
    pars_.deepEraseWithObjs();
}


void IOPar::remove( int idx )
{
    if ( idx >= size() ) return;
    
    AliasObject* aob = pars_[idx];
    pars_ -= aob;
    delete aob->obj; delete aob;
}


void IOPar::merge( const IOPar& iopar )
{
    for ( int idx=0; idx<iopar.pars_.size(); idx++ )
	set( iopar.pars_[idx]->name(), iopar.pars_[idx]->obj->name() );
}


const char* IOPar::compKey( const char* key1, const char* key2 )
{
    static BufferString ret;
    ret = key1;
    if ( key1 && key2 && *key1 && *key2 ) ret += ".";
    ret += key2;
    return ret;
}


IOPar* IOPar::subselect( const char* key ) const
{
    if ( !key ) return 0;

    IOPar* iopar = new IOPar( name() );
    for ( int idx=0; idx<pars_.size(); idx++ )
    {
	const char* nm = pars_[idx]->name();
	if ( !matchString(key,nm) ) continue;
	nm += strlen(key);
	if ( *nm == '.' && *(nm+1) )
	    iopar->add( nm+1, pars_[idx]->obj->name() );
    }

    if ( iopar->pars_.size() == 0 )
	{ delete iopar; iopar = 0; }
    return iopar;
}


void IOPar::mergeComp( const IOPar& iopar, const char* key )
{
    static BufferString buf;

    for ( int idx=0; idx<iopar.pars_.size(); idx++ )
    {
	buf = key;
	buf += ".";
	buf += iopar.pars_[idx]->name();
	set( buf, iopar.pars_[idx]->obj->name() );
    }
}


const char* IOPar::findKeyFor( const char* s, int nr ) const
{
    if ( !s ) return 0;

    for ( int idx=0; idx<pars_.size(); idx++ )
    {
	if ( pars_[idx]->obj->name() == s )
	{
	    if ( nr )	nr--;
	    else	return (const char*)pars_[idx]->name();
	}
    }

    return 0;
}


void IOPar::removeWithKey( const char* key )
{
    AliasObject* aob = pars_[key];
    while ( aob )
    {
	pars_ -= aob;
	delete aob->obj; delete aob;
	aob = pars_[key];
    }
}


const char* IOPar::operator[]( const char* keyw ) const
{
    const char* res = find( keyw );
    return res ? res : "";
}


const char* IOPar::find( const char* keyw ) const
{
    AliasObject* aob = pars_[keyw];
    return aob && aob->obj ? (const char*)aob->obj->name() : 0;
}


void IOPar::add( const char* nm, const char* val )
{
    UserIDObject* valstr = new UserIDObject( val );
    pars_ += new AliasObject( valstr, nm );
}


bool IOPar::get( const char* s, int& i ) const
{
    const char* ptr = (*this)[s];
    if ( ptr && *ptr ) { i = atoi(ptr); return YES; }
    return NO;
}


bool IOPar::get( const char* s, int& i1, int& i2 ) const
{
    const char* ptr = (*this)[s];
    if ( ptr && *ptr )
    {
	fms = ptr;
	ptr = fms[0];
	if ( *ptr ) i1 = atoi( ptr );
	ptr = fms[1];
	if ( *ptr ) i2 = atoi( ptr );
	return YES;
    }
    return NO;
}


bool IOPar::getSc( const char* s, float& f, float sc, bool udf ) const
{
    const char* ptr = (*this)[s];
    if ( ptr && *ptr )
    {
	f = atof( ptr );
	if ( !mIsUndefined(f) ) f *= sc;
	return YES;
    }
    else if ( udf )
	f = mUndefValue;

    return NO;
}


bool IOPar::getSc( const char* s, double& d, double sc, bool udf ) const
{
    const char* ptr = (*this)[s];
    if ( ptr && *ptr )
    {
	d = atof( ptr );
	if ( !mIsUndefined(d) ) d *= sc;
	return YES;
    }
    else if ( udf )
	d = mUndefValue;

    return NO;
}


bool IOPar::getSc( const char* s, float& f1, float& f2, float sc,
		   bool udf ) const
{
    double d1=f1, d2=f2;
    if ( getSc( s, d1, d2, sc, udf ) )
	{ f1 = (float)d1; f2 = (float)d2; return YES; }
    return NO;
}


bool IOPar::getSc( const char* s, double& d1, double& d2, double sc,
		 bool udf ) const
{
    const char* ptr = (*this)[s];
    if ( udf || *ptr )
    {
	fms = ptr;
	ptr = fms[0];
	if ( *ptr )
	{
	    d1 = atof( ptr );
	    if ( !mIsUndefined(d1) ) d1 *= sc;
	}
	else if ( udf )
	    d1 = mUndefValue;

	ptr = fms[1];
	if ( *ptr )
	{
	    d2 = atof( ptr );
	    if ( !mIsUndefined(d2) ) d2 *= sc;
	}
	else if ( udf )
	    d2 = mUndefValue;

	return YES;
    }
    return NO;
}


bool IOPar::get( const char* s, int& i1, int& i2, int& i3 ) const
{
    const char* ptr = (*this)[s];
    if ( ptr && *ptr )
    {
	fms = ptr;
	ptr = fms[0];
	if ( *ptr ) i1 = atoi( ptr );
	ptr = fms[1];
	if ( *ptr ) i2 = atoi( ptr );
	ptr = fms[2];
	if ( *ptr ) i3 = atoi( ptr );
	return YES;
    }
    return NO;
}


bool IOPar::getYN( const char* s, bool& i, char c ) const
{
    const char* ptr = (*this)[s];
    if ( !ptr || !*ptr ) return NO;

    if ( !c )	i = yesNoFromString(ptr);
    else	i = toupper(*ptr) == toupper(c);
    return YES;
}


void IOPar::set( const char* keyw, const char* vals )
{
    AliasObject* par = pars_[keyw];
    if ( !par )
	add( keyw, vals );
    else
	par->obj->setName( vals );
}


void IOPar::set( const char* keyw, int val )
{
    set( keyw, getStringFromInt(0,val) );
}


void IOPar::set( const char* keyw, float val )
{
    set( keyw, mIsUndefined(val) ? sUndefValue
				 : getStringFromFloat(0,val) );
}


void IOPar::set( const char* keyw, double val )
{
    set( keyw, mIsUndefined(val) ? sUndefValue : getStringFromDouble(0,val) );
}


void IOPar::set( const char* s, int i1, int i2 )
{
    fms = getStringFromInt(0,i1);
    fms.add( getStringFromInt(0,i2) );
    set( s, fms );
}


void IOPar::set( const char* s, float f1, float f2 )
{
    fms = mIsUndefined(f1) ? sUndefValue : getStringFromFloat(0,f1);
    fms.add( mIsUndefined(f2) ? sUndefValue : getStringFromFloat(0,f2) );
    set( s, fms );
}


void IOPar::set( const char* s, double d1, double d2 )
{
    fms = mIsUndefined(d1) ? sUndefValue : getStringFromDouble(0,d1);
    fms.add( mIsUndefined(d2) ? sUndefValue : getStringFromDouble(0,d2) );
    set( s, fms );
}


void IOPar::set( const char* s, int i1, int i2, int i3 )
{
    fms = getStringFromInt(0,i1);
    fms.add( getStringFromInt(0,i2) );
    fms.add( getStringFromInt(0,i3) );
    set( s, fms );
}


void IOPar::setYN( const char* keyw, bool i )
{
    set( keyw, getYesNoString(i) );
}


bool IOPar::get( const char* s, Coord& coord ) const
{ return get( s, coord.x, coord.y ); }
void IOPar::set( const char* s, const Coord& coord )
{ set( s, coord.x, coord.y ); }
bool IOPar::get( const char* s, BinID& binid ) const
{ return get( s, binid.inl, binid.crl ); }
void IOPar::set( const char* s, const BinID& binid )
{ set( s, binid.inl, binid.crl ); }


IOPar::IOPar( ascistream& astream, bool withname )
	: UserIDObject(withname?astream.keyWord():"")
	, pars_(*new AliasObjectSet(this))
{
    if ( withname ) astream.next();
    pars_.getFrom( astream );
}


void IOPar::putTo( ascostream& astream, bool withname ) const
{
    astream.tabsOff();
    if ( withname ) astream.put( name() );
    pars_.putTo( astream );
    astream.newParagraph();
}


static const char* sersep = "!^~";

void IOPar::putTo( BufferString& str ) const
{
    str = name();
    BufferString buf;
    for ( int idx=0; idx<pars_.size(); idx++ )
    {
	buf = sersep;
	buf += pars_[idx]->name();
	buf += sersep;
	buf += pars_[idx]->obj->name();
	str += buf;
    }
}


void IOPar::getFrom( const char* str )
{
    clear();

    BufferString buf = str;
    char* ptrstart = buf;
    char* ptr = ptrstart;

    bool name_done = false;
    AliasObject* aob = 0;
    while ( *ptr )
    {
	// advance to next separator or end of string
	while ( *ptr && *ptr != *sersep )
	    ptr++;
	if ( *ptr && (*(ptr+1) != *(sersep+1) || *(ptr+2) != *(sersep+2)) )
	    { ptr++; continue; }

	// skip separator
	if ( *ptr )
	    { *ptr++ = '\0'; if ( *ptr ) ptr++; if ( *ptr ) ptr++; }

	if ( !name_done )
	    { setName( ptrstart ); name_done = true; }
	else if ( !aob )
	{
	    aob = new AliasObject( new UserIDObject, ptrstart );
	    pars_ += aob;
	}
	else
	{
	    aob->obj->setName( ptrstart );
	    aob = 0;
	}

	ptrstart = ptr;
    }
}
