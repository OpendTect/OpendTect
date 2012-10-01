/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: uipropvalfld.cc,v 1.2 2012/08/30 14:50:12 cvsbert Exp $";

#include "uipropvalfld.h"
#include "uigeninput.h"
#include "uiunitsel.h"
#include "propertyref.h"
#include "unitofmeasure.h"


uiPropertyValFld::uiPropertyValFld( uiParent* p, const PropertyRef& pr,
			float defval, const UnitOfMeasure* defunit )
    : uiGroup(p,BufferString(pr.name()," input"))
    , curuom_(0)
{
    valfld_ = new uiGenInput( this, pr.name(), FloatInpSpec(defval) );
    unfld_ = new uiUnitSel( this, pr.stdType() );
    unfld_->setUnit( defunit );
    unfld_->selChange.notify( mCB(this,uiPropertyValFld,unChg) );
    unfld_->setName( BufferString(pr.name()," unit") );
    curuom_ = unfld_->getUnit();
    if ( curuom_ && !mIsUdf(defval) )
	valfld_->setValue( curuom_->userValue(defval) );
    unfld_->attach( rightOf, valfld_ );
    setHAlignObj( valfld_ );
}


void uiPropertyValFld::unChg( CallBacker* )
{
    const UnitOfMeasure* newuom = unfld_->getUnit();
    if ( newuom == curuom_ )
	return;
    float val = valfld_->getfValue();
    if ( curuom_ )
	val = curuom_->internalValue( val );

    curuom_ = newuom;
    setValue( val, true );
}


void uiPropertyValFld::setValue( float val, bool isinternal )
{
    if ( isinternal && curuom_ )
	val = curuom_->userValue( val );
    valfld_->setValue( val );
}


float uiPropertyValFld::getValue( bool internal ) const
{
    float val = valfld_->getfValue();
    if ( !mIsUdf(val) && internal && curuom_ )
	val = curuom_->internalValue( val );
    return val;
}


const char* uiPropertyValFld::getUnitName() const
{
    return curuom_->name();
}


void uiPropertyValFld::setUnit( const UnitOfMeasure* uom )
{
    unfld_->setUnit( uom );
}


void uiPropertyValFld::setUnitName( const char* nm )
{
    unfld_->setUnit( nm );
}


void uiPropertyValFld::setReadOnly( bool yn )
{
    valfld_->setReadOnly( yn );
}


const char* uiPropertyValFld::propName() const
{
    return valfld_->titleText();
}
