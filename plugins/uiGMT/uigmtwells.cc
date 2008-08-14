/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: uigmtwells.cc,v 1.2 2008-08-14 10:52:52 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtwells.h"

#include "gmtpar.h"
#include "ioman.h"
#include "ioobj.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"


int uiGMTWellsGrp::factoryid_ = -1;

void uiGMTWellsGrp::initClass()
{
    if ( factoryid_ < 0 )
	factoryid_ = uiGMTOF().add( "Wells",
				    uiGMTWellsGrp::createInstance );
}


uiGMTOverlayGrp* uiGMTWellsGrp::createInstance( uiParent* p )
{
    return new uiGMTWellsGrp( p );
}


uiGMTWellsGrp::uiGMTWellsGrp( uiParent* p )
    : uiGMTOverlayGrp(p,"Wells")
{
    uiLabeledListBox* llb = new uiLabeledListBox( this, "Select Wells", true );
    welllistfld_ = llb->box();
    Well::InfoCollector wic( false, false );
    wic.execute();
    for ( int idx=0; idx<wic.ids().size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( *wic.ids()[idx] );
	if ( ioobj )
	    welllistfld_->addItem( ioobj->name() );
    }

    namefld_ = new uiGenInput( this, "Name", StringInpSpec("Wells") );
    namefld_->attach( alignedBelow, llb );

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "Symbol shape");
    lcb->attach( alignedBelow, namefld_ );
    shapefld_ = lcb->box();
    shapefld_->setHSzPol( uiObject::Small );

    sizefld_ = new uiGenInput( this, "Size (cm)",
	    		       FloatInpSpec(0.2) );
    sizefld_->setElemSzPol( uiObject::Small );
    sizefld_->attach( rightTo, lcb );

    outcolfld_ = new uiColorInput( this, uiColorInput::Setup(Color::Black)
				   	.lbltxt("Color") );
    outcolfld_->attach( rightTo, sizefld_ );

    fillfld_ = new uiCheckBox( this, "Fill Color",
	   		       mCB(this,uiGMTWellsGrp,choiceSel) );
    fillfld_->attach( alignedBelow, lcb );

    fillcolfld_ = new uiColorInput( this, uiColorInput::Setup(Color::White) );
    fillcolfld_->attach( rightOf, fillfld_ );

    lebelfld_ = new uiCheckBox( this, "Post labels",
	   			mCB(this,uiGMTWellsGrp,choiceSel) );
    lebelfld_->attach( alignedBelow, fillfld_ );

    lebelalignfld_ = new uiComboBox( this, "Alignment" );
    lebelalignfld_->attach( rightTo, lebelfld_ );

    fillItems();
    choiceSel(0);
}


void uiGMTWellsGrp::fillItems()
{
    for ( int idx=0; idx<6; idx++ )
    {
	BufferString shapekey = eString( ODGMT::Shape, idx );
	if ( shapekey.isEmpty() ) break;

	shapekey.buf()[0] = tolower( shapekey.buf()[0] );
	shapekey += ".png";
	shapefld_->insertItem( ioPixmap(shapekey), "" );
    }

    for ( int idx=0; idx<4; idx++ )
    {
	BufferString alignkey = eString( ODGMT::Alignment, idx );
	lebelalignfld_->addItem( alignkey );
    }
}


void uiGMTWellsGrp::choiceSel( CallBacker* )
{
    if ( !fillcolfld_ || !lebelalignfld_ )
	return;

    fillcolfld_->setSensitive( fillfld_->isChecked() );
    lebelalignfld_->setSensitive( lebelfld_->isChecked() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMTWellsGrp::fillPar( IOPar& par ) const
{
    const int nrsel = welllistfld_->nrSelected();
    if ( !nrsel )
	mErrRet("Please select at least one well")

    par.set( sKey::Name, namefld_->text() );
    BufferStringSet selnames;
    welllistfld_->getSelectedItems( selnames );
    par.set( ODGMT::sKeyWellNames, selnames );
    const int shp = shapefld_->currentItem();
    BufferString shapestr = eString( ODGMT::Shape, shp );
    par.set( ODGMT::sKeyShape, shapestr );
    par.set( sKey::Size, sizefld_->getfValue() );
    par.setYN( ODGMT::sKeyFill, fillfld_->isChecked() );
    par.set( sKey::Color, outcolfld_->color() );
    par.set( ODGMT::sKeyFillColor, fillcolfld_->color() );
    par.setYN( ODGMT::sKeyPostLabel, lebelfld_->isChecked() );
    par.set( ODGMT::sKeyLabelAlignment, lebelalignfld_->text() );
    return true;
}


bool uiGMTWellsGrp::usePar( const IOPar& par )
{
    namefld_->setText( par.find(sKey::Name) );
    BufferStringSet selnames;
    par.get( ODGMT::sKeyWellNames, selnames );
    welllistfld_->clear();
    for ( int idx=0; idx<welllistfld_->size(); idx ++ )
    {
	const int index = selnames.indexOf( welllistfld_->textOfItem(idx) );
	if ( index >= 0 )
	    welllistfld_->setSelected( idx, true );
	else
	    welllistfld_->setSelected( idx, false );
    }

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

    bool postlabel = false;
    par.getYN( ODGMT::sKeyPostLabel, postlabel );
    lebelfld_->setChecked( postlabel );
    lebelalignfld_->setCurrentItem( par.find(ODGMT::sKeyLabelAlignment) );
    return true;
}

