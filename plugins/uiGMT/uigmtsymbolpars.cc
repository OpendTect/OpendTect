/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Sept 2008
________________________________________________________________________

-*/

#include "uigmtsymbolpars.h"

#include "gmtdef.h"
#include "iopar.h"
#include "keystrs.h"

#include "uicolor.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uipixmap.h"


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

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, tr("Symbol shape"));
    shapefld_ = lcb->box();
    shapefld_->setHSzPol( usewellsymbols_ ? uiObject::Wide : uiObject::Small );
    fillShapes();
    setHAlignObj( lcb );

    sizefld_ = new uiGenInput( this, tr("Size (cm)"),
			       FloatInpSpec( usewellsymbols_ ? 0.5f : 0.2f ) );
    sizefld_->setElemSzPol( uiObject::Small );
    sizefld_->attach( rightTo, lcb );

    outcolfld_ = new uiColorInput( this, uiColorInput::Setup(Color::Black())
					 .lbltxt(tr("Outline color")) );
    outcolfld_->attach( alignedBelow, lcb );
    if ( !usewellsymbols_ )
    {
	fillcolfld_ = new uiColorInput( this, uiColorInput::Setup(
		    Color::White()).lbltxt(tr("Fill Color")).withcheck(true) );
	fillcolfld_->attach( alignedBelow, outcolfld_ );
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
	fillcolfld_->setDoDraw( false );
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
	    BufferString nm = GMTWSR().get( idx )->name();
	    shapefld_->insertItem( uiPixmap(iconfilenm), toUiString(nm.buf()),
									  idx );
	}
    }
    else
    {
	for ( int idx=0; idx<6; idx++ )
	{
	    BufferString shapekey = ODGMT::ShapeDef().getKeyForIndex(idx);
	    if ( shapekey.isEmpty() ) break;

	    shapekey.getCStr()[0] = mCast( char, tolower( shapekey.buf()[0] ) );
	    shapefld_->insertItem( uiPixmap(shapekey),
				   uiString::empty(), idx );
	}
    }
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMTSymbolPars::fillPar( IOPar& par ) const
{
    par.setYN( ODGMT::sKeyUseWellSymbolsYN(), usewellsymbols_ );
    if ( !usewellsymbols_ )
    {
	const int shp = shapefld_->currentItem();
	BufferString shapestr = ODGMT::ShapeDef().getKeyForIndex(shp);
	par.set( ODGMT::sKeyShape(), shapestr );
	par.setYN( ODGMT::sKeyFill(), fillcolfld_->doDraw() );
	par.set( ODGMT::sKeyFillColor(), fillcolfld_->color() );
    }
    else
    {
	const int selitem = shapefld_->currentItem();
	par.set( ODGMT::sKeyWellSymbolName(), shapefld_->itemText(selitem) );
    }

    par.set( sKey::Size(), sizefld_->getFValue() );
    par.set( sKey::Color(), outcolfld_->color() );
    return true;
}


bool uiGMTSymbolPars::usePar( const IOPar& par )
{
    if ( usewellsymbols_ )
    {
	BufferString wellname;
	par.get( ODGMT::sKeyWellSymbolName(), wellname );
	shapefld_->setCurrentItem( wellname );
    }
    else
    {
	ODGMT::Shape shp;
	if ( ODGMT::ShapeDef().parse( par.find( ODGMT::sKeyShape() ), shp ) )
	    shapefld_->setCurrentItem( shp );

	Color col;
	bool dofill = false; par.getYN( ODGMT::sKeyFill(), dofill );
	fillcolfld_->setDoDraw( dofill );
	if ( dofill && par.get(ODGMT::sKeyFillColor(),col) )
	    fillcolfld_->setColor( col );
    }

    float size;
    if ( par.get(sKey::Size(),size) )
	sizefld_->setValue( size );

    Color col;
    if ( par.get(sKey::Color(),col) )
	outcolfld_->setColor( col );

    return true;
}
