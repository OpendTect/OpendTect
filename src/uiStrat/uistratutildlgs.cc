/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          August 2007
 RCS:		$Id: uistratutildlgs.cc,v 1.4 2007-09-05 15:31:52 cvshelene Exp $
________________________________________________________________________

-*/

#include "uistratutildlgs.h"

#include "randcolor.h"
#include "stratlevel.h"
#include "stratlith.h"
#include "stratunitrepos.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"

static const char* sNoLithoTxt      = "---None---";


uiStratUnitDlg::uiStratUnitDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Create Stratigraphic Unit","", 0))
{
    unitnmfld_ = new uiGenInput( this, "Name", StringInpSpec() );
    unitdescfld_ = new uiGenInput( this, "Description", StringInpSpec() );
    unitdescfld_->attach( alignedBelow, unitnmfld_ );
    unitlithfld_ = new uiGenInput( this, "Lithology", StringInpSpec() );
    unitlithfld_->attach( alignedBelow, unitdescfld_ );
    CallBack cb = mCB(this,uiStratUnitDlg,selLithCB);
    uiPushButton* sellithbut = new uiPushButton( this, "&Select", cb, false );
    sellithbut->attach( rightTo, unitlithfld_ );
}


void uiStratUnitDlg::selLithCB( CallBacker* )
{
    uiLithoDlg lithdlg(this);
    if ( lithdlg.go() )
	unitlithfld_->setText( lithdlg.getLithName() );
} 


const char* uiStratUnitDlg::getUnitName() const
{
    return unitnmfld_->text();
}


const char* uiStratUnitDlg::getUnitDesc() const
{
    return unitdescfld_->text();
}


const char* uiStratUnitDlg::getUnitLith() const
{
    const char* txt = unitlithfld_->text();
    return !strcmp( txt, sNoLithoTxt ) ? 0 : txt;
}


bool uiStratUnitDlg::acceptOK( CallBacker* )
{
    if ( !strcmp( getUnitName(), "" ) )
	{ uiMSG().error( "Please specify the unit name" ); return false; }
    return true;
}


uiLithoDlg::uiLithoDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Select Lithology","", 0))
{
    BufferStringSet lithoset;
    lithoset.add( sNoLithoTxt );
    for ( int idx=0; idx<Strat::UnRepo().nrLiths(); idx++ )
	lithoset.add( Strat::UnRepo().lith(idx).name() );
    
    listlithfld_ = new uiLabeledListBox( this, lithoset, "Existing lithologies",
	   				 false, uiLabeledListBox::AboveMid );
    uiGroup* rightgrp = new uiGroup( this, "right group" );
    uiLabel* lbl = new uiLabel( rightgrp,"Create new lithology with" );
    lithnmfld_ = new uiGenInput( rightgrp, "Name", StringInpSpec() );
    lithnmfld_->attach( alignedBelow, lbl );
    CallBack cb = mCB(this,uiLithoDlg,newLithCB);
    uiPushButton* newlithbut = new uiPushButton(rightgrp,"&Add as new",cb,true);
    newlithbut->attach( alignedBelow, lithnmfld_ );
    uiSeparator* sep = new uiSeparator( this, "Sep", false );
    sep->attach( rightTo, listlithfld_ );
    sep->attach( heightSameAs, listlithfld_ );
    rightgrp->attach( rightTo, sep );
}


void uiLithoDlg::newLithCB( CallBacker* )
{
    if ( listlithfld_->box()->isPresent( lithnmfld_->text() )
	 || !strcmp( lithnmfld_->text(), "" ) ) return;
    Strat::Lithology* newlith = new Strat::Lithology( lithnmfld_->text() );
    newlith->setId( Strat::UnRepo().getFreeLithID() );
    newlith->setSource( Strat::RT().source() );
    Strat::eUnRepo().addLith( newlith );

    listlithfld_->box()->addItem( lithnmfld_->text() );
    listlithfld_->box()->setCurrentItem( lithnmfld_->text() );
}


const char* uiLithoDlg::getLithName() const
{
    const char* txt = listlithfld_->box()->getText();
    return !strcmp( txt, sNoLithoTxt ) ? 0 : txt;
}


void uiLithoDlg::setSelectedLith( const char* lithnm )
{
    listlithfld_->box()->setCurrentItem( lithnm );
}


uiStratLevelDlg::uiStratLevelDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Create/Edit level","", 0))
{
    lvlnmfld_ = new uiGenInput( this, "Name", StringInpSpec() );
    lvlcolfld_ = new uiColorInput( this, getRandStdDrawColor(), "Color" );
    lvlcolfld_->attach( alignedBelow, lvlnmfld_ );
    lvltvstrgfld_ = new uiGenInput( this, "This level is ",
	    			    BoolInpSpec(true,"Isochron","Diachron") );
    lvltvstrgfld_->attach( alignedBelow, lvlcolfld_ );
    lvltvstrgfld_->valuechanged.notify( mCB(this,uiStratLevelDlg,isoDiaSel) );
    lvltimefld_ = new uiGenInput( this, "Level time (My)", FloatInpSpec() );
    lvltimefld_->attach( alignedBelow, lvltvstrgfld_ );
    lvltimergfld_ = new uiGenInput( this, "Level time range (My)",
	    			    FloatInpIntervalSpec() );
    lvltimergfld_->attach( alignedBelow, lvltvstrgfld_ );
    isoDiaSel(0);
}


void uiStratLevelDlg::isoDiaSel( CallBacker* )
{
    bool isiso = lvltvstrgfld_->getBoolValue();
    lvltimefld_->display( isiso );
    lvltimergfld_->display( !isiso );
}
