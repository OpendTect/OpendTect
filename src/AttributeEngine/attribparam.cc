/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: attribparam.cc,v 1.1 2005-02-01 14:05:24 kristofer Exp $";

#include "attribparam.h"

#include "datainpspec.h"
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


const char* Param::getKey() const { return (const char*) key; }


const DataInpSpec* Param::getSpec() const { return spec; }


bool Param::setValue( const char* nv )
{
    if ( !spec->setText(nv) )
	return false;


    return true;
};

/*
bool Parser::parseDefString( const char* nm )
{
    BufferString attribname = getAttribName( nm );
    if ( attribname!=basename )
	return false;

    for ( int idx=0; idx<params.size(); idx++ )
    {
	BufferString paramval;
	if ( !getParamString( nm, params[
    }
}





bool Param::getValueString(bool includekey, BufferString& res) const
{
    res = "";
    BufferString value;
    if ( !getValueString( value ) )
	return false;

    if ( includekey ) { res += key; res += "="; }
    
    res += value;
    return true;
}


bool Param::getValueString( BufferString& res) const
{ res = spec->text(); return true; }
    

ZGateParam::ZGateParam( const char* nm )
    : Param( nm, new FloatInpIntervalSpec(true) )
{}

mParamClone( ZGateParam );

bool ZGateParam::setValue( const char* gatestrvar )
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


bool ZGateParam::getValueString( BufferString& res ) const
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

bool BinIDParam::setValue( const char* posstr )
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


bool BinIDParam::getValueString( BufferString& res ) const
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


Parser::Parser( const char* basename_, ParserStatusUpdater updater )
    : basename( basename_ )
    , parserupdater( updater )
{}


Parser* Parser::clone() const
{
    Parser* res = new Parser( basename, parserupdater );

    for ( int idx=0; idx<params.size(); idx++ )
	res->addParam( params[idx]->clone() );

    return res;
}


bool Parser::getParamString( const char* defstr, const char* key,
			     BufferString& res )
{
    if ( !defstr || !key )
    return 0;

    const int inpsz = strlen(defstr);
    const int pattsz = strlen(key);
    bool inquotes = false;
    for ( int idx = 0; idx<inpsz; idx ++)
    {
	if ( !inquotes && defstr[idx] == '=' )
	{
	    int firstpos = idx - 1;

	    while ( isspace(defstr[firstpos]) && firstpos >= 0 ) firstpos --;
	    if ( firstpos < 0 ) continue;

	    int lastpos = firstpos;

	    while ( !isspace(defstr[firstpos]) && firstpos >= 0 ) firstpos --;
	    firstpos++;

	    if ( lastpos - firstpos + 1 == pattsz )
	    {
		if ( !strncmp( &defstr[firstpos], key, pattsz ) )
		{
		    firstpos = idx + 1;
		    while ( isspace(defstr[firstpos]) && firstpos < inpsz )
			firstpos ++;
		    if ( firstpos == inpsz ) continue;

		    bool hasquotes = false;
		    if (defstr[firstpos] == '"')
		    {
			hasquotes = true;
			if (firstpos == inpsz - 1)
			continue;
			firstpos++;
		    }
		    lastpos = firstpos;

		    while (( (hasquotes && defstr[lastpos] != '"')
			    || (!hasquotes && !isspace(defstr[lastpos])) )
			    &&  lastpos < inpsz)
			lastpos ++;

		    lastpos --;
		
		    char tmpres[lastpos-firstpos+2];
		    strncpy( tmpres, &defstr[firstpos], lastpos-firstpos+1 );
		    tmpres[lastpos-firstpos+1] = 0;

		    res = tmpres;
		    return true;
		}
	    }
	}
	else if ( defstr[idx] == '"' )
	    inquotes = !inquotes;
    }

    return false;
}

*/


}; //namespace

