/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : 12-1-2004
-*/

static const char* rcsID mUnusedVar = "$Id: datainpspec.cc,v 1.38 2012-07-10 08:05:31 cvskris Exp $";

#include "datainpspec.h"
#include "iopar.h"
#include "ptrman.h"


const char* DataInpSpec::valuestr = "Val";


DataInpSpec::DataInpSpec( DataType t )
    : tp_(t), prefempty_(true)
{}


DataInpSpec::DataInpSpec( const DataInpSpec& o )
    : tp_(o.tp_), prefempty_(true)
    , nameidxs_(o.nameidxs_), names_(o.names_)
{}


DataType DataInpSpec::type() const
{ return tp_; }


bool DataInpSpec::isInsideLimits(int idx) const
{
    if ( hasLimits() )
	pErrMsg("function must be defined on inheriting class");

    return !isUndef(idx);
}


void DataInpSpec::fillPar(IOPar& par) const
{
    for ( int idx=0; idx<nElems(); idx++ )
    {
	IOPar subpar;
	subpar.set(valuestr, text(idx) );
	par.mergeComp( subpar, toString(idx) );
    }
}


bool DataInpSpec::usePar( const IOPar& par )
{
    BufferStringSet values;
    for ( int idx=0; idx<nElems(); idx++ )
    {
	PtrMan<IOPar> subpar = par.subselect( toString(idx) );
	if ( !subpar ) return false;
	const char* valptr = subpar->find( valuestr );
	if ( !valptr || !*valptr )
	    return false;

	values.add( valptr );
    }

    for ( int idx=0; idx<nElems(); idx++ )
    {
	if ( !setText( values.get(idx), idx ) )
	    return false;
    }

    return true;
}


int DataInpSpec::getIntValue( int idx ) const
{
    int res;
    const char* valstr = text(idx);
    return valstr && getFromString(res, valstr,mUdf(int)) ? res : mUdf(int);
}


double DataInpSpec::getdValue( int idx ) const
{
    double res;
    const char* valstr = text(idx);
    return valstr && getFromString(res,valstr,mUdf(double))? res : mUdf(double);
}


float DataInpSpec::getfValue( int idx ) const
{
    float res;
    const char* valstr = text(idx);
    return valstr && getFromString(res,valstr,mUdf(float)) ? res : mUdf(float);
}


bool DataInpSpec::getBoolValue( int idx ) const
{ return (bool)getIntValue(idx); }


void DataInpSpec::setValue( int i, int idx )
{ setText( toString( i ),idx); }


void DataInpSpec::setValue( double d, int idx )
{ setText( toString( d ),idx); }


void DataInpSpec::setValue( float f, int idx )
{ setText( toString( f ),idx); }


void DataInpSpec::setValue( bool b, int idx )
{ setValue( ((int)b), idx ); }


int DataInpSpec::getDefaultIntValue( int idx ) const
{
    return mUdf(int);
}


double DataInpSpec::getDefaultValue( int idx ) const
{
    return mUdf(double);
}


float DataInpSpec::getDefaultfValue( int idx ) const
{
    return mUdf(float);
}


bool DataInpSpec::getDefaultBoolValue( int idx ) const
{ return false; }


const char* DataInpSpec::getDefaultStringValue( int idx ) const
{ return ""; }


void DataInpSpec::setType( DataType t )
{ tp_ = t; }


const char* DataInpSpec::name( int idx ) const
{
    const int nmidx = nameidxs_.indexOf( idx );
    if ( nmidx < 0 ) 
	return 0;
    return names_.get( nmidx );
}


DataInpSpec& DataInpSpec::setName( const char* nm, int idx )
{
    const int nmidx = nameidxs_.indexOf( idx );
    if ( nmidx>=0 )
    {
	nameidxs_.remove( nmidx );
	names_.remove( nmidx );
    }

    nameidxs_ += idx; names_.add( nm );
    return *this;
}


StringInpSpec::StringInpSpec( const char* s )
    : DataInpSpec( DataTypeImpl<const char*>() )
    , isUndef_(s?false:true), str( s )
{}


bool StringInpSpec::isUndef( int idx ) const
{ return isUndef_; }


DataInpSpec* StringInpSpec::clone() const
{ return new StringInpSpec( *this ); }


const char* StringInpSpec::text() const
{ 
    if ( isUndef() ) return "";
    return (const char*) str;
}

bool StringInpSpec::setText( const char* s, int idx )
{
    str = s; isUndef_ = s ? false : true;
    return true;
}


const char* StringInpSpec::text( int idx ) const
{
    return text();
}


