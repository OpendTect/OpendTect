/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Sept 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigmtsymbolpars.cc,v 1.6 2009-07-22 16:01:28 cvsbert Exp $";

#include "uigmtsymbolpars.h"

#include "gmtdef.h"
#include "iopar.h"
#include "pixmap.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uigeninput.h"


uiGMTSymbolPars::uiGMTSymbolPars( uiParent* p )
    : uiGroup(p,"Symbol Parameters")
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "Symbol shape");
    shapefld_ = lcb->box();
    shapefld_->setHSzPol( uiObject::Small );
    fillShapes();

    sizefld_ = new uiGenInput( this, "Size (cm)",
	    		       FloatInpSpec(0.2) );
    sizefld_->setElemSzPol( uiObject::Small );
    sizefld_->attach( rightTo, lcb );

    outcolfld_ = new uiColorInput( this,
	    			   uiColorInput::Setup(Color::Black())
				   .lbltxt("Outline color") );
    outcolfld_->attach( alignedBelow, lcb );

    fillfld_ = new uiCheckBox( this, "Fill Color",
	   		       mCB(this,uiGMTSymbolPars,fillSel) );
    fillfld_->attach( rightTo, outcolfld_ );

    fillcolfld_ = new uiColorInput( this,
	    			    uiColorInput::Setup(Color::White()) );
    fillcolfld_->attach( rightOf, fillfld_ );
    fillSel(0);
}


void uiGMTSymbolPars::reset()
{
    shapefld_->setCurrentItem( 0 );
    sizefld_->setValue( 0.2 );
    outcolfld_->setColor( Color::Black() );
    fillfld_->setChecked( false );
    fillcolfld_->setColor( Color::White() );
}


void uiGMTSymbolPars::fillShapes()
{
    for ( int idx=0; idx<6; idx++ )
    {
	BufferString shapekey = eString( ODGMT::Shape, idx );
	if ( shapekey.isEmpty() ) break;

	shapekey.buf()[0] = tolower( shapekey.buf()[0] );
	shapekey += ".png";
	shapefld_->insertItem( ioPixmap(shapekey), "", idx );
    }
}


void uiGMTSymbolPars::fillSel( CallBacker* )
{
    if ( !fillcolfld_ || !fillfld_ )
	return;

    fillcolfld_->setSensitive( fillfld_->isChecked() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMTSymbolPars::fillPar( IOPar& par ) const
{
    const int shp = shapefld_->currentItem();
    BufferString shapestr = eString( ODGMT::Shape, shp );
    par.set( ODGMT::sKeyShape, shapestr );
    par.set( sKey::Size, sizefld_->getfValue() );
    par.setYN( ODGMT::sKeyFill, fillfld_->isChecked() );
    par.set( sKey::Color, outcolfld_->color() );
    par.set( ODGMT::sKeyFillColor, fillcolfld_->color() );
    return true;
}


bool uiGMTSymbolPars::usePar( const IOPar& par )
{
    const char* shapestr = par.find( ODGMT::sKeyShape );
    if ( shapestr && *shapestr )
    {
	ODGMT::Shape shp = eEnum( ODGMT::Shape, shapestr );
	shapefld_->setCurrentItem( shp );
    }

    float size;
    if ( par.get(sKey::Size,size) )
	sizefld_->setValue( size );

    Color col;
    if ( par.get(sKey::Color,col) )
	outcolfld_->setColor( col );

    bool dofill = false;
    par.getYN( ODGMT::sKeyFill, dofill );
    fillfld_->setChecked( dofill );
    fillfld_->setChecked( dofill );
    if ( dofill && par.get(ODGMT::sKeyFillColor,col) )
	fillcolfld_->setColor( col );

    return true;
}


