/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    uiUnitSel::Setup ussu( pr.stdType() );
    ussu.mode( uiUnitSel::Setup::SymbolsOnly );
    unfld_ = new uiUnitSel( this, ussu );
    unfld_->setName( BufferString(pr.name()," unit") );
    unfld_->setUnit( pruom_ );
    unfld_->attach( rightOf, valfld_ );
    setHAlignObj( valfld_ );

    mAttachCB( valfld_->valueChanged, uiPropertyValFld::valChg );
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


void uiPropertyValFld::unChg( CallBacker* cb )
{
    if ( !cb )
	return;

    mCBCapsuleUnpack(const UnitOfMeasure*,prevuom,cb);
    const UnitOfMeasure* newuom = unfld_->getUnit();
    if ( newuom == prevuom )
	return;

    const float val = getConvertedValue( valfld_->getFValue(),
					 prevuom, newuom );
    prevuom = newuom;
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
    NotifyStopper ns( valfld_->valueChanged );
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
    setUnit( UoMR().get( nm ) );
}


void uiPropertyValFld::setReadOnly( bool yn )
{
    valfld_->setReadOnly( yn );
}


const char* uiPropertyValFld::propName() const
{
    return valfld_->titleText().getFullString();
}
