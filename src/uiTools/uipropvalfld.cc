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
				    float defval )
    : uiGroup(p,BufferString(pr.name()," input"))
    , pruom_(pr.unit())
    , lastsetvalue_(defval)
    , valueChanged(this)
{
    valfld_ = new uiGenInput( this, mToUiStringTodo(pr.name()),
			      FloatInpSpec(defval) );
    unfld_ = new uiUnitSel( this, pr.stdType() );
    unfld_->setName( BufferString(pr.name()," unit") );
    unfld_->setUnit( pruom_ );
    unfld_->attach( rightOf, valfld_ );
    setHAlignObj( valfld_ );

    prevuom_ = unfld_->getUnit();

    mAttachCB( valfld_->valuechanged, uiPropertyValFld::valChg );
    mAttachCB( unfld_->selChange, uiPropertyValFld::unChg );
}


uiPropertyValFld::~uiPropertyValFld()
{
    detachAllNotifiers();
}


void uiPropertyValFld::valChg( CallBacker* )
{
    handleValChg( getValue(), true );
}


void uiPropertyValFld::unChg( CallBacker* )
{
    const UnitOfMeasure* newuom = unfld_->getUnit();
    if ( newuom == prevuom_ )
	return;

    const float val = getConvertedValue( valfld_->getFValue(),
					 prevuom_, newuom );
    prevuom_ = newuom;
    setValue( val );
}


void uiPropertyValFld::handleValChg( float newvalue, bool emitnotif ) const
{
    const float diff = lastsetvalue_ - newvalue;
    lastsetvalue_ = newvalue;
    if ( emitnotif && !mIsZero(diff,1e-8) )
	const_cast<uiPropertyValFld*>(this)->valueChanged.trigger();
}


void uiPropertyValFld::setValue( float val )
{
    const UnitOfMeasure* curuom = unfld_->getUnit();
    const float newvalue = curuom == pruom_
			 ? val : getConvertedValue( val, pruom_, curuom );
    NotifyStopper ns( valfld_->valuechanged );
    valfld_->setValue( val );
    handleValChg( newvalue, false );
}


float uiPropertyValFld::getValue() const
{
    const UnitOfMeasure* curuom = unfld_->getUnit();
    float val = valfld_->getFValue();
    const float newvalue = curuom == pruom_
			 ? val : getConvertedValue( val, curuom, pruom_ );
    return newvalue;
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
