/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : H. Huck
 * DATE     : Jan 2008
-*/

static const char* rcsID = "$Id: attribparambase.cc,v 1.2 2009/07/22 16:01:30 cvsbert Exp $";

#include "attribparambase.h"

#include "datainpspec.h"

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

}; // namespace Attrib
