/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: attribparam.cc,v 1.2 2005-02-03 15:35:02 kristofer Exp $";

#include "attribparam.h"

#include "datainpspec.h"
#include "ioman.h"
#include "ioobj.h"
#include "linekey.h"
#include "ptrman.h"

namespace Attrib
{

Param::Param( const char* key_, DataInpSpec* spec_ )
    : key( key_ )
    , spec( spec_ )
    , enabled( true )
    , required( true )
{}


Param::Param( const Param& b )
    : key( b.key )
    , spec( b.spec->clone() )
    , enabled( b.enabled )
    , required( b.required )
{}


#define mParamClone( type ) \
type* type::clone() const { return new type(*this); }

mParamClone( Param );


bool Param::operator==( const Param& b ) const
{
    if ( key!=b.key ) return false;

    if ( spec->nElems()!=b.spec->nElems() )
	return false;

    for ( int idx=0; idx<spec->nElems(); idx++ )
    {
	if ( strcmp(spec->text(idx),b.spec->text(idx)) )
	    return false;
    }

    return true;
}

   
bool Param::operator!=( const Param& b ) const { return !(*this==b); }


bool Param::isOK() const
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


bool Param::isEnabled() const { return enabled; }


void Param::setEnabled(bool yn) { enabled=yn; }


bool Param::isRequired() const { return required; }


void Param::setRequired(bool yn) { required=yn; }



const char* Param::getKey() const { return (const char*) key; }


int Param::nrValues() const { return spec->nElems(); }


int Param::getIntValue( int idx ) const { return spec->getIntValue(idx); }
float Param::getfValue( int idx ) const { return spec->getfValue(idx); }
bool Param::getBoolValue( int idx ) const { return spec->getBoolValue(idx); }
const char* Param::getStringValue( int idx ) const { return spec->text(idx); }


bool Param::setCompositeValue( const char* nv )
{
    if ( !spec->setText(nv,0) )
	return false;

    return isOK();
};


bool Param::getCompositeValue(BufferString& buf) const
{
    if ( !spec ) return false;
    buf = spec->text();

    return true;
}

ZGateParam::ZGateParam( const char* nm )
    : Param( nm, new FloatInpIntervalSpec(true) )
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
{ reinterpret_cast<FloatInpSpec*>(spec)->setLimits(rg); }


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
    : Param( nm, new BinIDCoordInpSpec(false,true,false) )
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


bool BinIDParam::getCompositeValue( BufferString& res ) const
{
    res = spec->text(0);
    res += ",";
    res += spec->text(1);
    return true;
}


EnumParam::EnumParam( const char* nm )
    : Param( nm, new StringListInpSpec )
{}

mParamClone( EnumParam );

void EnumParam::addEnum( const char* ne )
{ reinterpret_cast<StringListInpSpec*>(spec)->addString(ne); }


StringParam::StringParam( const char* key_ )
    : Param( key_, new StringInpSpec )
{}


mParamClone( StringParam );


bool StringParam::setCompositeValue( const char* str_ )
{
    BufferString str = str_;
    if ( str.size() && str[0]=='"' && str[str.size()-1]=='"' )
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

}; //namespace

