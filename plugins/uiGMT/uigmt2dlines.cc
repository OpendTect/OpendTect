/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: uigmt2dlines.cc,v 1.2 2008-08-20 05:26:14 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmt2dlines.h"

#include "ctxtioobj.h"
#include "draw.h"
#include "gmtpar.h"
#include "ioman.h"
#include "ioobj.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uiseissel.h"
#include "uisellinest.h"
#include "uispinbox.h"
#include "uimsg.h"


int uiGMT2DLinesGrp::factoryid_ = -1;

void uiGMT2DLinesGrp::initClass()
{
    if ( factoryid_ < 0 )
	factoryid_ = uiGMTOF().add( "2D Lines",
				    uiGMT2DLinesGrp::createInstance );
}


uiGMTOverlayGrp* uiGMT2DLinesGrp::createInstance( uiParent* p )
{
    return new uiGMT2DLinesGrp( p );
}


uiGMT2DLinesGrp::uiGMT2DLinesGrp( uiParent* p )
    : uiGMTOverlayGrp(p,"2D Lines")
    , ctio_(*mMkCtxtIOObj(SeisTrc))
{
    inpfld_ = new uiSeisSel( this, ctio_, uiSeisSel::Setup(Seis::Line) );
    inpfld_->selectiondone.notify( mCB(this,uiGMT2DLinesGrp,objSel) );

    namefld_ = new uiGenInput( this, "Name", StringInpSpec() );
    namefld_->attach( alignedBelow, inpfld_ );

    uiLabeledListBox* llb = new uiLabeledListBox( this, "Select lines", true );
    linelistfld_ = llb->box();
    llb->attach( alignedBelow, namefld_ );

    lsfld_ = new uiSelLineStyle( this, LineStyle(), "Line Style" );
    lsfld_->attach( alignedBelow, llb );

    labelfld_ = new uiCheckBox( this, "Post label",
	   			mCB(this,uiGMT2DLinesGrp,labelSel) );
    labelfld_->attach( alignedBelow, lsfld_ );

    uiLabeledSpinBox* lsb = new uiLabeledSpinBox( this, "Font size" );
    labelfontfld_ = lsb->box();
    lsb->attach( rightOf, labelfld_ );
    labelfontfld_->setInterval( 8, 20 );
    labelfontfld_->setValue( 10 );
    labelSel( 0 );
}


void uiGMT2DLinesGrp::objSel( CallBacker* )
{
    if ( !inpfld_->commitInput(false) )
	return;

    IOObj* ioobj = ctio_.ioobj;
    if ( !ioobj ) return;

    namefld_->setText( ioobj->name() );
    SeisIOObjInfo info( *ioobj );
    BufferStringSet linenms;
    info.getLineNames( linenms );
    linelistfld_->empty();
    linelistfld_->addItems( linenms );
}


void uiGMT2DLinesGrp::labelSel( CallBacker* )
{
    labelfontfld_->setSensitive( labelfld_->isChecked() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMT2DLinesGrp::fillPar( IOPar& par ) const
{
    if ( !inpfld_->commitInput(false) || !ctio_.ioobj )
	mErrRet("Please select a lineset")

    if ( !linelistfld_->nrSelected() )
	mErrRet("Please select at least one line")

    inpfld_->fillPar( par );
    par.set( sKey::Name, namefld_->text() );
    BufferStringSet linenms;
    linelistfld_->getSelectedItems( linenms );
    par.set( ODGMT::sKeyLineNames, linenms );
    BufferString lskey;
    lsfld_->getStyle().toString( lskey );
    par.set( ODGMT::sKeyLineStyle, lskey );
    par.setYN( ODGMT::sKeyPostLabel, labelfld_->isChecked() );
    par.set( ODGMT::sKeyFontSize, labelfontfld_->getValue() );
    return true;
}


bool uiGMT2DLinesGrp::usePar( const IOPar& par )
{
    inpfld_->usePar( par );
    objSel( 0 );
    const char* nm = par.find( sKey::Name );
    if ( nm && *nm ) namefld_->setText( nm );

    BufferStringSet linenms;
    par.get( ODGMT::sKeyLineNames, linenms );
    linelistfld_->clear();
    for ( int idx=0; idx<linelistfld_->size(); idx++ )
	if ( linenms.indexOf(linelistfld_->textOfItem(idx)) >= 0 )
	    linelistfld_->setSelected( idx, true );

    BufferString lskey = par.find( ODGMT::sKeyLineStyle );
    if ( !lskey.isEmpty() )
    {
	LineStyle ls; ls.fromString( lskey.buf() );
	lsfld_->setStyle( ls );
    }

    bool postlabel = false;
    par.getYN( ODGMT::sKeyPostLabel, postlabel );
    labelfld_->setChecked( postlabel );
    int size = 10;
    par.get( ODGMT::sKeyFontSize, size );
    labelfontfld_->setValue( size );
    labelSel( 0 );
    
    return true;
}

