/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		September 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigmtrandlines.cc,v 1.9 2011/04/01 09:44:21 cvsbert Exp $";

#include "uigmtrandlines.h"

#include "ctxtioobj.h"
#include "draw.h"
#include "gmtpar.h"
#include "ioman.h"
#include "ioobj.h"
#include "randomlinegeom.h"
#include "randomlinetr.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiioobjsel.h"
#include "uisellinest.h"
#include "uispinbox.h"


int uiGMTRandLinesGrp::factoryid_ = -1;

void uiGMTRandLinesGrp::initClass()
{
    if ( factoryid_ < 0 )
	factoryid_ = uiGMTOF().add( "Random Lines",
				    uiGMTRandLinesGrp::createInstance );
}


uiGMTOverlayGrp* uiGMTRandLinesGrp::createInstance( uiParent* p )
{ return SI().has3D() ? new uiGMTRandLinesGrp( p ) : 0 ; }


uiGMTRandLinesGrp::uiGMTRandLinesGrp( uiParent* p )
    : uiGMTOverlayGrp(p,"Random Lines")
    , ctio_(*mMkCtxtIOObj(RandomLineSet))
    , linenms_(*new BufferStringSet)
{
    inpfld_ = new uiIOObjSel( this, ctio_, "Line(s)" );
    inpfld_->selectionDone.notify( mCB(this,uiGMTRandLinesGrp,objSel) );

    namefld_ = new uiGenInput( this, "Name", StringInpSpec() );
    namefld_->attach( alignedBelow, inpfld_ );

    lsfld_ = new uiSelLineStyle( this, LineStyle(), "Line Style" );
    lsfld_->attach( alignedBelow, namefld_ );

    labelfld_ = new uiCheckBox( this, "Post label",
	   			mCB(this,uiGMTRandLinesGrp,labelSel) );
    uiLabeledSpinBox* lsb = new uiLabeledSpinBox( this, "Font size" );
    labelfontfld_ = lsb->box();
    lsb->attach( alignedBelow, lsfld_ );
    labelfontfld_->setInterval( 8, 20 );
    labelfontfld_->setValue( 10 );
    labelfld_->attach( leftOf, lsb );

    labelSel( 0 );
}


uiGMTRandLinesGrp::~uiGMTRandLinesGrp()
{
    delete &ctio_;
    delete &linenms_;
}


void uiGMTRandLinesGrp::reset()
{
    inpfld_->clear();
    namefld_->clear();
    lsfld_->setStyle( LineStyle() );
    labelfld_->setChecked( false );
    labelfontfld_->setValue( 10 );
    linenms_.erase();
    labelSel( 0 );
}


void uiGMTRandLinesGrp::objSel( CallBacker* )
{
    if ( !inpfld_->commitInput() || !ctio_.ioobj )
	return;

    namefld_->setText( ctio_.ioobj->name() );
    Geometry::RandomLineSet inprls; BufferString msg;
    if ( !RandomLineSetTranslator::retrieve(inprls,ctio_.ioobj,msg) )
	return;

    linenms_.erase();
    if ( inprls.size() == 1 )
	linenms_.add( inprls.lines()[0]->name() );
    else if ( inprls.size() > 1 )
    {
	uiDialog dlg( this, uiDialog::Setup("Select lines","Select lines","") );
	uiListBox* lb = new uiListBox( &dlg, "Linelist", true );
	for ( int idx=0; idx<inprls.size(); idx++ )
	    lb->addItem( inprls.lines()[idx]->name() );

	if ( !dlg.go() ) return;
	lb->getSelectedItems( linenms_ );
    }
}


void uiGMTRandLinesGrp::labelSel( CallBacker* )
{
    labelfontfld_->setSensitive( labelfld_->isChecked() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMTRandLinesGrp::fillPar( IOPar& par ) const
{
    if ( !inpfld_->commitInput() || !ctio_.ioobj )
	mErrRet("Please select the Random line(set)")

    inpfld_->fillPar( par );
    par.set( sKey::Name, namefld_->text() );
    par.set( ODGMT::sKeyLineNames, linenms_ );
    BufferString lskey;
    lsfld_->getStyle().toString( lskey );
    par.set( ODGMT::sKeyLineStyle, lskey );
    const bool dolabel = labelfld_->isChecked();
    par.setYN( ODGMT::sKeyPostLabel, dolabel );
    par.set( ODGMT::sKeyFontSize, labelfontfld_->getValue() );

    return true;
}


bool uiGMTRandLinesGrp::usePar( const IOPar& par )
{
    inpfld_->usePar( par );
    FixedString nm = par.find( sKey::Name );
    if ( nm ) namefld_->setText( nm );

    linenms_.erase();
    par.get( ODGMT::sKeyLineNames, linenms_ );
    FixedString lskey = par.find( ODGMT::sKeyLineStyle );
    if ( lskey )
    {
	LineStyle ls; ls.fromString( lskey.str() );
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

