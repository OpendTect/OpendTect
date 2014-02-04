/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uipropvalfld.h"
#include "uigeninput.h"
#include "uiunitsel.h"
#include "propertyref.h"
#include "unitofmeasure.h"


uiPropertyValFld::uiPropertyValFld( uiParent* p, const PropertyRef& pr,
			float defval, const UnitOfMeasure* defunit )
    : uiGroup(p,BufferString(pr.name()," input"))
    , prevuom_(0)
{
    valfld_ = new uiGenInput( this, pr.name(), FloatInpSpec() );
    unfld_ = new uiUnitSel( this, pr.stdType() );
    if ( defunit )
	unfld_->setUnit( defunit );
    unfld_->selChange.notify( mCB(this,uiPropertyValFld,unChg) );
    unfld_->setName( BufferString(pr.name()," unit") );
    valfld_->setValue( unfld_->getUserValue(defval) );
    unfld_->attach( rightOf, valfld_ );
    setHAlignObj( valfld_ );
    prevuom_ = unfld_->getUnit();
}


void uiPropertyValFld::unChg( CallBacker* )
{
    const UnitOfMeasure* newuom = unfld_->getUnit();
    if ( newuom == prevuom_ )
	return;

    float val = valfld_->getfValue();
    if ( prevuom_ )
	val = prevuom_->internalValue( val );

    prevuom_ = newuom;
    setValue( val, true );
}


void uiPropertyValFld::setValue( float val, bool isinternal )
{
    if ( isinternal )
	val = unfld_->getUserValue( val );
    valfld_->setValue( val );
}


float uiPropertyValFld::getValue( bool internal ) const
{
    const float val = valfld_->getfValue();
    return internal ? unfld_->getInternalValue( val ) : val;
}


const char* uiPropertyValFld::getUnitName() const
{
    return unfld_->getUnitName();
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
