/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
    : uiGMTOverlayGrp(p,uiStrings::sRandomLine(mPlural))
    , ctio_(*mMkCtxtIOObj(RandomLineSet))
    , linenms_(*new BufferStringSet)
{
    inpfld_ = new uiIOObjSel( this, ctio_, uiStrings::sLine(mPlural)  );
    inpfld_->selectionDone.notify( mCB(this,uiGMTRandLinesGrp,objSel) );

    namefld_ = new uiGenInput( this, uiStrings::sName(), StringInpSpec() );
    namefld_->setElemSzPol( uiObject::Wide );
    namefld_->attach( alignedBelow, inpfld_ );

    lsfld_ = new uiSelLineStyle(this, OD::LineStyle(), uiStrings::sLineStyle());
    lsfld_->attach( alignedBelow, namefld_ );

    labelfld_ = new uiCheckBox( this, tr("Post label"),
				mCB(this,uiGMTRandLinesGrp,labelSel) );
    uiLabeledSpinBox* lsb = new uiLabeledSpinBox( this, tr("Font size") );
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
    lsfld_->setStyle( OD::LineStyle() );
    labelfld_->setChecked( false );
    labelfontfld_->setValue( 10 );
    linenms_.erase();
    labelSel( 0 );
}


void uiGMTRandLinesGrp::objSel( CallBacker* )
{
    if ( !inpfld_->commitInput() || !ctio_.ioobj_ )
	return;

    namefld_->setText( ctio_.ioobj_->name() );
    Geometry::RandomLineSet inprls; BufferString msg;
    if ( !RandomLineSetTranslator::retrieve(inprls,ctio_.ioobj_,msg) )
	return;

    linenms_.erase();
    if ( inprls.size() == 1 )
	linenms_.add( inprls.lines()[0]->name() );
    else if ( inprls.size() > 1 )
    {
	uiDialog dlg( this, uiDialog::Setup(tr("Select lines"),
                                            tr("Select lines"),
                                            mNoHelpKey) );
	uiListBox* lb = new uiListBox( &dlg, "Linelist", OD::ChooseAtLeastOne);
	for ( int idx=0; idx<inprls.size(); idx++ )
	    lb->addItem( inprls.lines()[idx]->name() );

	if ( !dlg.go() ) return;
	lb->getChosen( linenms_ );
    }
}


void uiGMTRandLinesGrp::labelSel( CallBacker* )
{
    labelfontfld_->setSensitive( labelfld_->isChecked() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMTRandLinesGrp::fillPar( IOPar& par ) const
{
    if ( !inpfld_->commitInput() || !ctio_.ioobj_ )
	mErrRet(tr("Please select the Random line(set)"))

    inpfld_->fillPar( par );
    par.set( sKey::Name(), namefld_->text() );
    par.set( ODGMT::sKeyLineNames(), linenms_ );
    BufferString lskey;
    lsfld_->getStyle().toString( lskey );
    par.set( ODGMT::sKeyLineStyle(), lskey );
    const bool dolabel = labelfld_->isChecked();
    par.setYN( ODGMT::sKeyPostLabel(), dolabel );
    par.set( ODGMT::sKeyFontSize(), labelfontfld_->getIntValue() );

    return true;
}


bool uiGMTRandLinesGrp::usePar( const IOPar& par )
{
    inpfld_->usePar( par );
    const BufferString nm = par.find( sKey::Name() );
    if ( !nm.isEmpty() )
	namefld_->setText( nm );

    linenms_.erase();
    par.get( ODGMT::sKeyLineNames(), linenms_ );
    const BufferString lskey = par.find( ODGMT::sKeyLineStyle() );
    if ( !lskey.isEmpty() )
    {
	OD::LineStyle ls; ls.fromString( lskey.str() );
	lsfld_->setStyle( ls );
    }

    bool postlabel = false;
    par.getYN( ODGMT::sKeyPostLabel(), postlabel );
    labelfld_->setChecked( postlabel );
    int size = 10;
    par.get( ODGMT::sKeyFontSize(), size );
    labelfontfld_->setValue( size );
    labelSel( 0 );
    return true;
}
