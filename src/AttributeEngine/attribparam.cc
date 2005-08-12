/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: attribparam.cc,v 1.16 2005-08-12 11:12:17 cvsnanne Exp $";

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
    , defaultval_( b.defaultval_ )
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
    defaultval_ = b.defaultval_;
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
    if ( !isRequired() || !getCompositeValue(val) )
	val = getDefaultValue();
    res += val;
}


ZGateParam::ZGateParam( const char* nm )
      : ValParam( nm, new FloatInpIntervalSpec() )
{}


mParamClone( ZGateParam );


bool ZGateParam::setCompositeValue( const char* gatestrvar )
{
    bool res = false;
    const int gatestrsz = strlen( gatestrvar );
    if ( gatestrvar[0] == '[' &&  gatestrvar[gatestrsz-1] == ']' )
    {
	int idx = 0;
	for ( ; idx < gatestrsz; idx ++)
	    if ( gatestrvar[idx] == ',' )
		break;

	if ( idx<gatestrsz )
	{
	    ArrPtrMan<char> valuestr = new char[gatestrsz];
	    strncpy( valuestr, &gatestrvar[1], idx - 1 );
	    valuestr[idx-1] = 0;
	    spec_->setText( valuestr, 0 );

	    strncpy( valuestr, &gatestrvar[idx+1], gatestrsz - idx - 2 );
	    valuestr[gatestrsz - idx - 2] = 0;
	    spec_->setText( valuestr, 1 );

	    res = true;
	}
    }

    return res;
}


void ZGateParam::setLimits( const Interval<float>& rg )
{ reinterpret_cast<FloatInpIntervalSpec*>(spec_)->setLimits(rg); }


void ZGateParam::setDefaultValue( const Interval<float>& defaultgate )
{
    BufferString str = "["; 
    str += defaultgate.start; str += ",";
    str += defaultgate.stop; str += "]";
    Param::setDefaultValue( str );
}


void ZGateParam::setValue( const Interval<float>& gate )
{ reinterpret_cast<FloatInpIntervalSpec*>(spec_)->setValue( gate ); }


bool ZGateParam::getCompositeValue( BufferString& res ) const
{
    res = "[";
    res += spec_->text(0);
    res += ",";
    res += spec_->text(1);
    res += "]";
    return true;
}



BinIDParam::BinIDParam( const char* nm )
    : ValParam( nm, new BinIDCoordInpSpec(false,true,false) )
{}


mParamClone( BinIDParam );


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
    BufferString defaultstring = bid.inl;
    defaultstring += ","; defaultstring += bid.crl;
    Param::setDefaultValue( defaultstring );
}


bool BinIDParam::getCompositeValue( BufferString& res ) const
{
    res = spec_->text(0);
    res += ",";
    res += spec_->text(1);
    return true;
}


BinID BinIDParam::getValue() const
{
    BinIDCoordInpSpec* spec = reinterpret_cast<BinIDCoordInpSpec*>(spec_);
    return BinID( mNINT(spec->value(0)), mNINT(spec->value(1)) );
}



BoolParam::BoolParam( const char* nm )
    : ValParam(nm,new BoolInpSpec("yes","no"))
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


void BoolParam::setDefaultValue( bool yn )
{
    BufferString str =
	reinterpret_cast<BoolInpSpec*>(spec_)->trueFalseTxt( yn );
    Param::setDefaultValue( str );
}


EnumParam::EnumParam( const char* nm )
    : ValParam( nm, new StringListInpSpec )
{}

mParamClone( EnumParam );

void EnumParam::addEnum( const char* ne )
{ reinterpret_cast<StringListInpSpec*>(spec_)->addString(ne); }


void EnumParam::addEnums( const char** nes )
{
    int idx=0;
    while ( nes[idx] )
    {
	addEnum( nes[idx] );
	idx++;
    }
}



StringParam::StringParam( const char* key )
    : ValParam( key, new StringInpSpec )
{}

mParamClone( StringParam );


bool StringParam::setCompositeValue( const char* str_ )
{
    BufferString str = str_;
    if ( str.size() )// && str[0]=='"' && str[str.size()-1]=='"' )
		     // no, this is never verified...
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
