/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2012
________________________________________________________________________

-*/

#include "uipropvalfld.h"
#include "uigeninput.h"
#include "uiunitsel.h"
#include "propertyref.h"
#include "unitofmeasure.h"


uiPropertyValFld::uiPropertyValFld( uiParent* p, const PropertyRef& pr,
			float defval, const UnitOfMeasure* defunit )
    : uiGroup(p,BufferString(pr.name()," input"))
    , prevuom_(0)
    , lastsetvalue_(defval)
    , valueChanged(this)
{
    valfld_ = new uiGenInput(this, mToUiStringTodo(pr.name()), FloatInpSpec());
    unfld_ = new uiUnitSel( this, pr.stdType() );
    if ( defunit )
	unfld_->setUnit( defunit );
    unfld_->setName( BufferString(pr.name()," unit") );
    valfld_->setValue( unfld_->getUserValue(defval) );
    unfld_->attach( rightOf, valfld_ );
    setHAlignObj( valfld_ );
    prevuom_ = unfld_->getUnit();

    valfld_->valuechanged.notify( mCB(this,uiPropertyValFld,valChg) );
    unfld_->selChange.notify( mCB(this,uiPropertyValFld,unChg) );
}


void uiPropertyValFld::valChg( CallBacker* )
{
    const float newvalue = getValue( true );
    handleValChg( newvalue, true );
}


void uiPropertyValFld::unChg( CallBacker* )
{
    const UnitOfMeasure* newuom = unfld_->getUnit();
    if ( newuom == prevuom_ )
	return;

    float val = valfld_->getFValue();
    if ( prevuom_ )
	val = prevuom_->internalValue( val );

    prevuom_ = newuom;
    setValue( val, true );
}


void uiPropertyValFld::handleValChg( float newvalue, bool emitnotif ) const
{
    const float diff = lastsetvalue_ - newvalue;
    lastsetvalue_ = newvalue;
    if ( emitnotif && !mIsZero(diff,1e-8) )
	const_cast<uiPropertyValFld*>(this)->valueChanged.trigger();
}


void uiPropertyValFld::setValue( float val, bool isinternal )
{
    const float newvalue = isinternal ? val : unfld_->getInternalValue( val );
    if ( isinternal )
	val = unfld_->getUserValue( val );
    NotifyStopper ns( valfld_->valuechanged );
    valfld_->setValue( val );
    handleValChg( newvalue, false );
}


float uiPropertyValFld::getValue( bool internal ) const
{
    float val = valfld_->getFValue();
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
    return valfld_->titleText().getFullString();
}
