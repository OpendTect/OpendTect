/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: attribparam.cc,v 1.12 2005-08-03 07:20:43 cvsnanne Exp $";

#include "attribparam.h"
#include "attribparamgroup.h"

#include "ioman.h"
#include "ioobj.h"
#include "linekey.h"
#include "ptrman.h"
#include "position.h"

namespace Attrib
{

Param::Param( const char* key_ )
    : key( key_ )
    , enabled( true )
    , required( true )
    , isgroup_( false )
{}


Param::Param( const Param& b )
    : key( b.key )
    , enabled( b.enabled )
    , required( b.required )
    , defaultval_( b.defaultval_ )
    , isgroup_( b.isgroup_ )
{}


ValParam::ValParam( const char* key_, DataInpSpec* spec_ )
    : Param( key_ )
    , spec( spec_ )
{}


ValParam::ValParam( const ValParam& b )
    : Param( b.key )
    , spec( b.spec->clone() )
{
    enabled = b.enabled;
    required = b.required;
    defaultval_ = b.defaultval_;
}


ValParam::~ValParam()
{
    delete spec;
}


#define mParamClone( type ) \
type* type::clone() const { return new type(*this); }

mParamClone( ValParam );


bool ValParam::isEqual( const Param& b ) const
{
    mDynamicCastGet(ValParam*,vp,&const_cast<Param&>(b))

    if ( spec->nElems() != vp->spec->nElems() )
	return false;

    for ( int idx=0; idx<spec->nElems(); idx++ )
    {
	BufferString buf1 = spec->text(idx);
	BufferString buf2 = vp->spec->text(idx);
	if ( buf1 != buf2 )
	    return false;
    }

    return true;
}


bool ValParam::isOK() const
{
    if ( !enabled ) return true;
    if ( spec->isUndef() ) return !required;

    for ( int idx=0; idx<spec->nElems(); idx++ )
    {
	if ( !spec->isInsideLimits(idx) )
	    return false;
    }

    return true;
}



int ValParam::nrValues() const
{ return spec->nElems(); }


#define mSetGet(type,getfunc) \
type ValParam::getfunc( int idx ) const \
{ return spec->getfunc(idx); } \
\
void ValParam::setValue( type val, int idx ) \
{ spec->setValue( val, idx ); }

mSetGet(int,getIntValue)
mSetGet(float,getfValue)
mSetGet(bool,getBoolValue)

const char* ValParam::getStringValue( int idx ) const
{ return spec->text(idx); }

void ValParam::setValue( const char* str, int idx )
{ spec->setText( str, idx ); }


bool ValParam::setCompositeValue( const char* nv )
{
    if ( !spec->setText(nv,0) )
	return false;

    return isOK();
};


bool ValParam::getCompositeValue( BufferString& res ) const
{
    if ( !spec ) return false;
    res = spec->text();

    return true;
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
	    spec->setText( valuestr, 0 );

	    strncpy( valuestr, &gatestrvar[idx+1], gatestrsz - idx - 2 );
	    valuestr[gatestrsz - idx - 2] = 0;
	    spec->setText( valuestr, 1 );

	    res = true;
	}
    }

    return res;
}


void ZGateParam::setLimits( const Interval<float>& rg )
{ reinterpret_cast<FloatInpIntervalSpec*>(spec)->setLimits(rg); }


void ZGateParam::setDefaultValue(const Interval<float>& defaultgate)
{
    BufferString defaultstring = "["; 
    defaultstring += defaultgate.start; defaultstring += ",";
    defaultstring += defaultgate.stop; defaultstring += "]";
    Param::setDefaultValue( defaultstring );
}


void ZGateParam::setValue(const Interval<float>& gate)
{
    BufferString string = "["; 
    string += gate.start; string += ",";
    string += gate.stop; string += "]";
    setCompositeValue( string );
}


bool ZGateParam::getCompositeValue( BufferString& res ) const
{
    res = "[";
    res += spec->text(0);
    res += ",";
    res += spec->text(1);
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

    if ( !spec->setText(valuestr,0) )
	return false;

    strncpy( valuestr, &posstr[idx+1], posstrsz - idx - 1 );
    valuestr[posstrsz - idx - 1] = 0;

    if ( !spec->setText(valuestr,1) )
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
    res = spec->text(0);
    res += ",";
    res += spec->text(1);
    return true;
}



BoolParam::BoolParam( const char* nm )
    : ValParam(nm,new BoolInpSpec("yes","no"))
{}

mParamClone( BoolParam );


bool BoolParam::setCompositeValue( const char* str )
{
    if ( !strcasecmp(str,"yes") || !strcasecmp(str,"true") )
	spec->setValue( true );
    else if ( !strcasecmp(str,"no") || !strcasecmp(str,"false") )
	spec->setValue( false );
    else
	return false;

    return true;
}


void BoolParam::setDefaultValue( bool yn )
{
    BufferString str = yn;
    spec->setValue( yn );
    Param::setDefaultValue( str );
}


EnumParam::EnumParam( const char* nm )
    : ValParam( nm, new StringListInpSpec )
{}

mParamClone( EnumParam );

void EnumParam::addEnum( const char* ne )
{ reinterpret_cast<StringListInpSpec*>(spec)->addString(ne); }


void EnumParam::addEnums( const char** nes )
{
    int idx=0;
    while ( nes[idx] )
    {
	addEnum( nes[idx] );
	idx++;
    }
}



StringParam::StringParam( const char* key_ )
    : ValParam( key_, new StringInpSpec )
{}

mParamClone( StringParam );


bool StringParam::setCompositeValue( const char* str_ )
{
    BufferString str = str_;
    if ( str.size() )// && str[0]=='"' && str[str.size()-1]=='"' )
		     // no, this is never verified...
    {
	if ( !spec->setText( str, 0 ) )
	    return false;
    }

    return isOK();
}


bool StringParam::getCompositeValue( BufferString& res ) const
{
    const char* txt = spec->text(0);
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


SeisStorageRefParam::SeisStorageRefParam( const char* key_ )
    : StringParam( key_ )
{}


mParamClone( SeisStorageRefParam );


bool SeisStorageRefParam::isOK() const
{
    const char* val = spec->text(0);
    const LineKey lk( val );

    const MultiID mid( lk.lineName() );
    PtrMan<IOObj> ioobj = IOM().get( mid );
    return ioobj;
}

}; // namespace Attrib
