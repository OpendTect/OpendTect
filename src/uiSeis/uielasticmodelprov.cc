/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uielasticmodelprov.h"

#include "ctxtioobj.h"
#include "uigeninput.h"
#include "uiseissel.h"
#include "uiveldesc.h"



static const char* inpsourceacstrs[] =
{
    "PWave, Density",
    "Acoustic Impedance, (Density)",
    0
};

static const char* inpsourceelstrs[] =
{
    "P-wave, S-wave, Density",
    "Acoustic Impedance, Shear Impedance, (Density)",
    0
};


uiElasticModelProvider::uiElasticModelProvider( uiParent* p, bool is2d )
    : uiGroup( p, "ElasticModelProvider setup" )
{

    inptypefld_ = new uiGenInput( this, tr("Input type"),
				  BoolInpSpec(true, tr("Acoustic"),
					      tr("Elastic")) );
    inptypefld_->valueChanged.notify(
			    mCB(this,uiElasticModelProvider,inpTypeSel) );

    inpsourceacfld_ = new uiGenInput( this, tr("Input Source"),
				      StringListInpSpec(inpsourceacstrs) );
    inpsourceacfld_->valueChanged.notify(
			    mCB(this,uiElasticModelProvider,sourceSel) );
    inpsourceacfld_->attach( alignedBelow, inptypefld_ );

    inpsourceelfld_ = new uiGenInput( this, tr("Input Source"),
				      StringListInpSpec(inpsourceelstrs) );
    inpsourceelfld_->valueChanged.notify(
			    mCB(this,uiElasticModelProvider,sourceSel) );
    inpsourceelfld_->attach( alignedBelow, inptypefld_ );

    IOObjContext pwctxt = uiVelSel::ioContext();
    pwctxt.forread_ = true;
    uiSeisSel::Setup pwsu( false, false );
    pwsu.seltxt( tr("P-wave Velocity cube") );
    pwavefld_ = new uiVelSel( this, pwctxt, pwsu, true );
    pwavefld_->attach( alignedBelow, inpsourceacfld_ );

    IOObjContext swctxt = uiVelSel::ioContext();
    swctxt.forread_ = true;
    uiSeisSel::Setup swsu( false, false );
    swsu.seltxt( tr("S-wave Velocity cube") );
    swavefld_ = new uiVelSel( this, swctxt, swsu, true );
    swavefld_->attach( alignedBelow, pwavefld_ );

    IOObjContext aictxt =
		uiSeisSel::ioContext( is2d?Seis::Line:Seis::Vol, true );
    aictxt.forread_ = true;
    uiSeisSel::Setup aisu( is2d, false );
    aisu.seltxt( tr("Acoustic Impedance") );
    aifld_ = new uiSeisSel( this, aictxt, aisu );
    aifld_->attach( alignedBelow, inpsourceacfld_ );

    IOObjContext sictxt =
		uiSeisSel::ioContext( is2d?Seis::Line:Seis::Vol, true );
    sictxt.forread_ = true;
    uiSeisSel::Setup sisu( is2d, false );
    sisu.seltxt( tr("Shear Impedance") );
    sifld_ = new uiSeisSel( this, sictxt, sisu );
    sifld_->attach( alignedBelow, aifld_ );

    IOObjContext denctxt =
		uiSeisSel::ioContext( is2d?Seis::Line:Seis::Vol, true );
    denctxt.forread_ = true;
    uiSeisSel::Setup su1( is2d, false );
    su1.seltxt( tr("Density") );
    densityfld_ = new uiSeisSel( this, denctxt, su1 );
    densityfld_->attach( alignedBelow, sifld_ );

    IOObjContext optdenctxt =
		uiSeisSel::ioContext( is2d?Seis::Line:Seis::Vol, true );
    optdenctxt.forread_ = true;
    uiSeisSel::Setup su2( is2d, false );
    su2.optional_= true;
    su2.seltxt( tr("Density") );
    optdensityfld_ = new uiSeisSel( this, optdenctxt, su2 );
    optdensityfld_->attach( alignedBelow, sifld_ );

    inpTypeSel(0);
    sourceSel(0);
    setHAlignObj( inptypefld_ );
}


