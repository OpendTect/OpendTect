/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimnemonicprops.h"

#include "draw.h"
#include "separstr.h"
#include "uigeninput.h"
#include "uisellinest.h"
#include "uiunitsel.h"


uiMnemonicProperties::uiMnemonicProperties( uiParent* p )
    : uiGroup(p)
    , valueChanged(this)
{
    scalefld_ = new uiGenInput( this, tr("Scale Type"),
				StringListInpSpec(Mnemonic::ScaleDef()) );
    FloatInpIntervalSpec fspec;
    fspec.setName("Left",0).setName("Right",1);
    rangefld_ = new uiGenInput( this, tr("Display range"), fspec );
    rangefld_->attach( alignedBelow, scalefld_ );

    const Mnemonic* mn = nullptr;
    uomfld_ = new uiUnitSel( this, mn );
    uomfld_->attach( rightTo, rangefld_ );

    linestylefld_ = new uiSelLineStyle( this, OD::LineStyle() );
    linestylefld_->attach( alignedBelow, rangefld_ );

    mAttachCB(rangefld_->valueChanged, uiMnemonicProperties::changedCB);
    mAttachCB(uomfld_->selChange, uiMnemonicProperties::changedCB);
    mAttachCB(linestylefld_->changed, uiMnemonicProperties::changedCB);
}


uiMnemonicProperties::~uiMnemonicProperties()
{
    detachAllNotifiers();
}


void uiMnemonicProperties::setScale( const Mnemonic::Scale scale )
{
    scalefld_->setValue( scale );
}


void uiMnemonicProperties::setRange( const Interval<float>& rg )
{
    rangefld_->setValue( rg );
}


void uiMnemonicProperties::setLineStyle( const OD::LineStyle& ls )
{
    linestylefld_->setStyle( ls );
}


void uiMnemonicProperties::setUOM( const UnitOfMeasure* uom )
{
    uomfld_->setUnit( uom );
}


Mnemonic::Scale uiMnemonicProperties::getScale() const
{
    return Mnemonic::ScaleDef().getEnumForIndex( scalefld_->getIntValue() );
}


Interval<float> uiMnemonicProperties::getRange() const
{
    return rangefld_->getFInterval();
}


OD::LineStyle uiMnemonicProperties::getLineStyle() const
{
    return linestylefld_->getStyle();
}


BufferString uiMnemonicProperties::getUOMStr() const
{
    return uomfld_->getUnitName();
}

void uiMnemonicProperties::changedCB( CallBacker* )
{
    valueChanged.trigger();
}


BufferString uiMnemonicProperties::toString() const
{
    BufferString res;
    FileMultiString fms;
    Interval<float> rg = getRange();
    OD::LineStyle ls = getLineStyle();
    OD::Color lcol = ls.color_;
    fms.add( Mnemonic::toString(getScale()) )
	.add( rg.start ).add( rg.stop )
	.add( getUOMStr() )
	.add( OD::LineStyle::toString(ls.type_) )
	.add( ls.width_ )
	.add( lcol.r() )
	.add( lcol.g() )
	.add( lcol.b() );

    res = fms;
    return res;
}