void StringInpSpec::setDefaultValue( const char* s, int idx )
{
    defaultstr = s;
}


const char* StringInpSpec::getDefaultStringValue( int idx ) const
{
    return defaultstr;
}


FileNameInpSpec::FileNameInpSpec( const char* fname )
    : StringInpSpec( fname )
{
    setType( DataTypeImpl<const char*>( DataType::filename ) );
}


DataInpSpec* FileNameInpSpec::clone() const
{ return new FileNameInpSpec( *this ); }



BoolInpSpec::BoolInpSpec( bool yesno, const char* truetxt,
			  const char* falsetxt, bool setyn )
    : DataInpSpec( DataTypeImpl<bool>() )
    , truetext(truetxt ? truetxt : (const char*) sKey::Yes() )
    , yn(yesno)
    , defaultyn(true)
    , isset(setyn)
{
    if ( falsetxt ) falsetext = falsetxt;
    if ( !truetext.isEmpty() )
	setName( truetext, 0 );
    if ( !falsetext.isEmpty() )
	setName( falsetext, 1 );
}



BoolInpSpec::BoolInpSpec( const BoolInpSpec& oth )
    : DataInpSpec( oth )
    , truetext( oth.truetext )
    , falsetext( oth.falsetext )
    , yn( oth.yn )
    , defaultyn( oth.defaultyn )
    , isset(oth.isset)
{}


bool BoolInpSpec::isUndef( int idx ) const
{ return false; }


DataInpSpec* BoolInpSpec::clone() const
{ return new BoolInpSpec( *this ); }


const char* BoolInpSpec::trueFalseTxt( bool tf ) const
{ return tf ? truetext : falsetext; }


void BoolInpSpec::setTrueFalseTxt( bool tf, const char* txt )
{
    if ( tf )
	truetext=txt;
    else
	falsetext=txt;

    setName( txt, tf ? 0 : 1 );
}


bool BoolInpSpec::checked() const 
{ return yn; }


void BoolInpSpec::setChecked( bool yesno )
{ yn = yesno; isset = true;}


const char* BoolInpSpec::text( int idx ) const
{
    return yn ? (const char*)truetext
	      : (const char*)falsetext;
}


bool BoolInpSpec::setText( const char* s, int idx )
{
    yn = s && strcmp(s,falsetext);
    isset = true;
    return true;
}


bool BoolInpSpec::getBoolValue( int idx ) const
{ return yn; }


void BoolInpSpec::setValue( bool b, int idx )
{ yn = b; isset = true; }


bool BoolInpSpec::getDefaultBoolValue( int idx ) const
{ return defaultyn; }


void BoolInpSpec::setDefaultValue( bool b, int idx )
{ defaultyn = b; }


StringListInpSpec::StringListInpSpec( const BufferStringSet& bss )
    : DataInpSpec( DataTypeImpl<const char*> (DataType::list) )
    , strings_(bss)
    , cur_(0)
    , defaultval_(0)
    , isset_(0)
{}


StringListInpSpec::StringListInpSpec( const char** sl )
    : DataInpSpec( DataTypeImpl<const char*>(DataType::list) )
    , cur_(0)
    , defaultval_(0)
    , isset_(0)
{
    if ( !sl ) return;
    for ( int idx=0; sl[idx]; idx++ )
	strings_.add( sl[idx] );
}


StringListInpSpec::StringListInpSpec( const StringListInpSpec& oth )
    : DataInpSpec( oth )
    , cur_(oth.cur_)
    , defaultval_(oth.defaultval_)
    , isset_(oth.isset_)
{ strings_ = oth.strings_; }


StringListInpSpec::~StringListInpSpec()
{}


bool StringListInpSpec::isUndef( int idx ) const
{ return strings_.isEmpty() || cur_ < 0; }


DataInpSpec* StringListInpSpec::clone() const
{ return new StringListInpSpec( *this ); }


const BufferStringSet& StringListInpSpec::strings() const
{ return strings_; }


void StringListInpSpec::addString( const char* txt )
{ strings_.add( txt ); }


const char* StringListInpSpec::text( int idx ) const
{
    return isUndef() ? "" : strings_.get( cur_ ).buf();
}


void StringListInpSpec::setItemText( int idx, const char* s )
{ strings_.get( cur_ ) = s; }


bool StringListInpSpec::setText( const char* s, int nr )
{
    for ( int idx=0; idx<strings_.size(); idx++ )
    {
	if ( strings_.get(idx) == s )
	{ cur_ = idx; isset_ = true; return true; }
    }

    return false;
}


int StringListInpSpec::getIntValue( int idx ) const
{ return cur_; }


