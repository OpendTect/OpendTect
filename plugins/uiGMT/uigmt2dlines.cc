/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		August 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uigmt2dlines.h"

#include "ctxtioobj.h"
#include "draw.h"
#include "gmtpar.h"
#include "ioman.h"
#include "ioobj.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseislinesel.h"
#include "uisellinest.h"
#include "uispinbox.h"


int uiGMT2DLinesGrp::factoryid_ = -1;

void uiGMT2DLinesGrp::initClass()
{
    if ( factoryid_ < 0 )
	factoryid_ = uiGMTOF().add( "2D Lines",
				    uiGMT2DLinesGrp::createInstance );
}


uiGMTOverlayGrp* uiGMT2DLinesGrp::createInstance( uiParent* p )
{ return SI().has2D() ? new uiGMT2DLinesGrp( p ) : 0 ; }


uiGMT2DLinesGrp::uiGMT2DLinesGrp( uiParent* p )
    : uiGMTOverlayGrp(p,"2D Lines")
    , ctio_(*mMkCtxtIOObj(SeisTrc))
{
    namefld_ = new uiGenInput( this, "Name", StringInpSpec() );

    lineselfld_ = new uiSeis2DLineSel( this, true );
    lineselfld_->attach( alignedBelow, namefld_ );

    lsfld_ = new uiSelLineStyle( this, LineStyle(), "Line Style" );
    lsfld_->attach( alignedBelow, lineselfld_ );

    labelfld_ = new uiCheckBox( this, "Post Line names",
				mCB(this,uiGMT2DLinesGrp,labelSel) );
    const char* posoptions [] = { "Start", "End", "Both", 0 };
    labelposfld_ = new uiGenInput( this, "", StringListInpSpec(posoptions) );
    labelposfld_->attach( alignedBelow, lsfld_ );
    labelfld_->attach( leftOf, labelposfld_ );

    uiLabeledSpinBox* lsb = new uiLabeledSpinBox( this, "Font size" );
    labelfontfld_ = lsb->box();
    lsb->attach( rightOf, labelposfld_ );
    labelfontfld_->setInterval( 8, 20 );
    labelfontfld_->setValue( 10 );

    trclabelfld_ = new uiCheckBox( this, "Post Trace numbers",
				   mCB(this,uiGMT2DLinesGrp,labelSel) );
    trcstepfld_ = new uiGenInput( this, "Steps", IntInpSpec(100) );
    trcstepfld_->attach( alignedBelow, labelposfld_ );
    trclabelfld_->attach( leftOf, trcstepfld_ );

    labelSel( 0 );
}


uiGMT2DLinesGrp::~uiGMT2DLinesGrp()
{
    delete &ctio_;
}


void uiGMT2DLinesGrp::reset()
{
    namefld_->clear();
    lineselfld_->clearSelection();
    lsfld_->setStyle( LineStyle() );
    labelfld_->setChecked( false );
    labelfontfld_->setValue( 10 );
    trclabelfld_->setChecked( false );
    trcstepfld_->setValue( 100 );
    labelSel( 0 );
}


void uiGMT2DLinesGrp::labelSel( CallBacker* )
{
    const bool dolabel = labelfld_->isChecked();
    labelposfld_->setSensitive( dolabel );
    labelfontfld_->setSensitive( dolabel );
    trclabelfld_->setSensitive( dolabel );
    trcstepfld_->setSensitive( dolabel && trclabelfld_->isChecked());
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMT2DLinesGrp::fillPar( IOPar& par ) const
{
    TypeSet<Pos::GeomID> geomids;
    lineselfld_->getSelGeomIDs( geomids );
    if ( geomids.isEmpty() )
	mErrRet( "Please select at least one 2D line" );

    par.set( sKey::Name(), namefld_->text() );
    par.set( sKey::GeomID(), geomids );
    BufferString lskey;
    lsfld_->getStyle().toString( lskey );
    par.set( ODGMT::sKeyLineStyle(), lskey );
    const bool dolabel = labelfld_->isChecked();
    par.setYN( ODGMT::sKeyPostLabel(), dolabel );
    if ( dolabel )
    {
	par.set( ODGMT::sKeyFontSize(), labelfontfld_->getValue() );
	const int pos = labelposfld_->getIntValue();
	par.setYN( ODGMT::sKeyPostStart(), pos == 0 || pos == 2 );
	par.setYN( ODGMT::sKeyPostStop(), pos == 1 || pos == 2 );
	const bool dotrc = trclabelfld_->isChecked();
	par.setYN( ODGMT::sKeyPostTraceNrs(), dotrc );
	if ( dotrc )
	    par.set( ODGMT::sKeyLabelIntv(), trcstepfld_->getIntValue() );
    }

    return true;
}


bool uiGMT2DLinesGrp::usePar( const IOPar& par )
{
    FixedString nm = par.find( sKey::Name() );
    if ( nm ) namefld_->setText( nm );

    TypeSet<Pos::GeomID> geomids;
    par.get( sKey::GeomID(), geomids );
    if ( geomids.isEmpty() )
	mErrRet( "No 2D lines found " );

    lineselfld_->setSelGeomIDs( geomids );
    FixedString lskey = par.find( ODGMT::sKeyLineStyle() );
    if ( lskey )
    {
	LineStyle ls; ls.fromString( lskey.str() );
	lsfld_->setStyle( ls );
    }

    bool postlabel = false;
    par.getYN( ODGMT::sKeyPostLabel(), postlabel );
    labelfld_->setChecked( postlabel );
    int size = 10;
    par.get( ODGMT::sKeyFontSize(), size );
    labelfontfld_->setValue( size );
    labelSel( 0 );
    bool poststart = false, poststop = false, dotrc = false;
    par.getYN( ODGMT::sKeyPostStart(), poststart );
    par.getYN( ODGMT::sKeyPostStop(), poststop );
    par.getYN( ODGMT::sKeyPostTraceNrs(), dotrc );
    const int pos = poststart ? ( poststop ? 2 : 0 ) : 1;
    labelposfld_->setValue( pos );
    trclabelfld_->setChecked( dotrc );
    int labelstep = 100;
    par.get( ODGMT::sKeyLabelIntv(), labelstep );
    trcstepfld_->setValue( labelstep );
    return true;
}

