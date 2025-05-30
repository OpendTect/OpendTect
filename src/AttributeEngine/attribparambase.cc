/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attribparambase.h"

#include "datainpspec.h"
#include "multiid.h"

namespace Attrib
{

Param::Param( const char* key )
    : key_( key )
    , isgroup_( false )
    , enabled_( true )
    , required_( true )
{}


Param::Param( const Param& b )
    : key_( b.key_ )
    , isgroup_( b.isgroup_ )
    , enabled_( b.enabled_ )
    , required_( b.required_ )
{}


Param::~Param()
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
mSetGet(float,getFValue)
mSetGet(bool,getBoolValue)
mSetGet(double,getDValue)

#define mSetGetDefault(type,getfunc,dataspecgetfunc) \
type ValParam::getfunc( int idx ) const \
{ return spec_->dataspecgetfunc(idx); } \
\
void ValParam::setDefaultValue( type val, int idx ) \
{ spec_->setDefaultValue( val, idx ); }

mSetGetDefault(int,getDefaultIntValue,getDefaultIntValue)
mSetGetDefault(float,getDefaultfValue,getDefaultfValue)
mSetGetDefault(double,getDefaultdValue,getDefaultValue)
mSetGetDefault(bool,getDefaultBoolValue,getDefaultBoolValue)
mSetGetDefault(const char*,getDefaultStringValue,getDefaultStringValue)


const char* ValParam::getStringValue( int idx ) const
{ return spec_->text(idx); }


MultiID ValParam::getMultiID( int idx ) const
{
    MultiID key;
    return key.fromString( spec_->text(idx) ) ? key : MultiID::udf();
}


void ValParam::setValue( const char* str, int idx )
{ spec_->setText( str, idx ); }

void ValParam::setValue( const MultiID& key, int idx )
{ spec_->setText( key.toString(), idx ); }


bool ValParam::setCompositeValue( const char* nv )
{
    if ( !spec_->setText(nv,0) )
	return false;

    return isOK();
}


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

} // namespace Attrib
