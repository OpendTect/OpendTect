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
    nullptr
};

static const char* inpsourceelstrs[] =
{
    "P-wave, S-wave, Density",
    "Acoustic Impedance, Shear Impedance, (Density)",
    nullptr
};


uiElasticModelProvider::uiElasticModelProvider( uiParent* p, bool is2d )
    : uiGroup(p,"ElasticModelProvider setup")
{
    inptypefld_ = new uiGenInput( this, tr("Input type"),
				  BoolInpSpec(true, tr("Acoustic"),
						    tr("Elastic")) );
    mAttachCB( inptypefld_->valueChanged, uiElasticModelProvider::inpTypeSel );

    inpsourceacfld_ = new uiGenInput( this, tr("Input Source"),
				      StringListInpSpec(inpsourceacstrs) );
    mAttachCB( inpsourceacfld_->valueChanged,uiElasticModelProvider::sourceSel);
    inpsourceacfld_->attach( alignedBelow, inptypefld_ );

    inpsourceelfld_ = new uiGenInput( this, tr("Input Source"),
				      StringListInpSpec(inpsourceelstrs) );
    mAttachCB( inpsourceelfld_->valueChanged,
	       uiElasticModelProvider::sourceSel );
    inpsourceelfld_->attach( alignedBelow, inptypefld_ );

    pwavefld_ = new uiVelSel( this, tr("P-wave Velocity cube"), is2d );
    pwavefld_->attach( alignedBelow, inpsourceacfld_ );

    swavefld_ = new uiVelSel( this, tr("S-wave Velocity cube"), is2d );
    swavefld_->attach( alignedBelow, pwavefld_ );

    const Seis::GeomType gt = is2d ? Seis::Line : Seis::Vol;
    const IOObjContext ctxt = uiSeisSel::ioContext( gt, true );

    uiSeisSel::Setup aisu( gt );
    aisu.allowsetdefault( false ).seltxt( tr("Acoustic Impedance") );
    aifld_ = new uiSeisSel( this, ctxt, aisu );
    aifld_->attach( alignedBelow, inpsourceacfld_ );

    uiSeisSel::Setup sisu( gt );
    sisu.allowsetdefault( false ).seltxt( tr("Shear Impedance") );
    sifld_ = new uiSeisSel( this, ctxt, sisu );
    sifld_->attach( alignedBelow, aifld_ );

    uiSeisSel::Setup rhosu( gt );
    rhosu.allowsetdefault( false ).seltxt( tr("Density") );
    densityfld_ = new uiSeisSel( this, ctxt, rhosu );
    densityfld_->attach( alignedBelow, sifld_ );

    rhosu.optional_= true;
    optdensityfld_ = new uiSeisSel( this, ctxt, rhosu );
    optdensityfld_->attach( alignedBelow, sifld_ );

    setHAlignObj( inptypefld_ );
    mAttachCB( postFinalize(), uiElasticModelProvider::initGrpCB );
}


uiElasticModelProvider::~uiElasticModelProvider()
{
    detachAllNotifiers();
}


void uiElasticModelProvider::initGrpCB( CallBacker* )
{
    inpTypeSel( nullptr );
}


void uiElasticModelProvider::inpTypeSel( CallBacker* cb )
{
    const bool isac = inptypefld_->getBoolValue();
    inpsourceacfld_->display( isac );
    inpsourceelfld_->display( !isac );
    sourceSel( nullptr );
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

    sourceSel( nullptr );

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
	mSelf().errmsg_.set( basestr ).append( reasonstr, true );
	return false;
    }

    return true;
}
