/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigmtpolyline.h"

#include "ctxtioobj.h"
#include "draw.h"
#include "gmtpar.h"
#include "ioman.h"
#include "ioobj.h"
#include "pickset.h"
#include "picksettr.h"
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
    : uiGMTOverlayGrp(p,uiStrings::sPolyLine())
    , ctio_(*mMkCtxtIOObj(PickSet))
{
    ctio_.ctxt_.requireType( sKey::Polygon() );
    inpfld_ = new uiIOObjSel( this, ctio_, uiStrings::sPolygon() );
    inpfld_->selectionDone.notify( mCB(this,uiGMTPolylineGrp,objSel) );

    namefld_ = new uiGenInput( this, uiStrings::sName(), StringInpSpec() );
    namefld_->setElemSzPol( uiObject::Wide );
    namefld_->attach( alignedBelow, inpfld_ );

    lsfld_ = new uiSelLineStyle(this, OD::LineStyle(), uiStrings::sLineStyle());
    lsfld_->attach( alignedBelow, namefld_ );

    fillcolfld_ = new uiColorInput( this,
					uiColorInput::Setup(OD::Color::White())
					.lbltxt(tr("Fill Color"))
                                        .withcheck(true) );
    fillcolfld_->attach( alignedBelow, lsfld_ );
}


void uiGMTPolylineGrp::reset()
{
    inpfld_->clear();
    namefld_->clear();
    lsfld_->setStyle( OD::LineStyle() );
    fillcolfld_->setColor( OD::Color::White() );
    fillcolfld_->setDoDraw( false );
}


void uiGMTPolylineGrp::objSel( CallBacker* )
{
    if ( !inpfld_->commitInput() )
	return;

    IOObj* ioobj = ctio_.ioobj_;
    if ( ioobj )
	namefld_->setText( ioobj->name() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMTPolylineGrp::fillPar( IOPar& par ) const
{
    if ( !inpfld_->commitInput() || !ctio_.ioobj_ )
	mErrRet(tr("Please select a polygon"))

    inpfld_->fillPar( par );
    par.set( sKey::Name(), namefld_->text() );
    BufferString lskey;
    lsfld_->getStyle().toString( lskey );
    par.set( ODGMT::sKeyLineStyle(), lskey );
    par.setYN( ODGMT::sKeyFill(), fillcolfld_->doDraw() );
    par.set( ODGMT::sKeyFillColor(), fillcolfld_->color() );
    return true;
}


bool uiGMTPolylineGrp::usePar( const IOPar& par )
{
    inpfld_->usePar( par );
    const BufferString nm = par.find( sKey::Name() );
    if ( !nm.isEmpty() )
	namefld_->setText( nm );

    const BufferString lskey = par.find( ODGMT::sKeyLineStyle() );
    if ( !lskey.isEmpty() )
    {
	OD::LineStyle ls; ls.fromString( lskey );
	lsfld_->setStyle( ls );
    }

    bool dofill = false;
    par.getYN( ODGMT::sKeyFill(), dofill );
    fillcolfld_->setDoDraw( dofill );
    if ( dofill )
    {
	OD::Color fillcol;
	par.get( ODGMT::sKeyFillColor(), fillcol );
	fillcolfld_->setColor( fillcol );
    }

    return true;
}