double StringListInpSpec::getdValue( int idx ) const
{ return cur_; }


float StringListInpSpec::getfValue( int idx ) const
{ return cur_; }


void StringListInpSpec::setValue( int i, int idx )
{ if ( i < strings_.size() ) cur_ = i; isset_ = true; }


void StringListInpSpec::setValue( double d, int idx )
{
    if ( (int)(d+.5) < strings_.size() )
    {
	cur_ = (int)(d+.5);
	isset_ = true;
    }
}


void StringListInpSpec::setValue( float f, int idx )
{
    if ( (int)(f+.5) < strings_.size() )
    {
	cur_ = (int)(f+.5);
	isset_ = true;
    }
}


int StringListInpSpec::getDefaultIntValue( int idx ) const
{ return defaultval_; }


void StringListInpSpec::setDefaultValue( int i, int idx )
{ if ( i < strings_.size() ) defaultval_ = i; }


#define mGetMembAsFloat(s,idx) ( \
    idx > 1 ? s.offs_ :	( \
    s.wantcoords_  ? (idx == 0 ? (float)s.coord_.x : (float)s.coord_.y) \
	   : (s.is2d_ ? (idx == 0 ? (float)s.binid_.crl : s.offs_) \
		      : (float)(idx == 0 ? s.binid_.inl : s.binid_.crl) ) \
			) \
   )

#define mSetMemb(s,idx,f) { \
    if ( idx > 1 || (s.is2d_ && !s.wantcoords_ && idx == 1) ) \
	s.offs_ = f; \
    else if ( s.wantcoords_  ) \
       	(idx == 0 ? s.coord_.x : s.coord_.y) = f; \
    else if ( !s.is2d_ && idx == 0 ) \
      s.binid_.inl = mNINT32(f); \
    else \
      s.binid_.crl = mNINT32(f); \
}


PositionInpSpec::PositionInpSpec( const PositionInpSpec::Setup& s )
    : DataInpSpec( DataTypeImpl<float>(DataType::position) )
    , setup_(s)
{
    defsetup_.clear();
}


PositionInpSpec::PositionInpSpec( const BinID& bid, bool isps )
    : DataInpSpec( DataTypeImpl<float>(DataType::position) )
{
    setup_ = Setup( false, false, isps );
    setup_.binid_ = bid;
}


PositionInpSpec::PositionInpSpec( const Coord& c, bool isps, bool is2d )
    : DataInpSpec( DataTypeImpl<float>(DataType::position) )
{
    setup_ = Setup( true, is2d, isps );
    setup_.coord_ = c;
}


PositionInpSpec::PositionInpSpec( int trcnr, bool isps )
    : DataInpSpec( DataTypeImpl<float>(DataType::position) )
{
    setup_ = Setup( false, true, isps );
    setup_.binid_.crl = trcnr;
}


int PositionInpSpec::nElems() const
{
    const bool usetrcnr = setup_.is2d_ && !setup_.wantcoords_;
    int nr = setup_.isps_ ? 1 : 0;
    nr += usetrcnr ? 1 : 2;
    return nr;
}


bool PositionInpSpec::isUndef( int idx ) const
{
    if ( idx < 0 || idx > 2 || (!setup_.isps_ && idx > 1) )
	return true;

    const float v = getVal( setup_, idx );
    return mIsUdf(v);
}


Coord PositionInpSpec::getCoord( double udfval ) const
{
    return mIsUdf(setup_.coord_.x) ? Coord(udfval,udfval) : setup_.coord_;
}


BinID PositionInpSpec::getBinID( int udfval ) const
{
    return mIsUdf(setup_.binid_.crl) ? BinID(udfval,udfval) : setup_.binid_;
}


int PositionInpSpec::getTrcNr( int udfval ) const
{
    return mIsUdf(setup_.binid_.crl) ? udfval : setup_.binid_.crl;
}


float PositionInpSpec::getOffset( float udfval ) const
{
    return mIsUdf(setup_.offs_) ? udfval : setup_.offs_;
}


const char* PositionInpSpec::text( int idx ) const
{
    static char ret[255];
    const float v = getVal( setup_, idx );
    getStringFromFloat( 0, v, ret );
    return ret;
}


bool PositionInpSpec::setText( const char* s, int idx ) 
{
    setVal( setup_, idx, toFloat(s) );
    return true;
}


void PositionInpSpec::setVal( Setup& s, int idx, float f )
{
    mSetMemb( s, idx, f );
}


float PositionInpSpec::getVal( const Setup& s, int idx ) const
{
    return mGetMembAsFloat(s,idx);
}
