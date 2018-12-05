/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : 12-1-2004
-*/


#include "datainpspec.h"
#include "iopar.h"
#include "ptrman.h"
#include "staticstring.h"
#include "enums.h"


const char* DataInpSpec::valuestr = "Val";


DataInpSpec::DataInpSpec( InpDataType t )
    : tp_(t), prefempty_(true)
{}


DataInpSpec::DataInpSpec( const DataInpSpec& o )
    : tp_(o.tp_), prefempty_(true)
    , nameidxs_(o.nameidxs_), names_(o.names_)
{}


InpDataType DataInpSpec::type() const
{ return tp_; }


bool DataInpSpec::isInsideLimits(int idx) const
{
    if ( hasLimits() )
	{ pErrMsg("function must be defined on inheriting class"); }

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
    return Conv::to<int>( text(idx) );
}


od_int64 DataInpSpec::getInt64Value( int idx ) const
{
    return Conv::to<od_int64>( text(idx) );
}


double DataInpSpec::getDValue( int idx ) const
{
    return Conv::to<double>( text(idx) );
}


float DataInpSpec::getFValue( int idx ) const
{
    return Conv::to<float>( text(idx) );
}


bool DataInpSpec::getBoolValue( int idx ) const
{ return (bool)getIntValue(idx); }


void DataInpSpec::setValue( int i, int idx )
{ setText( toString( i ),idx); }


void DataInpSpec::setValue( od_int64 i, int idx )
{ setText( toString( i ),idx); }


void DataInpSpec::setValue( double d, int idx )
{ setText( toString( d ),idx); }


void DataInpSpec::setValue( float f, int idx )
{ setText( toString( f ),idx); }


void DataInpSpec::setValue( bool b, int idx )
{ setValue( ((int)b), idx ); }


int DataInpSpec::getDefaultIntValue( int idx ) const
{ return mUdf(int); }


double DataInpSpec::getDefaultDValue( int idx ) const
{ return mUdf(double); }


float DataInpSpec::getDefaultFValue( int idx ) const
{ return mUdf(float); }


bool DataInpSpec::getDefaultBoolValue( int idx ) const
{ return false; }


const char* DataInpSpec::getDefaultStringValue( int idx ) const
{ return ""; }


void DataInpSpec::setType( InpDataType t )
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
	nameidxs_.removeSingle( nmidx );
	names_.removeSingle( nmidx );
    }

    nameidxs_ += idx; names_.add( nm );
    return *this;
}


StringInpSpec::StringInpSpec( const char* s )
    : DataInpSpec( InpDataTypeImpl<const char*>() )
    , isUndef_(s?false:true), str_( s )
{}


bool StringInpSpec::isUndef( int idx ) const
{ return isUndef_; }


DataInpSpec* StringInpSpec::clone() const
{ return new StringInpSpec( *this ); }


const char* StringInpSpec::text() const
{
    return isUndef() ? "" : str_.buf();
}

bool StringInpSpec::setText( const char* s, int idx )
{
    str_ = s; isUndef_ = s ? false : true;
    return true;
}


const char* StringInpSpec::text( int idx ) const
{
    return text();
}


void StringInpSpec::setDefaultValue( const char* s, int idx )
{
    defaultstr_ = s;
}


const char* StringInpSpec::getDefaultStringValue( int idx ) const
{
    return defaultstr_;
}


FileNameInpSpec::FileNameInpSpec( const char* fname )
    : StringInpSpec( fname )
{
    setType( InpDataTypeImpl<const char*>( InpDataType::filename ) );
}


DataInpSpec* FileNameInpSpec::clone() const
{ return new FileNameInpSpec( *this ); }



BoolInpSpec::BoolInpSpec( bool yesno, const uiString& truetxt,
			  const uiString& falsetxt, bool setyn )
    : DataInpSpec( InpDataTypeImpl<bool>() )
    , truetext_(!truetxt.isEmpty() ? truetxt : uiStrings::sYes() )
    , yn_(yesno)
    , defaultyn_(true)
    , isset_(setyn)
{
    if ( !falsetxt.isEmpty() ) falsetext_ = falsetxt;
    if ( !truetext_.isEmpty() )
	setName( toString(truetext_), 0 );
    if ( !falsetext_.isEmpty() )
	setName( toString(falsetext_), 1 );
}



