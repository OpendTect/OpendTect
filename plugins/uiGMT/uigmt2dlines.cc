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
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseissel.h"
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
    inpfld_ = new uiSeisSel( this, ctio_, uiSeisSel::Setup(Seis::Line) );
    inpfld_->selectionDone.notify( mCB(this,uiGMT2DLinesGrp,objSel) );

    namefld_ = new uiGenInput( this, "Name", StringInpSpec() );
    namefld_->attach( alignedBelow, inpfld_ );

    uiLabeledListBox* llb = new uiLabeledListBox( this, "Lines", true );
    linelistfld_ = llb->box();
    llb->attach( alignedBelow, namefld_ );

    lsfld_ = new uiSelLineStyle( this, LineStyle(), "Line Style" );
    lsfld_->attach( alignedBelow, llb );

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
    inpfld_->clear();
    namefld_->clear();
    linelistfld_->setEmpty();
    lsfld_->setStyle( LineStyle() );
    labelfld_->setChecked( false );
    labelfontfld_->setValue( 10 );
    trclabelfld_->setChecked( false );
    trcstepfld_->setValue( 100 );
    labelSel( 0 );
}


void uiGMT2DLinesGrp::objSel( CallBacker* )
{
    if ( !inpfld_->commitInput() || !ctio_.ioobj )
	return;

    namefld_->setText( ctio_.ioobj->name() );
    SeisIOObjInfo info( *ctio_.ioobj );
    BufferStringSet linenms;
    info.getLineNames( linenms );
    linelistfld_->setEmpty();
    linelistfld_->addItems( linenms );
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
    if ( !inpfld_->commitInput() || !ctio_.ioobj )
	mErrRet("Please select a lineset")

    if ( !linelistfld_->nrSelected() )
	mErrRet("Please select at least one line")

    inpfld_->fillPar( par );
    par.set( sKey::Name(), namefld_->text() );
    BufferStringSet linenms;
    linelistfld_->getSelectedItems( linenms );
    par.set( ODGMT::sKeyLineNames(), linenms );
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
    inpfld_->usePar( par );
    objSel( 0 );
    FixedString nm = par.find( sKey::Name() );
    if ( nm ) namefld_->setText( nm );

    BufferStringSet linenms;
    par.get( ODGMT::sKeyLineNames(), linenms );
    linelistfld_->clearSelection();
    for ( int idx=0; idx<linelistfld_->size(); idx++ )
	if ( linenms.indexOf(linelistfld_->textOfItem(idx)) >= 0 )
	    linelistfld_->setSelected( idx, true );

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