uiElasticModelProvider::~uiElasticModelProvider()
{}


void uiElasticModelProvider::inpTypeSel( CallBacker* cb )
{
    const bool isac = inptypefld_->getBoolValue();
    inpsourceacfld_->display( isac );
    inpsourceelfld_->display( !isac );
    sourceSel(0);
}


void uiElasticModelProvider::sourceSel( CallBacker* cb )
{
    const bool isac = inptypefld_->getBoolValue();
    const bool needai = isac ? inpsourceacfld_->getIntValue() == 1
			     : inpsourceelfld_->getIntValue() == 1;

    optdensityfld_->display( needai );
    densityfld_->display( !needai );
    aifld_->display( needai );
    sifld_->display( needai && !isac );
    pwavefld_->display( !needai );
    swavefld_->display( !needai && !isac );
}


void uiElasticModelProvider::setInputMIDs(
	const MultiID& pwmid, const MultiID& swmid, const MultiID& aimid,
	const MultiID& simid, const MultiID& denmid )
{
    const bool iselastic = !simid.isUdf() || !swmid.isUdf();
    const int sourceoptidx = aimid.isUdf() ? 0 : 1;
    inptypefld_->setValue( !iselastic );
    inpsourceacfld_->display( !iselastic );
    inpsourceelfld_->display( iselastic );
    if ( iselastic )
	inpsourceelfld_->setValue( sourceoptidx );
    else
	inpsourceacfld_->setValue( sourceoptidx );

    sourceSel( 0 );

    pwavefld_->setInput( pwmid );
    swavefld_->setInput( swmid );
    aifld_->setInput( aimid );
    sifld_->setInput( simid );
    densityfld_->setInput( denmid );
    optdensityfld_->setChecked( !denmid.isUdf() );
}


bool uiElasticModelProvider::getInputMIDs( MultiID& pwmid, MultiID& swmid,
					   MultiID& aimid, MultiID& simid,
					   MultiID& denmid ) const
{
    pwmid = pwavefld_->attachObj()->isDisplayed() ? pwavefld_->key()
						  : MultiID::udf();
    swmid = swavefld_->attachObj()->isDisplayed() ? swavefld_->key()
						 : MultiID::udf();
    aimid = aifld_->attachObj()->isDisplayed() ? aifld_->key()
					       : MultiID::udf();
    simid = sifld_->attachObj()->isDisplayed() ? sifld_->key()
					       : MultiID::udf();

    const bool isac = inptypefld_->getBoolValue();
    const bool needsi = !isac && inpsourceelfld_->getIntValue() == 1;
    const bool needai = isac ? inpsourceacfld_->getIntValue() == 1
			     : inpsourceelfld_->getIntValue() == 1;

    denmid = needai ? optdensityfld_->isChecked() ? optdensityfld_->key()
						  : MultiID::udf()
		    : densityfld_->key();

    uiString basestr  = tr( "Selected inputs are not in adequation with \n"
			    "chosen model type and source." );
    uiString reasonstr;
    if ( needai && aimid.isUdf() )
	reasonstr.append( tr( "Acoustic Impedance input is missing"), true );

    if ( needsi && simid.isUdf() )
	reasonstr.append( tr("Shear Impedance input is missing" ), true );

    if ( !needai && denmid.isUdf() )
	reasonstr.append( tr("Density input is missing" ), true );

    if ( !needai && pwmid.isUdf() )
	reasonstr.append( tr("P-Wave input is missing" ), true );

    if ( !isac && !needsi && swmid.isUdf() )
	reasonstr.append( tr("S-Wave input is missing" ), true );

    if ( !reasonstr.isEmpty() )
    {
	const_cast<uiElasticModelProvider*>(this)->errmsg_ = basestr;
	const_cast<uiElasticModelProvider*>(this)->
					    errmsg_.append( reasonstr, true );
	return false;
    }

    return true;
}
