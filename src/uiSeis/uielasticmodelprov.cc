/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene
 Date:		February 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uielasticmodelprov.h"

#include "ctxtioobj.h"
#include "uigeninput.h"
#include "uimsg.h"
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
    inptypefld_->valuechanged.notify(
			    mCB(this,uiElasticModelProvider,inpTypeSel) );

    inpsourceacfld_ = new uiGenInput( this, tr("Input Source"),
				      StringListInpSpec(inpsourceacstrs) );
    inpsourceacfld_->valuechanged.notify(
			    mCB(this,uiElasticModelProvider,sourceSel) );
    inpsourceacfld_->attach( alignedBelow, inptypefld_ );

    inpsourceelfld_ = new uiGenInput( this, tr("Input Source"),
				      StringListInpSpec(inpsourceelstrs) );
    inpsourceelfld_->valuechanged.notify(
			    mCB(this,uiElasticModelProvider,sourceSel) );
    inpsourceelfld_->attach( alignedBelow, inptypefld_ );

    IOObjContext pwctxt = uiVelSel::ioContext();
    pwctxt.forread = true;
    uiSeisSel::Setup pwsu( false, false );
    pwsu.seltxt( tr("P-wave Velocity cube") );
    pwavefld_ = new uiVelSel( this, pwctxt, pwsu, false );
    pwavefld_->attach( alignedBelow, inpsourceacfld_ );

    IOObjContext swctxt = uiVelSel::ioContext();
    swctxt.forread = true;
    uiSeisSel::Setup swsu( false, false );
    swsu.seltxt( tr("S-wave Velocity cube") );
    swavefld_ = new uiVelSel( this, swctxt, swsu, false );
    swavefld_->attach( alignedBelow, pwavefld_ );

    IOObjContext aictxt =
		uiSeisSel::ioContext( is2d?Seis::Line:Seis::Vol, true );
    aictxt.forread = true;
    uiSeisSel::Setup aisu( is2d, false );
    aisu.seltxt( tr("Acoustic Impedance") );
    aifld_ = new uiSeisSel( this, aictxt, aisu );
    aifld_->attach( alignedBelow, inpsourceacfld_ );

    IOObjContext sictxt =
		uiSeisSel::ioContext( is2d?Seis::Line:Seis::Vol, true );
    sictxt.forread = true;
    uiSeisSel::Setup sisu( is2d, false );
    sisu.seltxt( tr("Shear Impedance") );
    sifld_ = new uiSeisSel( this, sictxt, sisu );
    sifld_->attach( alignedBelow, aifld_ );

    IOObjContext denctxt =
		uiSeisSel::ioContext( is2d?Seis::Line:Seis::Vol, true );
    denctxt.forread = true;
    densityfld_ = new uiSeisSel( this, denctxt, uiSeisSel::Setup(is2d,false) );
    densityfld_->attach( alignedBelow, sifld_ );

    IOObjContext optdenctxt =
		uiSeisSel::ioContext( is2d?Seis::Line:Seis::Vol, true );
    optdenctxt.forread = true;
    uiSeisSel::Setup su( is2d, false );
    su.optional_= true;
    su.seltxt( tr("Density") );
    optdensityfld_ = new uiSeisSel( this, optdenctxt, su );
    optdensityfld_->attach( alignedBelow, sifld_ );

    inpTypeSel(0);
    sourceSel(0);
}


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


bool uiElasticModelProvider::getInputMIDs( MultiID& pwmid, MultiID& swmid,
					   MultiID& aimid, MultiID& simid,
					   MultiID& denmid ) const
{
    pwmid = pwavefld_->key();
    swmid = swavefld_->key();
    aimid = aifld_->key();
    simid = sifld_->key();

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

