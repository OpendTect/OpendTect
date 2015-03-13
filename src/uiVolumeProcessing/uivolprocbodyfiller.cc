/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uivolprocbodyfiller.h"
#include "volprocbodyfiller.h"

#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uivolprocchain.h"

#include "embodytr.h"
#include "separstr.h"
#include "od_helpids.h"


namespace VolProc
{

void uiBodyFiller::initClass()
{
    SeparString str( sFactoryKeyword(), uiStepDialog::factory().cSeparator() );
    str += BodyFiller::sKeyOldType();

    uiStepDialog::factory().addCreator( createInstance, str,
				        sFactoryDisplayName() );
}


uiBodyFiller::uiBodyFiller( uiParent* p, BodyFiller* bf )
    : uiStepDialog( p, BodyFiller::sFactoryDisplayName(), bf )
    , bodyfiller_( bf )
{
    setHelpKey( mODHelpKey(mBodyFillerHelpID) );

    IOObjContext ctxt = mIOObjContext( EMBody );
    ctxt.forread = true;
    uinputselfld_ = new uiIOObjSel( this, ctxt, "Input body" );
    uinputselfld_->selectionDone.notify( mCB(this,uiBodyFiller,bodySel) );
    if ( bf )
	uinputselfld_->setInput( bf->getSurfaceID() );

    BufferStringSet types;
    types.add("Constant Value").add("Previous Step").add("Undefined Value");

    useinsidefld_ = new uiGenInput( this, "Inside Fill Type",
				    StringListInpSpec(types) );
    useinsidefld_->valuechanged.notify( mCB(this,uiBodyFiller,updateFlds) );
    useinsidefld_->attach( alignedBelow, uinputselfld_ );

    const float udfval = mUdf(float);
    insidevaluefld_ = new uiGenInput( this, "Inside Value",
		FloatInpSpec(bf ? bf->getInsideValue() : udfval) );
    insidevaluefld_->attach( alignedBelow, useinsidefld_ );

    useoutsidefld_ = new uiGenInput( this, "Outside Fill Type",
				     StringListInpSpec(types) );
    useoutsidefld_->valuechanged.notify( mCB(this,uiBodyFiller,updateFlds) );
    useoutsidefld_->attach( alignedBelow, insidevaluefld_ );

    outsidevaluefld_ = new uiGenInput( this, "Outside Value",
		FloatInpSpec(bf ? bf->getOutsideValue() : udfval) );
    outsidevaluefld_->attach( alignedBelow, useoutsidefld_ );

    addNameFld( outsidevaluefld_ );

    useinsidefld_->setValue( bf ? (int)bf->getInsideValueType() : 0 );
    updateFlds( useinsidefld_ );
    useoutsidefld_->setValue( bf ? (int)bf->getOutsideValueType() : 0 );
    updateFlds( useoutsidefld_ );
}


uiBodyFiller::~uiBodyFiller()
{
}


void uiBodyFiller::bodySel( CallBacker* )
{
    const IOObj* ioobj = uinputselfld_->ioobj();
    if ( ioobj )
	namefld_->setText( ioobj->name() );
}


void uiBodyFiller::updateFlds( CallBacker* cb )
{
    if ( cb==useinsidefld_ )
	insidevaluefld_->display( useinsidefld_->getIntValue()==0 );
    if ( cb==useoutsidefld_ )
	outsidevaluefld_->display( useoutsidefld_->getIntValue()==0 );
}


uiStepDialog* uiBodyFiller::createInstance( uiParent* parent, Step* ps )
{
    mDynamicCastGet(BodyFiller*,bf,ps);
    if ( !bf ) return 0;

    return new uiBodyFiller( parent, bf );
}


static BodyFiller::ValueType getValueType( uiGenInput& fld )
{ return (BodyFiller::ValueType)fld.getIntValue(); }

bool uiBodyFiller::acceptOK( CallBacker* cb )
{
    if ( !uiStepDialog::acceptOK(cb) )
	return false;

    const IOObj* ioobj = uinputselfld_->ioobj();
    if ( !ioobj )
    {
	uiMSG().error("Invalid empty input body");
	return false;
    }

    BodyFiller::ValueType vt = getValueType( *useinsidefld_ );
    bodyfiller_->setInsideValueType( vt );
    bodyfiller_->setInsideValue(
	vt==BodyFiller::Constant ? insidevaluefld_->getfValue() : mUdf(float) );

    vt = getValueType( *useoutsidefld_ );
    bodyfiller_->setOutsideValueType( vt );
    bodyfiller_->setOutsideValue(
	vt==BodyFiller::Constant ? outsidevaluefld_->getfValue() : mUdf(float));

    bodyfiller_->setSurface( ioobj->key() );

    return true;
}

} // namespace VolProc

