/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/


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


uiStepDialog* uiBodyFiller::createInstance( uiParent* parent, Step* step,
					    bool is2d )
{
    mDynamicCastGet(BodyFiller*,bf,step);
    return bf ? new uiBodyFiller( parent, bf, is2d ) : 0;
}



uiBodyFiller::uiBodyFiller( uiParent* p, BodyFiller* bf, bool is2d )
    : uiStepDialog( p, BodyFiller::sFactoryDisplayName(), bf, is2d )
    , bodyfiller_( bf )
{
    setHelpKey( mODHelpKey(mBodyFillerHelpID) );

    IOObjContext ctxt = mIOObjContext( EMBody );
    ctxt.forread_ = true;
    bodyfld_ = new uiIOObjSel( this, ctxt, tr("Input body") );
    bodyfld_->selectionDone.notify( mCB(this,uiBodyFiller,bodySel) );
    if ( bf )
	bodyfld_->setInput( bf->getSurfaceID() );

    uiStringSet types;
    types += tr("Constant Value");
    types += tr("Previous Step");
    types += tr("Undefined Value");

    insidetypfld_ = new uiGenInput( this, tr("Inside Fill Type"),
				    StringListInpSpec(types) );
    insidetypfld_->valuechanged.notify( mCB(this,uiBodyFiller,typeSel) );
    insidetypfld_->attach( alignedBelow, bodyfld_ );

    const float udfval = mUdf(float);
    insidevaluefld_ = new uiGenInput( this, tr("Inside Value"),
		FloatInpSpec(bf ? bf->getInsideValue() : udfval) );
    insidevaluefld_->attach( alignedBelow, insidetypfld_ );

    outsidetypfld_ = new uiGenInput( this, tr("Outside Fill Type"),
				     StringListInpSpec(types) );
    outsidetypfld_->valuechanged.notify( mCB(this,uiBodyFiller,typeSel) );
    outsidetypfld_->attach( alignedBelow, insidevaluefld_ );

    outsidevaluefld_ = new uiGenInput( this, tr("Outside Value"),
		FloatInpSpec(bf ? bf->getOutsideValue() : udfval) );
    outsidevaluefld_->attach( alignedBelow, outsidetypfld_ );

    addNameFld( outsidevaluefld_ );

    insidetypfld_->setValue( bf ? (int)bf->getInsideValueType() : 0 );
    typeSel( insidetypfld_ );
    outsidetypfld_->setValue( bf ? (int)bf->getOutsideValueType() : 0 );
    typeSel( outsidetypfld_ );
}


void uiBodyFiller::bodySel( CallBacker* )
{
    const IOObj* ioobj = bodyfld_->ioobj();
    if ( ioobj )
	namefld_->setText( ioobj->name() );
}


void uiBodyFiller::typeSel( CallBacker* cb )
{
    if ( cb==insidetypfld_ )
	insidevaluefld_->display( insidetypfld_->getIntValue()==0 );
    if ( cb==outsidetypfld_ )
	outsidevaluefld_->display( outsidetypfld_->getIntValue()==0 );
}


static BodyFiller::ValueType getValueType( uiGenInput& fld )
{ return (BodyFiller::ValueType)fld.getIntValue(); }

bool uiBodyFiller::acceptOK( CallBacker* cb )
{
    if ( !uiStepDialog::acceptOK(cb) )
	return false;

    const IOObj* ioobj = bodyfld_->ioobj();
    if ( !ioobj )
    {
	uiMSG().error( tr("Invalid empty input body") );
	return false;
    }

    BodyFiller::ValueType vt = getValueType( *insidetypfld_ );
    bodyfiller_->setInsideValueType( vt );
    bodyfiller_->setInsideValue(
	vt==BodyFiller::Constant ? insidevaluefld_->getFValue() : mUdf(float) );

    vt = getValueType( *outsidetypfld_ );
    bodyfiller_->setOutsideValueType( vt );
    bodyfiller_->setOutsideValue(
	vt==BodyFiller::Constant ? outsidevaluefld_->getFValue() : mUdf(float));

    bodyfiller_->setSurface( ioobj->key() );

    return true;
}

} // namespace VolProc