BoolInpSpec::BoolInpSpec( const BoolInpSpec& oth )
    : DataInpSpec( oth )
    , truetext_( oth.truetext_ )
    , falsetext_( oth.falsetext_ )
    , yn_( oth.yn_ )
    , defaultyn_( oth.defaultyn_ )
    , isset_(oth.isset_)
{}


bool BoolInpSpec::isUndef( int idx ) const
{ return false; }


DataInpSpec* BoolInpSpec::clone() const
{ return new BoolInpSpec( *this ); }


uiString BoolInpSpec::trueFalseTxt( bool tf ) const
{ return tf ? truetext_ : falsetext_; }


void BoolInpSpec::setTrueFalseTxt( bool tf, const uiString& txt )
{
    if ( tf )
	truetext_=txt;
    else
	falsetext_=txt;

    setName( toString(txt), tf ? 0 : 1 );
}


bool BoolInpSpec::checked() const
{ return yn_; }


void BoolInpSpec::setChecked( bool yesno )
{ yn_ = yesno; isset_ = true;}


const char* BoolInpSpec::text( int idx ) const
{
    mDeclStaticString( ret );
    ret.set( yn_ ? toString(truetext_) : toString(falsetext_) );
    return ret.str();
}


bool BoolInpSpec::setText( const char* s, int idx )
{
    yn_ = falsetext_.getString() != s;
    isset_ = true;
    return true;
}


bool BoolInpSpec::getBoolValue( int idx ) const
{ return yn_; }


void BoolInpSpec::setValue( bool b, int idx )
{ yn_ = b; isset_ = true; }


bool BoolInpSpec::getDefaultBoolValue( int idx ) const
{ return defaultyn_; }


void BoolInpSpec::setDefaultValue( bool b, int idx )
{ defaultyn_ = b; }


StringListInpSpec::StringListInpSpec( const BufferStringSet& bss )
    : DataInpSpec( InpDataTypeImpl<const char*> (InpDataType::list) )
    , cur_(0)
    , defaultval_(0)
    , isset_(0)
    , enumdef_( 0 )
{
    for ( int idx=0; idx<bss.size(); idx++ )
	strings_ += toUiString( bss.get(idx) );
}


StringListInpSpec::StringListInpSpec( const char** sl )
    : DataInpSpec( InpDataTypeImpl<const char*>(InpDataType::list) )
    , cur_(0)
    , defaultval_(0)
    , isset_(0)
    , enumdef_( 0 )
{
    if ( !sl ) return;
    for ( int idx=0; sl[idx]; idx++ )
	strings_.add( toUiString(sl[idx]) );
}

StringListInpSpec::StringListInpSpec( const uiString* strs )
    : DataInpSpec( InpDataTypeImpl<const char*>(InpDataType::list) )
    , cur_(0)
    , defaultval_(0)
    , isset_(0)
    , enumdef_( 0 )
{
    for ( int idx=0; !strs[idx].isEmpty(); idx++ )
	strings_.add( strs[idx] );
}


StringListInpSpec::StringListInpSpec( const EnumDef& enums )
    : DataInpSpec( InpDataTypeImpl<const char*>(InpDataType::list) )
    , cur_(0)
    , defaultval_(0)
    , isset_(0)
    , enumdef_(  0 )
{
    setEnumDef( enums );
}


StringListInpSpec::StringListInpSpec( const StringListInpSpec& oth )
    : DataInpSpec( oth )
    , cur_(oth.cur_)
    , defaultval_(oth.defaultval_)
    , isset_(oth.isset_)
    , enumdef_(oth.enumdef_)
{
    for ( int idx=0; idx<oth.strings_.size(); idx++ )
	strings_.add( oth.strings_[idx] );
}


StringListInpSpec::StringListInpSpec( const uiStringSet& sl )
    : DataInpSpec( InpDataTypeImpl<const char*> (InpDataType::list) )
    , cur_(0)
    , defaultval_(0)
    , isset_(0)
    , enumdef_( 0 )
{
    for ( int idx=0; idx<sl.size(); idx++ )
	strings_.add( sl[idx] );
}


StringListInpSpec::~StringListInpSpec()
{}


bool StringListInpSpec::isUndef( int idx ) const
{ return strings_.isEmpty() || cur_ < 0; }


DataInpSpec* StringListInpSpec::clone() const
{ return new StringListInpSpec( *this ); }


const uiStringSet& StringListInpSpec::strings() const
{ return strings_; }


void StringListInpSpec::addString( const uiString& txt )
{ strings_.add( txt ); }


