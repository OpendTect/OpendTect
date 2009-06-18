/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigmtpolyline.cc,v 1.8 2009-06-18 15:01:44 cvskris Exp $";

#include "uigmtpolyline.h"

#include "ctxtioobj.h"
#include "draw.h"
#include "gmtpar.h"
#include "ioman.h"
#include "ioobj.h"
#include "pickset.h"
#include "picksettr.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uisellinest.h"
#include "uimsg.h"


int uiGMTPolylineGrp::factoryid_ = -1;

void uiGMTPolylineGrp::initClass()
{
    if ( factoryid_ < 0 )
	factoryid_ = uiGMTOF().add( "Polyline",
				    uiGMTPolylineGrp::createInstance );
}


uiGMTOverlayGrp* uiGMTPolylineGrp::createInstance( uiParent* p )
{
    return new uiGMTPolylineGrp( p );
}


uiGMTPolylineGrp::uiGMTPolylineGrp( uiParent* p )
    : uiGMTOverlayGrp(p,"Polyline")
    , ctio_(*mMkCtxtIOObj(PickSet))
{
    ctio_.ctxt.parconstraints.set( sKey::Type, sKey::Polygon );
    ctio_.ctxt.allowcnstrsabsent = false;
    inpfld_ = new uiIOObjSel( this, ctio_,"Select Polygon" );
    inpfld_->selectiondone.notify( mCB(this,uiGMTPolylineGrp,objSel) );

    namefld_ = new uiGenInput( this, "Name", StringInpSpec() );
    namefld_->attach( alignedBelow, inpfld_ );

    lsfld_ = new uiSelLineStyle( this, LineStyle(), "Line Style" );
    lsfld_->attach( alignedBelow, namefld_ );

    fillfld_ = new uiCheckBox( this, "Fill Color",
	   		       mCB(this,uiGMTPolylineGrp,fillSel) );
    fillfld_->attach( alignedBelow, lsfld_ );

    fillcolfld_ = new uiColorInput( this,
	    			    uiColorInput::Setup(Color::White()) );
    fillcolfld_->attach( rightOf, fillfld_ );
    fillSel(0);
}


void uiGMTPolylineGrp::reset()
{
    inpfld_->clear();
    namefld_->clear();
    lsfld_->setStyle( LineStyle() );
    fillfld_->setChecked( false );
    fillcolfld_->setColor( Color::White() );
    fillSel( 0 );
}


void uiGMTPolylineGrp::objSel( CallBacker* )
{
    if ( !inpfld_->commitInput() )
	return;

    IOObj* ioobj = ctio_.ioobj;
    if ( ioobj )
	namefld_->setText( ioobj->name() );
}


void uiGMTPolylineGrp::fillSel( CallBacker* )
{
    if ( !fillcolfld_ || !fillfld_ )
	return;

    fillcolfld_->setSensitive( fillfld_->isChecked() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMTPolylineGrp::fillPar( IOPar& par ) const
{
    if ( !inpfld_->commitInput() || !ctio_.ioobj )
	mErrRet("Please select a polygon")

    inpfld_->fillPar( par );
    par.set( sKey::Name, namefld_->text() );
    BufferString lskey;
    lsfld_->getStyle().toString( lskey );
    par.set( ODGMT::sKeyLineStyle, lskey );
    par.setYN( ODGMT::sKeyFill, fillfld_->isChecked() );
    par.set( ODGMT::sKeyFillColor, fillcolfld_->color() );
    return true;
}


bool uiGMTPolylineGrp::usePar( const IOPar& par )
{
    inpfld_->usePar( par );
    FixedString nm = par.find( sKey::Name );
    if ( nm ) namefld_->setText( nm );

    FixedString lskey = par.find( ODGMT::sKeyLineStyle );
    if ( !lskey.isEmpty() )
    {
	LineStyle ls; ls.fromString( lskey.buf() );
	lsfld_->setStyle( ls );
    }

    bool dofill = false;
    par.getYN( ODGMT::sKeyFill, dofill );
    fillfld_->setChecked( dofill );
    if ( dofill )
    {
	Color fillcol;
	par.get( ODGMT::sKeyFillColor, fillcol );
	fillcolfld_->setColor( fillcol );
    }

    return true;
}

