/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigmtwells.h"

#include "gmtpar.h"
#include "ioman.h"
#include "ioobj.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uigmtsymbolpars.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uispinbox.h"


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
    : uiGMTOverlayGrp(p,toUiString("Wells"))
{
    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Well(s)") );
    welllistfld_ = new uiListBox( this, su );
    Well::InfoCollector wic( false, false );
    wic.execute();
    for ( int idx=0; idx<wic.ids().size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( wic.ids()[idx] );
	if ( ioobj )
	    welllistfld_->addItem( ioobj->name() );
    }

    namefld_ = new uiGenInput( this, uiStrings::sName(),
			       StringInpSpec("Wells") );
    namefld_->setElemSzPol( uiObject::Wide );
    namefld_->attach( alignedBelow, welllistfld_ );

    symbfld_ = new uiGMTSymbolPars( this, true );
    symbfld_->attach( alignedBelow, namefld_ );

    lebelfld_ = new uiCheckBox( this, tr("Post labels"),
				mCB(this,uiGMTWellsGrp,choiceSel) );
    lebelalignfld_ = new uiComboBox( this, "Alignment" );
    lebelalignfld_->attach( alignedBelow, symbfld_ );
    lebelfld_->attach( leftOf, lebelalignfld_ );
    uiLabeledSpinBox* lsb = new uiLabeledSpinBox( this, tr("Font size") );
    labelfontszfld_ = lsb->box();
    lsb->attach( rightTo, lebelalignfld_ );
    labelfontszfld_->setInterval( 8, 20 );
    labelfontszfld_->setValue( 10 );

    fillItems();
    choiceSel(0);
}


void uiGMTWellsGrp::fillItems()
{
    for ( int idx=0; idx<4; idx++ )
    {
	BufferString alignkey = ODGMT::AlignmentNames()[idx];
	lebelalignfld_->addItem( alignkey );
    }
}


void uiGMTWellsGrp::reset()
{
    welllistfld_->chooseAll( false );
    namefld_->setText( "Wells" );
    symbfld_->reset();
    lebelfld_->setChecked( false );
    labelfontszfld_->setValue( 10 );
    choiceSel( 0 );
}


void uiGMTWellsGrp::choiceSel( CallBacker* )
{
    if ( !lebelalignfld_ )
	return;

    lebelalignfld_->setSensitive( lebelfld_->isChecked() );
    labelfontszfld_->setSensitive( lebelfld_->isChecked() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMTWellsGrp::fillPar( IOPar& par ) const
{
    const int nrsel = welllistfld_->nrChosen();
    if ( !nrsel )
	mErrRet(tr("Please select at least one well"))

    const char* namestr = namefld_->text();
    if ( !namestr || !*namestr )
	mErrRet(tr(" Please enter name"))
    par.set( sKey::Name(), namefld_->text() );

    BufferStringSet selnames;
    welllistfld_->getChosen( selnames );
    par.set( ODGMT::sKeyWellNames(), selnames );
    symbfld_->fillPar( par );
    par.setYN( ODGMT::sKeyPostLabel(), lebelfld_->isChecked() );
    par.set( ODGMT::sKeyLabelAlignment(), lebelalignfld_->text() );
    par.set( ODGMT::sKeyFontSize(), labelfontszfld_->getIntValue() );
    return true;
}


bool uiGMTWellsGrp::usePar( const IOPar& par )
{
    namefld_->setText( par.find(sKey::Name()) );
    BufferStringSet selnames;
    par.get( ODGMT::sKeyWellNames(), selnames );
    welllistfld_->chooseAll( false );
    for ( int idx=0; idx<welllistfld_->size(); idx ++ )
    {
	const bool issel = selnames.isPresent( welllistfld_->textOfItem(idx) );
	welllistfld_->setChosen( idx, issel );
    }

    symbfld_->usePar( par );
    bool postlabel = false;
    par.getYN( ODGMT::sKeyPostLabel(), postlabel );
    lebelfld_->setChecked( postlabel );
    lebelalignfld_->setCurrentItem(
				par.find(ODGMT::sKeyLabelAlignment()).buf() );
    int fontsize = 10;
    par.get( ODGMT::sKeyFontSize(), fontsize );
    labelfontszfld_->setValue( fontsize );
    choiceSel( 0 );
    return true;
}
