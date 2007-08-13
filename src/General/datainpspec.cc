/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : 12-1-2004
-*/

static const char* rcsID = "$Id: datainpspec.cc,v 1.23 2007-08-13 13:11:16 cvsjaap Exp $";

#include "datainpspec.h"
#include "iopar.h"
#include "ptrman.h"


const char* DataInpSpec::valuestr = "Val";


DataInpSpec::DataInpSpec( DataType t )
    : tp_(t), prefempty_(true)
{}


DataInpSpec::DataInpSpec( const DataInpSpec& o )
    : tp_(o.tp_), prefempty_(true)
    , nameidx_(o.nameidx_), name_(o.name_)
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
	const BufferString key(idx);
	par.mergeComp(subpar, key);
    }
}


bool DataInpSpec::usePar(const IOPar& par)
{
    BufferStringSet values;
    for ( int idx=0; idx<nElems(); idx++ )
    {
	const BufferString key(idx);
	BufferString value;
	PtrMan<IOPar> subpar = par.subselect(key);
	if ( !subpar ) return false;
	const char* valptr = (*subpar)[valuestr];
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
    const int nmidx = nameidx_.indexOf( idx );
    if ( nmidx < 0 ) 
	return 0;
    return name_[nmidx];
}


const DataInpSpec& DataInpSpec::setName( const char* nm, int idx )
{
    const int nmidx = nameidx_.indexOf( idx );
    if ( nmidx>=0 )
    {
	nameidx_.remove( nmidx ); name_.remove( nmidx );
    }
    nameidx_+=idx; name_+=nm;
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


void StringInpSpec::setDefaultStringValue( const char* s, int idx )
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
    , truetext(truetxt ? truetxt : sKey::Yes )
    , yn(yesno)
    , defaultyn(true)
    , isset(setyn)
{
    if ( falsetxt ) falsetext = falsetxt;
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
{ if ( tf ) truetext=txt; else falsetext=txt; }


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
{ deepCopy( strings_, oth.strings_ ); }


StringListInpSpec::~StringListInpSpec()
{ deepErase(strings_); }


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
    if ( isUndef() ) return "";
    else return (const char*)*strings_[cur_];
}


void StringListInpSpec::setItemText( int idx, const char* s )
{ *strings_[cur_] = s; }


bool StringListInpSpec::setText( const char* s, int nr )
{
    for ( int idx=0; idx<strings_.size(); idx++ )
    {
	if ( *strings_[idx] == s ) { cur_ = idx; isset_ = true; return true; }
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


PositionInpSpec::PositionInpSpec( bool docrd, float x_inline, float y_crossline,
				  bool isrelative, const RCol2Coord* b2c )
    : DataInpSpec( DataTypeImpl<int>(DataType::binID) )
    , x_inl_( x_inline )
    , y_crl_( y_crossline )
    , docoord_( docrd )
    , isrelative_( isrelative )
    , b2c_( b2c )
    , defaultx_inl_( mUdf(float) )
    , defaulty_crl_( mUdf(float) )
{}


DataInpSpec* PositionInpSpec::clone() const   
{ return new PositionInpSpec( *this ); }


int PositionInpSpec::nElems() const
{ return 2; }


float PositionInpSpec::value( int idx ) const
{ return idx==0 ? x_inl_ : y_crl_; }


bool PositionInpSpec::isUndef( int idx ) const
{
    if ( idx<0 || idx>1 ) return true;
    return idx==0 ? mIsUdf( x_inl_ ) : mIsUdf( y_crl_ );
} 


const char* PositionInpSpec::text( int idx ) const
{
    return isUndef(idx) ? "" : toString( value(idx) );
}


bool PositionInpSpec::setText( const char* s, int idx ) 
{ return getFromString( idx==0 ? x_inl_ : y_crl_, s); }


const char* PositionInpSpec::otherTxt() const
{
    if ( docoord_ ) { return "Inline/Xline ..."; }
    return isrelative_ ? "Distance ..." : "Coords ...";
}


const RCol2Coord* PositionInpSpec::binID2Coord() const
{ return b2c_; }


float PositionInpSpec::defaultValue( int idx ) const
{ return idx==0 ? defaultx_inl_ : defaulty_crl_; }


void PositionInpSpec::setDefaultValue( float f, int idx )
{ idx==0 ? defaultx_inl_= f : defaulty_crl_= f; }

