/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigmtcoastline.h"

#include "draw.h"
#include "gmtpar.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uisellinest.h"
#include "uispinbox.h"


int uiGMTCoastlineGrp::factoryid_ = -1;

void uiGMTCoastlineGrp::initClass()
{
    if ( factoryid_ < 0 )
	factoryid_ = uiGMTOF().add( "Coastline",
				    uiGMTCoastlineGrp::createInstance );
}


uiGMTOverlayGrp* uiGMTCoastlineGrp::createInstance( uiParent* p )
{
    return new uiGMTCoastlineGrp( p );
}


uiGMTCoastlineGrp::uiGMTCoastlineGrp( uiParent* p )
    : uiGMTOverlayGrp(p,tr("Coastline"))
{
    uiLabeledSpinBox* lsb = new uiLabeledSpinBox( this,
				tr("UTM zone / CM") );
    utmfld_ = lsb->box();
    utmfld_->setHSzPol( uiObject::Medium );
    utmfld_->setInterval( 1, 60 );
    utmfld_->setPrefix( tr("Zone ") );
    utmfld_->valueChanging.notify( mCB(this,uiGMTCoastlineGrp,utmSel) );
    cmfld_ = new uiSpinBox( this );
    cmfld_->setHSzPol( uiObject::Medium );
    cmfld_->setInterval( 3, 177 );
    cmfld_->setStep( 6, true );
    cmfld_->attach( rightTo, lsb );
    cmfld_->setSuffix( tr(" deg") );
    cmfld_->valueChanging.notify( mCB(this,uiGMTCoastlineGrp,utmSel) );
    ewfld_ = new uiGenInput( this, uiString::emptyString(),
	BoolInpSpec(true,uiStrings::sEast(false),uiStrings::sWest(false)) );
    ewfld_->attach( rightTo, cmfld_ );
    ewfld_->valueChanged.notify( mCB(this,uiGMTCoastlineGrp,utmSel) );

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
	    ODGMT::ResolutionDef(), uiStrings::sResolution() );
    resolutionfld_ = lcb->box();

    lsfld_ = new uiSelLineStyle( this, OD::LineStyle(),uiStrings::sLineStyle());
    lsfld_->attach( alignedBelow, lcb );

    wetcolfld_ = new uiColorInput( this, uiColorInput::Setup(OD::Color::White())
					.lbltxt(tr("Fill wet regions"))
					.withcheck(true) );
    wetcolfld_->attach( alignedBelow, lsfld_ );
    drycolfld_ = new uiColorInput( this, uiColorInput::Setup(OD::Color::White())
					.lbltxt(tr("Fill dry regions"))
					.withcheck(true) );
    drycolfld_->attach( alignedBelow, wetcolfld_ );

    reset();
}


void uiGMTCoastlineGrp::reset()
{
    utmfld_->setValue( 31 );
    cmfld_->setValue( 3 );
    ewfld_->setValue( true );
    resolutionfld_->setCurrentItem( 0 );
    lsfld_->setStyle( OD::LineStyle() );
    wetcolfld_->setDoDraw( false );
    drycolfld_->setDoDraw( false );
    wetcolfld_->setColor( OD::Color(170,255,255) );
    drycolfld_->setColor( OD::Color(170,170,127) );
}


void uiGMTCoastlineGrp::utmSel( CallBacker* cb )
{
    if ( !cb ) return;

    utmfld_->valueChanging.disable();
    cmfld_->valueChanging.disable();
    ewfld_->valueChanged.disable();
    mDynamicCastGet(uiSpinBox*,box,cb)
    if ( box == utmfld_ )
    {
	const int utmzone = utmfld_->getIntValue();
	const int relzone = utmzone - 30;
	const int cm = 6 * relzone - 3;
	cmfld_->setValue( cm > 0 ? cm : -cm );
	ewfld_->setValue( cm > 0 );
    }
    else
    {
	const int cm = cmfld_->getIntValue();
	const bool iseast = ewfld_->getBoolValue();
	const int relcm = iseast ? cm : -cm;
	const int utmzone = 30 + ( relcm + 3 ) / 6;
	utmfld_->setValue( utmzone );
    }

    utmfld_->valueChanging.enable();
    cmfld_->valueChanging.enable();
    ewfld_->valueChanged.enable();
}

#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMTCoastlineGrp::fillPar( IOPar& par ) const
{
    const int utmzone = utmfld_->getIntValue();
    par.set( ODGMT::sKeyUTMZone(), utmzone );
    const char* res = resolutionfld_->text();
    par.set( ODGMT::sKeyResolution(), res );
    const OD::LineStyle ls = lsfld_->getStyle();
    BufferString lsstr; ls.toString( lsstr );
    par.set( ODGMT::sKeyLineStyle(), lsstr );

    const bool wetfill = wetcolfld_->doDraw();
    const bool dryfill = drycolfld_->doDraw();
    par.setYN( ODGMT::sKeyWetFill(), wetfill );
    par.setYN( ODGMT::sKeyDryFill(), dryfill );
    if ( wetfill )
	par.set( ODGMT::sKeyWetFillColor(), wetcolfld_->color() );
    if ( wetcolfld_ )
	par.set( ODGMT::sKeyDryFillColor(), drycolfld_->color() );

    return true;
}


bool uiGMTCoastlineGrp::usePar( const IOPar& par )
{
    int utmzone;
    par.get( ODGMT::sKeyUTMZone(), utmzone );
    utmfld_->setValue( utmzone );
    resolutionfld_->setCurrentItem( par.find(ODGMT::sKeyResolution()).buf() );
    OD::LineStyle ls; BufferString lsstr;
    par.get( ODGMT::sKeyLineStyle(), lsstr );
    ls.fromString( lsstr ); lsfld_->setStyle( ls );

    bool wetfill = false;
    bool dryfill = false;
    par.getYN( ODGMT::sKeyWetFill(), wetfill );
    par.getYN( ODGMT::sKeyDryFill(), dryfill );
    wetcolfld_->setDoDraw( wetfill );
    drycolfld_->setDoDraw( dryfill );
    if ( wetfill )
    {
	OD::Color col; par.get( ODGMT::sKeyWetFillColor(), col );
	wetcolfld_->setColor( col );
    }
    if ( dryfill )
    {
	OD::Color col; par.get( ODGMT::sKeyDryFillColor(), col );
	drycolfld_->setColor( col );
    }

    return true;
}
