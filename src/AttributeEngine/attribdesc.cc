/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attribdesc.cc,v 1.4 2005-02-03 15:35:02 kristofer Exp $";

#include "attribdesc.h"

#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "errh.h"

namespace Attrib
{

bool InputSpec::operator==(const InputSpec& b) const
{
    if ( desc!=b.desc || enabled!=b.enabled || issteering!=b.issteering )
	return false;

    for ( int idx=0; idx<forbiddenDts.size(); idx++ )
    {
	if ( b.forbiddenDts.indexOf(forbiddenDts[idx])==-1 )
	    return false;
    }

    for ( int idx=0; idx<b.forbiddenDts.size(); idx++ )
    {
	if ( forbiddenDts.indexOf(b.forbiddenDts[idx])==-1 )
	    return false;
    }

    return true;
}


Desc::Desc( const char* attribname_, DescStatusUpdater updater, DescChecker dc )
    : ds( 0 )
    , attribname( attribname_ )
    , statusupdater( updater )
    , descchecker( dc )
    , issteering( false )
{
    mRefCountConstructor;
    inputs.allowNull(true);
}


Desc::Desc( const Desc& a )
    : ds( a.ds )
    , attribname( a.attribname )
    , statusupdater( a.statusupdater )
    , descchecker( a.descchecker )
    , issteering( false )
{
    mRefCountConstructor;
    inputs.allowNull(true);

    for ( int idx=0; idx<a.params.size(); idx++ )
	addParam( a.params[idx]->clone() );
}


Desc::~Desc()
{
    deepErase( params );
    deepUnRef( inputs );
}


const char* Desc::attribName() const { return attribname; }


Desc* Desc::clone() const { return new Desc( *this ); }


void Desc::setDescSet( DescSet* nds )
{ ds = nds; }


DescSet* Desc::descSet() const { return ds; }


int Desc::id() const { return ds ? ds->getID(*this) : -1; }


bool Desc::getDefStr( BufferString& res ) const
{
    res = attribName();
    for ( int idx=0; idx<params.size(); idx++ )
    {
	res += " ";
	res += params[idx]->getKey();
	res += "=";
	BufferString val;
	params[idx]->getCompositeValue(val);
	res += val;
    }

    if ( seloutput!=-1 )
    {
	res += " output=";
	res += seloutput;
    }

    return true;
}


bool Desc::parseDefStr( const char* defstr )
{
    BufferString defstrnm;
    if ( !getAttribName(defstr,defstrnm) || defstrnm!=attribname )
	return false;

     for ( int idx=0; idx<params.size(); idx++ )
     {
	 BufferString paramval;
	 if ( !getParamString( defstr, params[idx]->getKey(), paramval ) )
	     return false;

	 if ( !params[idx]->setCompositeValue(paramval) )
	     return false;
     }

     return true;
}


const char* Desc::userRef() const { return userref; }


void Desc::setUserRef(const char* nur ) { userref = nur; }


int Desc::nrOutputs() const { return outputtypes.size(); }


void Desc::selectOutput(int outp) { seloutput = outp; }


int Desc::selectedOutput() const { return seloutput; }


Seis::DataType Desc::dataType() const
{
    if ( seloutput==-1 ) return Seis::UnknowData;
    if ( outputtypelinks[seloutput]!=-1 )
	return inputs[outputtypelinks[seloutput]]
	    ? inputs[outputtypelinks[seloutput]]->dataType() : Seis::UnknowData;

    return outputtypes[seloutput];
}


int Desc::nrInputs() const { return inputs.size(); }


bool Desc::setInput( int input, Desc* nd )
{
    if ( nd && (inputspecs[input].forbiddenDts.indexOf(nd->dataType())!=-1 ||
		inputspecs[input].issteering!=nd->isSteering()) )
	return false;

    if ( inputs[input] ) inputs[input]->unRef();
    inputs.replace( nd, input );
    if ( inputs[input] ) inputs[input]->ref();

    return true;
}


Desc* Desc::getInput( int input ) { return inputs[input]; }


int Desc::isSatisfied() const
{
    if ( seloutput==-1 ) return 2;

    for ( int idx=0; idx<params.size(); idx++ )
    {
	if ( !params[idx]->isOK() )
	    return 2;
    }

    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( !inputspecs[idx].enabled ) continue;
	if ( !inputs[idx] ) return 2;
    }

