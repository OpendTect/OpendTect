/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: attribparam.cc,v 1.27 2007-02-05 14:32:25 cvsnanne Exp $";

#include "attribparam.h"
#include "attribparamgroup.h"

#include "ioman.h"
#include "ioobj.h"
#include "linekey.h"
#include "ptrman.h"
#include "position.h"

namespace Attrib
{

Param::Param( const char* key )
    : key_( key )
    , enabled_( true )
    , required_( true )
    , isgroup_( false )
{}


Param::Param( const Param& b )
    : key_( b.key_ )
    , enabled_( b.enabled_ )
    , required_( b.required_ )
    , isgroup_( b.isgroup_ )
{}


ValParam::ValParam( const char* key, DataInpSpec* spec )
    : Param( key )
    , spec_( spec )
{}


ValParam::ValParam( const ValParam& b )
    : Param( b.key_ )
    , spec_( b.spec_->clone() )
{
    enabled_ = b.enabled_;
    required_ = b.required_;
}


ValParam::~ValParam()
{
    delete spec_;
}


#define mParamClone( type ) \
type* type::clone() const { return new type(*this); }

mParamClone( ValParam );


bool ValParam::isEqual( const Param& b ) const
{
    mDynamicCastGet(ValParam*,vp,&const_cast<Param&>(b))

    if ( spec_->nElems() != vp->spec_->nElems() )
	return false;

    for ( int idx=0; idx<spec_->nElems(); idx++ )
    {
	BufferString buf1 = spec_->text(idx);
	BufferString buf2 = vp->spec_->text(idx);
	if ( buf1 != buf2 )
	    return false;
    }

    return true;
}


bool ValParam::isOK() const
{
    if ( !enabled_ ) return true;
    if ( spec_->isUndef() ) return !required_;

    for ( int idx=0; idx<spec_->nElems(); idx++ )
    {
	if ( !spec_->isInsideLimits(idx) )
	    return false;
    }

    return true;
}



int ValParam::nrValues() const
{ return spec_->nElems(); }


#define mSetGet(type,getfunc) \
type ValParam::getfunc( int idx ) const \
{ return spec_->getfunc(idx); } \
\
void ValParam::setValue( type val, int idx ) \
{ spec_->setValue( val, idx ); }

mSetGet(int,getIntValue)
mSetGet(float,getfValue)
mSetGet(bool,getBoolValue)
    
#define mSetGetDefault(type,getfunc) \
type ValParam::getfunc( int idx ) const \
{ return spec_->getfunc(idx); } \
\
void ValParam::setDefaultValue( type val, int idx ) \
{ spec_->setDefaultValue( val, idx ); }

mSetGetDefault(int,getDefaultIntValue)
mSetGetDefault(float,getDefaultfValue)
mSetGetDefault(bool,getDefaultBoolValue)
mSetGetDefault(const char*,getDefaultStringValue)

    
const char* ValParam::getStringValue( int idx ) const
{ return spec_->text(idx); }

void ValParam::setValue( const char* str, int idx )
{ spec_->setText( str, idx ); }


bool ValParam::setCompositeValue( const char* nv )
{
    if ( !spec_->setText(nv,0) )
	return false;

    return isOK();
};


bool ValParam::getCompositeValue( BufferString& res ) const
{
    if ( !spec_ ) return false;
    res = spec_->text();

    return true;
}


void ValParam::fillDefStr( BufferString& res ) const
{
    res += getKey();
    res += "=";
    BufferString val;
    if ( !getCompositeValue(val) && !isRequired() )
	val = getDefaultValue();
    res += val;
}


BinIDParam::BinIDParam( const char* nm )
    : ValParam( nm, new BinIDInpSpec() )
{}


mParamClone( BinIDParam );


void BinIDParam::setLimits( const Interval<int>& inlrg,
			    const Interval<int>& crlrg )
{
    /*
    TODO: implement setLimits in BinIDInpSpec
    reinterpret_cast<BinIDInpSpec*>(spec_)->setLimits( inlrg, crlrg );
    */
}


bool BinIDParam::setCompositeValue( const char* posstr )
{
    int posstrsz = strlen( posstr );

    int idx = 0;
    for ( ; idx < posstrsz; idx ++)
	if ( posstr[idx] == ',' )
	    break;

    if ( idx>=posstrsz )
	return false;

    ArrPtrMan<char> valuestr = new char[posstrsz];
    strncpy( valuestr, &posstr[0], idx  );
    valuestr[idx] = 0;

    if ( !spec_->setText(valuestr,0) )
	return false;

    strncpy( valuestr, &posstr[idx+1], posstrsz - idx - 1 );
    valuestr[posstrsz - idx - 1] = 0;

    if ( !spec_->setText(valuestr,1) )
	return false;

    return true;
}


void BinIDParam::setDefaultValue( const BinID& bid )
{
    reinterpret_cast<BinIDInpSpec*>(spec_)->setDefaultValue(bid.inl,0);
    reinterpret_cast<BinIDInpSpec*>(spec_)->setDefaultValue(bid.crl,1);
}


bool BinIDParam::getCompositeValue( BufferString& res ) const
{
    BinID bid = getValue();
    toString( res, bid );
    return true;
}


void BinIDParam::toString( BufferString& res, const BinID& bid ) const
{
    res += bid.inl;
    res += ",";
    res += bid.crl;
}


BinID BinIDParam::getValue() const
{
    BinIDInpSpec* spec = reinterpret_cast<BinIDInpSpec*>(spec_);
    const float val0 = spec->value(0);
    const float val1 = spec->value(1);
    return BinID( mIsUdf(val0) ? mUdf(int) : mNINT(val0),
	          mIsUdf(val1) ? mUdf(int) : mNINT(val1) );
}


BinID BinIDParam::getDefaultBinIDValue() const
{
    BinIDInpSpec* spec = reinterpret_cast<BinIDInpSpec*>(spec_);
    const float defval0 = spec->defaultValue(0);
    const float defval1 = spec->defaultValue(1);
    return BinID( mIsUdf(defval0) ? mUdf(int) : mNINT(defval0),
		  mIsUdf(defval1) ? mUdf(int) : mNINT(defval1) );
}


BufferString BinIDParam::getDefaultValue() const
{
    BinID bid = getDefaultBinIDValue();
    BufferString res;
    toString( res, bid );
    return res;
}


BoolParam::BoolParam( const char* nm )
    : ValParam(nm,new BoolInpSpec(true,"yes","no",false))
{}

mParamClone( BoolParam );


bool BoolParam::setCompositeValue( const char* str )
{
    if ( !strcasecmp(str,"yes") || !strcasecmp(str,"true") )
	spec_->setValue( true );
    else if ( !strcasecmp(str,"no") || !strcasecmp(str,"false") )
	spec_->setValue( false );
    else
	return false;

    return true;
}


BufferString BoolParam::getDefaultValue() const
{
    const bool yn = getDefaultBoolValue();
    BufferString str = reinterpret_cast<BoolInpSpec*>(spec_)->trueFalseTxt(yn);
    return str;
}


bool BoolParam::isSet() const
{ return reinterpret_cast<BoolInpSpec*>(spec_)->isSet(); }


void BoolParam::setSet( bool yn )
{ reinterpret_cast<BoolInpSpec*>(spec_)->setSet(yn); }


EnumParam::EnumParam( const char* nm )
    : ValParam( nm, new StringListInpSpec )
{}

mParamClone( EnumParam );

void EnumParam::addEnum( const char* ne )
{ reinterpret_cast<StringListInpSpec*>(spec_)->addString(ne); }

    
BufferString EnumParam::getDefaultValue() const
{
    int strindex = getDefaultIntValue();  
    const BufferStringSet& strings = 
	reinterpret_cast<StringListInpSpec*>(spec_)->strings();
    if ( strindex < 0 || strindex >= strings.size() )
	strindex = 0;

    return strings.get(strindex);
}


void EnumParam::addEnums( const char** nes )
{
    int idx=0;
    while ( nes[idx] )
    {
	addEnum( nes[idx] );
	idx++;
    }
}


void EnumParam::fillDefStr( BufferString& res ) const
{
    bool usequotes = false;
    res += getKey();
    res += "=";
    BufferString val;
    if ( !getCompositeValue(val) && !isRequired() )
	val = getDefaultValue();

    const char* ptr = val.buf();
    while ( *ptr )
    {
	if ( isspace(*ptr) )
	    { usequotes = true; break; }
	ptr++;
    }

    if ( usequotes ) res += "\"";
    res += val;
    if ( usequotes ) res += "\"";
}


bool EnumParam::isSet() const
{ return reinterpret_cast<StringListInpSpec*>(spec_)->isSet(); }


void EnumParam::setSet( bool yn )
{ reinterpret_cast<StringListInpSpec*>(spec_)->setSet(yn); }


StringParam::StringParam( const char* key )
    : ValParam( key, new StringInpSpec )
{}

mParamClone( StringParam );


bool StringParam::setCompositeValue( const char* str_ )
{
    BufferString str = str_;
    if ( str.size() )
    {
	if ( !spec_->setText( str, 0 ) )
	    return false;
    }

    return isOK();
}


bool StringParam::getCompositeValue( BufferString& res ) const
{
    const char* txt = spec_->text(0);
    if ( !txt ) return false;
    const int sz = strlen(txt);
    bool quote = false;
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( isspace(txt[idx]) ) { quote = true; break; }
    }

    res = "";

    if ( quote ) res += "\"";
    res += txt;
    if ( quote ) res += "\"";

    return true;
}


SeisStorageRefParam::SeisStorageRefParam( const char* key )
    : StringParam( key )
{}


mParamClone( SeisStorageRefParam );


bool SeisStorageRefParam::isOK() const
{
    const char* val = spec_->text(0);
    const LineKey lk( val );

    const MultiID mid( lk.lineName() );
    PtrMan<IOObj> ioobj = IOM().get( mid );
    return ioobj;
}

}; // namespace Attrib
