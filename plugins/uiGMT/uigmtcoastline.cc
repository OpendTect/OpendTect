/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: uigmtcoastline.cc,v 1.2 2008-08-14 10:52:52 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtcoastline.h"

#include "draw.h"
#include "gmtpar.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uicombobox.h"
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
    : uiGMTOverlayGrp(p,"Coastline")
{
    uiLabeledSpinBox* lsb = new uiLabeledSpinBox( this, "Select UTM zone" );
    utmfld_ = lsb->box();
    utmfld_->setInterval( 1, 60 );

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "Resolution" );
    resolutionfld_ = lcb->box();
    lcb->attach( alignedBelow, lsb );
    for ( int idx=0; idx<5; idx++ )
	resolutionfld_->insertItem( eString(ODGMT::Resolution,idx) );

    lsfld_ = new uiSelLineStyle( this, LineStyle(), "Line Style" );
    lsfld_->attach( alignedBelow, lcb );

    fillwetfld_ = new uiCheckBox( this, "Fill wet regions",
	    			  mCB(this,uiGMTCoastlineGrp,fillSel) );
    fillwetfld_->attach( alignedBelow, lsfld_ );

    filldryfld_ = new uiCheckBox( this, "Fill dry regions",
	    			  mCB(this,uiGMTCoastlineGrp,fillSel) );
    filldryfld_->attach( alignedBelow, fillwetfld_ );

    wetcolfld_ = new uiColorInput( this,
	    			   uiColorInput::Setup(Color::White) );
    wetcolfld_->attach( rightOf, fillwetfld_ );

    drycolfld_ = new uiColorInput( this,
	    			   uiColorInput::Setup(Color::DgbColor) );
    drycolfld_->attach( rightOf, filldryfld_ );
    fillSel(0);
}


void uiGMTCoastlineGrp::fillSel( CallBacker* )
{
    wetcolfld_->setSensitive( fillwetfld_->isChecked() );
    drycolfld_->setSensitive( filldryfld_->isChecked() );
}

#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMTCoastlineGrp::fillPar( IOPar& par ) const
{
    const int utmzone = utmfld_->getValue();
    par.set( ODGMT::sKeyUTMZone, utmzone );
    const char* res = resolutionfld_->text();
    par.set( ODGMT::sKeyResolution, res );
    const LineStyle ls = lsfld_->getStyle();
    const bool drawline = ls.type_ != LineStyle::None;
    BufferString lsstr; ls.toString( lsstr );
    par.set( ODGMT::sKeyLineStyle, lsstr );

    const bool wetfill = fillwetfld_->isChecked();
    const bool dryfill = filldryfld_->isChecked();
    par.setYN( ODGMT::sKeyWetFill, wetfill );
    par.setYN( ODGMT::sKeyDryFill, dryfill );
    if ( wetfill )
	par.set( ODGMT::sKeyWetFillColor, wetcolfld_->color() );
    if ( wetcolfld_ )
	par.set( ODGMT::sKeyDryFillColor, drycolfld_->color() );

    return true;
}


bool uiGMTCoastlineGrp::usePar( const IOPar& par )
{
    int utmzone;
    par.get( ODGMT::sKeyUTMZone, utmzone );
    utmfld_->setValue( utmzone );
    resolutionfld_->setCurrentItem( par.find(ODGMT::sKeyResolution) );
    LineStyle ls; BufferString lsstr;
    par.get( ODGMT::sKeyLineStyle, lsstr );
    ls.fromString( lsstr ); lsfld_->setStyle( ls );

    bool wetfill = false;
    bool dryfill = false;
    par.getYN( ODGMT::sKeyWetFill, wetfill );
    par.getYN( ODGMT::sKeyDryFill, dryfill );
    fillwetfld_->setChecked( wetfill );
    filldryfld_->setChecked( dryfill );
    if ( wetfill )
    {
	Color col;
	par.get( ODGMT::sKeyWetFillColor, col );
	wetcolfld_->setColor( col );
    }
    if ( wetcolfld_ )
    {
	Color col;
	par.get( ODGMT::sKeyDryFillColor, col );
	drycolfld_->setColor( col );
    }

    return true;
}