void StringListInpSpec::setEnumDef( const EnumDef& enums )
{
    strings_.setEmpty();
    enumdef_ = &enums;

    for ( int idx=0; idx<enums.size(); idx++ )
	strings_.add( enums.getUiStringForIndex(idx) );
}



const char* StringListInpSpec::text( int ) const
{
    if ( isUndef() )
	return OD::EmptyString();
    if ( enumdef_ )
	return enumdef_->getKeyForIndex(cur_);

    mDeclStaticString( ret );
    ret = toString( strings_[cur_] );
    return ret.buf();
}


void StringListInpSpec::setItemText( int idx, const uiString& s )
{
    strings_[cur_] = s;
}


bool StringListInpSpec::setText( const char* s, int nr )
{
    if ( enumdef_ )
    {
	if ( enumdef_->isValidKey( s ) )
	{
	    cur_ = enumdef_->indexOf( s );
	    isset_ = true;
	    return true;
	}
    }
    else
    {
	for ( int idx=0; idx<strings_.size(); idx++ )
	{
	    if ( strings_[idx].getString() == s )
		{ cur_ = idx; isset_ = true; return true; }
	}
    }

    return false;
}


int StringListInpSpec::getIntValue( int idx ) const
{ return cur_; }

od_int64 StringListInpSpec::getInt64Value( int idx ) const
{ return cur_; }


double StringListInpSpec::getDValue( int idx ) const
{ return cur_; }


float StringListInpSpec::getFValue( int idx ) const
{ return (float)cur_; }


void StringListInpSpec::setValue( int i, int idx )
{ if ( i < strings_.size() ) cur_ = i; isset_ = true; }

void StringListInpSpec::setValue( od_int64 i, int idx )
{ setValue( (int)i, idx ); }


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
    s.wantcoords_  ? (idx == 0 ? (float)s.coord_.x_ : (float)s.coord_.y_) \
	   : (s.is2d_ ? (idx == 0 ? (float)s.binid_.crl() : s.offs_) \
		      : (float)(idx == 0 ? s.binid_.inl() : s.binid_.crl()) ) \
			) \
   )

#define mSetMemb(s,idx,f) { \
    if ( idx > 1 || (s.is2d_ && !s.wantcoords_ && idx == 1) ) \
	s.offs_ = f; \
    else if ( s.wantcoords_  ) \
	(idx == 0 ? s.coord_.x_ : s.coord_.y_) = f; \
    else if ( !s.is2d_ && idx == 0 ) \
      s.binid_.inl() = mNINT32(f); \
    else \
      s.binid_.crl() = mNINT32(f); \
}


PositionInpSpec::PositionInpSpec( const PositionInpSpec::Setup& s )
    : DataInpSpec( InpDataTypeImpl<float>(InpDataType::position) )
    , setup_(s)
{
    defsetup_.clear();
}


PositionInpSpec::PositionInpSpec( const BinID& bid, bool isps )
    : DataInpSpec( InpDataTypeImpl<float>(InpDataType::position) )
{
    setup_ = Setup( false, false, isps );
    setup_.binid_ = bid;
}


PositionInpSpec::PositionInpSpec( const Coord& c, bool isps, bool is2d )
    : DataInpSpec( InpDataTypeImpl<float>(InpDataType::position) )
{
    setup_ = Setup( true, is2d, isps );
    setup_.coord_ = c;
}


PositionInpSpec::PositionInpSpec( int trcnr, bool isps )
    : DataInpSpec( InpDataTypeImpl<float>(InpDataType::position) )
{
    setup_ = Setup( false, true, isps );
    setup_.binid_.crl() = trcnr;
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
    return mIsUdf(setup_.coord_.x_) ? Coord(udfval,udfval) : setup_.coord_;
}


BinID PositionInpSpec::getBinID( int udfval ) const
{
    return mIsUdf(setup_.binid_.crl()) ? BinID(udfval,udfval) : setup_.binid_;
}


int PositionInpSpec::getTrcNr( int udfval ) const
{
    return mIsUdf(setup_.binid_.crl()) ? udfval : setup_.binid_.crl();
}


float PositionInpSpec::getOffset( float udfval ) const
{
    return mIsUdf(setup_.offs_) ? udfval : setup_.offs_;
}


const char* PositionInpSpec::text( int idx ) const
{
    mDeclStaticString( ret );
    ret.set( getVal(setup_,idx) );
    return ret.buf();
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
