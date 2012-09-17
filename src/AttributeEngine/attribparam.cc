/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: attribparam.cc,v 1.32 2011/01/20 12:56:05 cvshelene Exp $";

#include "attribparam.h"
#include "attribparamgroup.h"

#include "datapack.h"
#include "ioman.h"
#include "ioobj.h"
#include "linekey.h"
#include "ptrman.h"
#include "position.h"

namespace Attrib
{

#define mParamClone( type ) \
type* type::clone() const { return new type(*this); }

BinIDParam::BinIDParam( const char* nm )
    : ValParam( nm, new PositionInpSpec(BinID(mUdf(int),mUdf(int))) )
{}


BinIDParam::BinIDParam( const char* nm, const BinID& defbid, bool isreq )
    : ValParam( nm, new PositionInpSpec(BinID(mUdf(int),mUdf(int))) )
{
    setDefaultValue( defbid );
    setRequired( isreq );
}

mParamClone( BinIDParam );


void BinIDParam::setLimits( const Interval<int>& inlrg,
			    const Interval<int>& crlrg )
{
    /*
    TODO: implement setLimits in BinIDInpSpec
    reinterpret_cast<BinIDInpSpec*>(spec_)->setLimits( inlrg, crlrg );
    */
}


void BinIDParam::setLimits( int mininl, int maxinl, int mincrl,int maxcrl )
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
    reinterpret_cast<PositionInpSpec*>(spec_)->setup(true).binid_ = bid;
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
    const PositionInpSpec& spec = *reinterpret_cast<PositionInpSpec*>(spec_);
    return spec.setup(false).binid_;
}


BinID BinIDParam::getDefaultBinIDValue() const
{
    const PositionInpSpec& spec = *reinterpret_cast<PositionInpSpec*>(spec_);
    return spec.setup(true).binid_;
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

BoolParam::BoolParam( const char* nm, bool defval, bool isreq )
    : ValParam(nm,new BoolInpSpec(true,"yes","no",false))
{
    setValue( defval );
    setDefaultValue( defval );
    setRequired( isreq );
}

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

EnumParam::EnumParam( const char* nm, int defval, bool isreq )
    : ValParam( nm, new StringListInpSpec )
{
    setDefaultValue( defval );
    setRequired( isreq );
}

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

StringParam::StringParam( const char* key, const char* defstr, bool isreq )
    : ValParam( key, new StringInpSpec )
{
    setDefaultValue( defstr );
    setRequired( isreq );
}

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

    BufferString bstring = lk.lineName();
    const char* linenm = bstring.buf();
    if ( linenm && *linenm == '#' )
    {
	DataPack::FullID fid( linenm+1 );
	return DPM(fid).haveID( fid );
    }

    const MultiID mid( bstring );
    PtrMan<IOObj> ioobj = IOM().get( mid );
    return ioobj;
}

}; // namespace Attrib
