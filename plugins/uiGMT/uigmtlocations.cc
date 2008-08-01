/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: uigmtlocations.cc,v 1.1 2008-08-01 08:31:21 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtlocations.h"

#include "ctxtioobj.h"
#include "gmtpar.h"
#include "ioobj.h"
#include "ioman.h"
#include "picksettr.h"
#include "pixmap.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"


int uiGMTLocationsGrp::factoryid_ = -1;

void uiGMTLocationsGrp::initClass()
{
    if ( factoryid_ < 0 )
	factoryid_ = uiGMTOF().add( "Locations",
				    uiGMTLocationsGrp::createInstance );
}


uiGMTOverlayGrp* uiGMTLocationsGrp::createInstance( uiParent* p )
{
    return new uiGMTLocationsGrp( p );
}


uiGMTLocationsGrp::uiGMTLocationsGrp( uiParent* p )
    : uiGMTOverlayGrp(p,"Locations")
    , ctio_(*mMkCtxtIOObj(PickSet))
{
    inpfld_ = new uiIOObjSel( this, ctio_,"Select Pickset" );
    inpfld_->selectiondone.notify( mCB(this,uiGMTLocationsGrp,objSel) );

    namefld_ = new uiGenInput( this, "Name", StringInpSpec() );
    namefld_->attach( alignedBelow, inpfld_ );

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "Symbol shape");
    lcb->attach( alignedBelow, namefld_ );
    shapefld_ = lcb->box();
    shapefld_->setHSzPol( uiObject::Small );
    fillShapes();

    sizefld_ = new uiGenInput( this, "Size (cm)",
	    		       FloatInpSpec(0.2) );
    sizefld_->setElemSzPol( uiObject::Small );
    sizefld_->attach( rightTo, lcb );

    outcolfld_ = new uiColorInput( this,
	    			   uiColorInput::Setup(Color::DgbColor)
				   .lbltxt("Outline color") );
    outcolfld_->attach( alignedBelow, lcb );

    fillfld_ = new uiCheckBox( this, "Fill Color",
	   		       mCB(this,uiGMTLocationsGrp,fillSel) );
    fillfld_->attach( rightTo, outcolfld_ );

    fillcolfld_ = new uiColorInput( this,
	    			    uiColorInput::Setup(Color::DgbColor) );
    fillcolfld_->attach( rightOf, fillfld_ );
    fillSel(0);
}


void uiGMTLocationsGrp::fillShapes()
{
    for ( int idx=0; idx<6; idx++ )
    {
	BufferString shapekey = eString( ODGMT::Shape, idx );
	if ( shapekey.isEmpty() ) break;

	shapekey.buf()[0] = tolower( shapekey.buf()[0] );
	shapekey += ".png";
	shapefld_->insertItem( ioPixmap(shapekey), "" );
    }
}


void uiGMTLocationsGrp::objSel( CallBacker* )
{
    if ( !inpfld_->commitInput(false) )
	return;

    IOObj* ioobj = ctio_.ioobj;
    if ( ioobj ) 
	namefld_->setText( ioobj->name() );
}


void uiGMTLocationsGrp::fillSel( CallBacker* )
{
    if ( !fillcolfld_ || !fillfld_ )
	return;

    fillcolfld_->setSensitive( fillfld_->isChecked() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMTLocationsGrp::fillPar( IOPar& par ) const
{
    if ( !inpfld_->commitInput(false) || !ctio_.ioobj )
	mErrRet("Please select a pickset")

    inpfld_->fillPar( par );
    par.set( sKey::Name, namefld_->text() );
    const int shp = shapefld_->currentItem();
    BufferString shapestr = eString( ODGMT::Shape, shp );
    par.set( ODGMT::sKeyShape, shapestr );
    par.set( sKey::Size, sizefld_->getfValue() );
    par.setYN( ODGMT::sKeyFill, fillfld_->isChecked() );
    par.set( sKey::Color, outcolfld_->color() );
    par.set( ODGMT::sKeyFillColor, fillcolfld_->color() );
    return true;
}


bool uiGMTLocationsGrp::usePar( const IOPar& par )
{
    inpfld_->usePar( par );
    const char* nm = par.find( sKey::Name );
    if ( nm && *nm ) namefld_->setText( nm );

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