    return 0;
}


bool Desc::isIdenticalTo( const Desc& b, bool cmpoutput ) const
{
    if ( this==&b ) return true;

    if ( params.size()!=b.params.size() || inputs.size()!=b.inputs.size() )
	return false;

    for ( int idx=0; idx<params.size(); idx++ )
    {
	if ( *params[idx]!=*b.params[idx] )
	    return false;
    }

    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( inputs[idx]==b.inputs[idx] ) continue;
	if ( !inputs[idx]->isIdenticalTo(*b.inputs[idx],cmpoutput) )
	    return false;
    }

    return cmpoutput ? seloutput==b.seloutput : true;
}


void Desc::addParam( Param* param ) { params += param; }


const Param* Desc::getParam( const char* key ) const
{ return const_cast<Desc*>(this)->getParam(key); }


const Param* Desc::getParam( const char* key )
{
    return findParam(key);
}


void Desc::setParamEnabled( const char* key, bool yn )
{
    Param* param = findParam( key );
    if ( !param ) return;

    param->setEnabled(yn);
}


bool Desc::isParamEnabled( const char* key ) const
{
    const Param* param = getParam( key );
    if ( !param ) return false;

    return param->isEnabled();
}
	


void Desc::setParamRequired( const char* key, bool yn )
{
    Param* param = findParam( key );
    if ( !param ) return;

    param->setRequired(yn);
}


bool Desc::isParamRequired( const char* key ) const
{
    const Param* param = getParam( key );
    if ( !param ) return false;

    return param->isRequired();
}


bool Desc::setParamVal( const char* key, const char* val )
{
    Param* param = findParam(key);
    if ( !param ) return false;

    if ( !param->setCompositeValue( val ) )
	return false;

    if ( statusupdater ) statusupdater(*this);
    return true;
}

/*
bool Desc::getParamVal( const char* key, BufferString& val ) const
{
    const Param* param = getParam(key);
    if ( !param ) return false;

    return param->getCompositeValue(val);
}
*/


/*
const DataInpSpec* Desc::getParamSpec(const char* key)
{
    const Param* param = getParam(key);
    if ( !param ) return 0;
    return param->getSpec();
}
*/


void Desc::addInput( const InputSpec& is )
{
    inputspecs += is;
    inputs += 0;
}


InputSpec& Desc::inputSpec( int input ) { return inputspecs[input]; }


const InputSpec&  Desc::inputSpec( int input ) const
{ return const_cast<Desc*>(this)->inputSpec(input); }


void Desc::removeOutputs()
{
    outputtypes.erase();
    outputtypelinks.erase();
}


void Desc::addOutputDataType( Seis::DataType dt )
{ outputtypes+=dt; outputtypelinks+=-1; }
	

void Desc::addOutputDataTypeSameAs( int input )
{ outputtypes+= Seis::UnknowData; outputtypelinks+=input; }
	

bool Desc::getAttribName( const char* defstr_, BufferString& res )
{
    char defstr[strlen(defstr_)+1];
    strcpy(defstr, defstr_ );

    int start = 0;
    while ( start<strlen(defstr) && isspace(defstr[start]) ) start++;

    if ( start>=strlen(defstr) ) return false;

    int stop = start+1;
    while ( stop<strlen(defstr) && !isspace(defstr[stop]) ) stop++;
    defstr[stop] = 0;

    res = &defstr[start];
    return true;
}


bool Desc::getParamString( const char* defstr, const char* key,
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


Param* Desc::findParam( const char* key )
{
    for ( int idx=0; idx<params.size(); idx++ )
    {
	if ( !strcmp(params[idx]->getKey(), key ) )
	    return params[idx];
    }

    return 0;
}




}; //namespace
