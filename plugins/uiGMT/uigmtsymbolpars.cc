/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Sept 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigmtsymbolpars.cc,v 1.8 2010-12-07 22:59:52 cvskris Exp $";

#include "uigmtsymbolpars.h"

#include "gmtdef.h"
#include "iopar.h"
#include "pixmap.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uigeninput.h"


uiGMTSymbolPars::uiGMTSymbolPars( uiParent* p, bool usewellsymbols )
    : uiGroup(p,"Symbol Parameters")
    , usewellsymbols_(usewellsymbols)
{
    if ( usewellsymbols_ )
    {
	const GMTWellSymbolRepository& rep = GMTWSR();
	const int size = rep.size();
	if ( size == 0 )
	    usewellsymbols_ = false;
    }

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "Symbol shape");
    shapefld_ = lcb->box();
    shapefld_->setHSzPol( usewellsymbols_ ? uiObject::Wide : uiObject::Small );
    fillShapes();

    sizefld_ = new uiGenInput( this, "Size (cm)",
	    		       FloatInpSpec( usewellsymbols_ ? 0.5 : 0.2 ) );
    sizefld_->setElemSzPol( uiObject::Small );
    sizefld_->attach( rightTo, lcb );

    outcolfld_ = new uiColorInput( this, uiColorInput::Setup(Color::Black())
	    				 .lbltxt("Outline color") );
    outcolfld_->attach( alignedBelow, lcb );
    if ( !usewellsymbols_ )
    {
	fillfld_ = new uiCheckBox( this, "Fill Color",
				   mCB(this,uiGMTSymbolPars,fillSel) );
	fillfld_->attach( rightTo, outcolfld_ );

	fillcolfld_ = new uiColorInput( this,
					uiColorInput::Setup(Color::White()) );
	fillcolfld_->attach( rightOf, fillfld_ );
	fillSel(0);
    }
}


void uiGMTSymbolPars::reset()
{
    shapefld_->setCurrentItem( 0 );
    sizefld_->setValue( 0.5 );
    outcolfld_->setColor( Color::Black() );
    if ( !usewellsymbols_ )
    {
	sizefld_->setValue( 0.2 );
	fillfld_->setChecked( false );
	fillcolfld_->setColor( Color::White() );
    }
}


void uiGMTSymbolPars::fillShapes()
{
    if ( usewellsymbols_ )
    {
	const int size = GMTWSR().size();
	for ( int idx=0; idx<size; idx++ )
	{
	    BufferString iconfilenm = GMTWSR().get( idx )->iconfilenm_;
	    BufferString name = GMTWSR().get( idx )->name();
	    shapefld_->insertItem( ioPixmap(iconfilenm), name.buf(), idx );
	}
    }
    else
    {
	for ( int idx=0; idx<6; idx++ )
	{
	    BufferString shapekey = ODGMT::ShapeNames()[idx];
	    if ( shapekey.isEmpty() ) break;

	    shapekey.buf()[0] = tolower( shapekey.buf()[0] );
	    shapekey += ".png";
	    shapefld_->insertItem( ioPixmap(shapekey), "", idx );
	}
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
    par.setYN( ODGMT::sKeyUseWellSymbolsYN, usewellsymbols_ );
    if ( !usewellsymbols_ )
    {
	const int shp = shapefld_->currentItem();
	BufferString shapestr = ODGMT::ShapeNames()[shp];
	par.set( ODGMT::sKeyShape, shapestr );
	par.setYN( ODGMT::sKeyFill, fillfld_->isChecked() );
	par.set( ODGMT::sKeyFillColor, fillcolfld_->color() );
    }
    else
    {
	const int selitem = shapefld_->currentItem();
	par.set( ODGMT::sKeyWellSymbolName, shapefld_->textOfItem(selitem) );
    }

    par.set( sKey::Size, sizefld_->getfValue() );
    par.set( sKey::Color, outcolfld_->color() );
    return true;
}


bool uiGMTSymbolPars::usePar( const IOPar& par )
{
    if ( usewellsymbols_ )
    {
	BufferString wellname;
	par.get( ODGMT::sKeyWellSymbolName, wellname );
	shapefld_->setCurrentItem( wellname );
    }
    else
    {
	ODGMT::Shape shp;
	if ( ODGMT::parseEnumShape( par.find( ODGMT::sKeyShape ), shp ) )
	    shapefld_->setCurrentItem( shp );

	Color col;
	bool dofill = false;
	par.getYN( ODGMT::sKeyFill, dofill );
	fillfld_->setChecked( dofill );
	fillfld_->setChecked( dofill );
	if ( dofill && par.get(ODGMT::sKeyFillColor,col) )
	    fillcolfld_->setColor( col );
    }

    float size;
    if ( par.get(sKey::Size,size) )
	sizefld_->setValue( size );

    Color col;
    if ( par.get(sKey::Color,col) )
	outcolfld_->setColor( col );

    return true;
}
