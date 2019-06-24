/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2003
-*/


#include "attribparam.h"
#include "attribparamgroup.h"
#include "uistrings.h"
#include "datapack.h"
#include "ioobj.h"
#include "ptrman.h"
#include "position.h"

namespace Attrib
{

#define mDefParamClone( type ) \
type* type::clone() const { return new type(*this); }
#define mGetSpec(typ) \
    (static_cast<typ*>( spec_ ))

BinIDParam::BinIDParam( const char* nm )
    : ValParam( nm, new PositionInpSpec(BinID(mUdf(int),mUdf(int))) )
{}


BinIDParam::BinIDParam( const char* nm, const BinID& defbid, bool isreq )
    : ValParam( nm, new PositionInpSpec(BinID(mUdf(int),mUdf(int))) )
{
    setDefaultValue( defbid );
    setRequired( isreq );
}

mDefParamClone( BinIDParam );


void BinIDParam::setLimits( const Interval<int>& inlrg,
			    const Interval<int>& crlrg )
{
    /*
    TODO: implement setLimits in BinIDInpSpec
    mGetSpec(BinIDInpSpec)->setLimits( inlrg, crlrg );
    */
}


void BinIDParam::setLimits( int mininl, int maxinl, int mincrl,int maxcrl )
{
    /*
    TODO: implement setLimits in BinIDInpSpec
    mGetSpec(BinIDInpSpec)->setLimits( inlrg, crlrg );
    */
}


bool BinIDParam::setCompositeValue( const char* posstr )
{
    BufferString posbs( posstr );
    char* ptrcrl = posbs.find( ',' );
    if ( !ptrcrl )
	return false;
    *ptrcrl++ = '\0';

    if ( !spec_->setText(posbs.buf(),0) || !spec_->setText(ptrcrl,1) )
	return false;

    return true;
}


void BinIDParam::setDefaultValue( const BinID& bid )
{
    mGetSpec(PositionInpSpec)->setup(true).binid_ = bid;
    mGetSpec(PositionInpSpec)->setup(true).binid_ = bid;
}


bool BinIDParam::getCompositeValue( BufferString& res ) const
{
    BinID bid = getValue();
    toString( res, bid );
    return true;
}


void BinIDParam::toString( BufferString& res, const BinID& bid ) const
{
    res.add( bid.inl() ).add( "," ).add( bid.crl() );
}


BinID BinIDParam::getValue() const
{
    return mGetSpec(PositionInpSpec)->setup(false).binid_;
}


BinID BinIDParam::getDefaultBinIDValue() const
{
    return mGetSpec(PositionInpSpec)->setup(true).binid_;
}


BufferString BinIDParam::getDefaultValue() const
{
    BinID bid = getDefaultBinIDValue();
    BufferString res;
    toString( res, bid );
    return res;
}


static BoolInpSpec* mkBoolInpSpec()
{
    // These strings should NOT be translated! They end up in files!
    const uiString yesstr = toUiString( getYesNoString(true) );
    const uiString nostr = toUiString( getYesNoString(false) );
    return new BoolInpSpec( true, yesstr, nostr, false );
}


BoolParam::BoolParam( const char* nm )
    : ValParam(nm,mkBoolInpSpec())
{}

BoolParam::BoolParam( const char* nm, bool defval, bool isreq )
    : ValParam(nm,mkBoolInpSpec())
{
    setValue( defval );
    setDefaultValue( defval );
    setRequired( isreq );
}

mDefParamClone( BoolParam );


bool BoolParam::setCompositeValue( const char* str )
{
    spec_->setValue( yesNoFromString(str) );
    return true;
}


BufferString BoolParam::getDefaultValue() const
{
    const bool yn = getDefaultBoolValue();
    return BufferString( getYesNoString(yn) );
}


bool BoolParam::isSet() const
{
    return mGetSpec(BoolInpSpec)->isSet();
}


void BoolParam::setSet( bool yn )
{
    mGetSpec(BoolInpSpec)->setSet( yn );
}


EnumParam::EnumParam( const char* nm )
    : ValParam( nm, new StringListInpSpec )
{
}


EnumParam::EnumParam( const char* nm, int defval, bool isreq )
    : ValParam( nm, new StringListInpSpec )
{
    setDefaultValue( defval );
    setRequired( isreq );
}

mDefParamClone( EnumParam );

void EnumParam::addEnum( const char* ne )
{
    mGetSpec(StringListInpSpec)->addString(toUiString(ne));
}


BufferString EnumParam::getDefaultValue() const
{
    int strindex = getDefaultIntValue();
    const uiStringSet& strings = mGetSpec(StringListInpSpec)->strings();
    if ( strindex < 0 || strindex >= strings.size() )
	strindex = 0;

    return toString( strings[strindex] );
}


void EnumParam::setEnums( const EnumDef& defs )
{
    mGetSpec(StringListInpSpec)->setEnumDef( defs );
}

mStartAllowDeprecatedSection
void EnumParam::addEnums( const char** nes )
{
    int idx=0;
    while ( nes[idx] )
    {
	addEnum( nes[idx] );
	idx++;
    }
}
mStopAllowDeprecatedSection


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
	if ( iswspace(*ptr) )
	    { usequotes = true; break; }
	ptr++;
    }

    if ( usequotes ) res += "\"";
    res += val;
    if ( usequotes ) res += "\"";
}


bool EnumParam::isSet() const
{
    return mGetSpec(StringListInpSpec)->isSet();
}


void EnumParam::setSet( bool yn )
{
    mGetSpec(StringListInpSpec)->setSet( yn );
}


StringParam::StringParam( const char* key )
    : ValParam( key, new StringInpSpec )
{}

StringParam::StringParam( const char* key, const char* defstr, bool isreq )
    : ValParam( key, new StringInpSpec )
{
    setDefaultValue( defstr );
    setRequired( isreq );
}

mDefParamClone( StringParam );


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
    res = spec_->text( 0 );
    const char* ptr = res.buf();
    bool havespace = false;
    for ( ; *ptr; ptr++ )
    {
	if ( iswspace(*ptr) )
	    { havespace = true; break; }
    }

    if ( havespace )
	res.quote( '"' );

    return true;
}


SeisStorageRefParam::SeisStorageRefParam( const char* key )
    : StringParam( key )
{}


mDefParamClone( SeisStorageRefParam );


bool SeisStorageRefParam::isOK() const
{
    const StringPair compstr = spec_->text(0);
    const BufferString storstr = compstr.first();
    if ( !storstr.isEmpty() && storstr[0] == '#' )
    {
	DataPack::FullID fid = DataPack::FullID::getFromString(
							storstr.buf()+1 );
	return DPM(fid).isPresent( fid );
    }

    const DBKey storid( storstr );
    PtrMan<IOObj> ioobj = storid.getIOObj();
    return ioobj;
}


}; // namespace Attrib
