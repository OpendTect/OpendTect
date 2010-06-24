/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          August 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratutildlgs.cc,v 1.19 2010-06-24 11:54:01 cvsbruno Exp $";

#include "uistratutildlgs.h"

#include "randcolor.h"
#include "stratlith.h"
#include "stratunitrepos.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uispinbox.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistratmgr.h"

static const char* sNoLithoTxt      = "---None---";

#define mErrRet(msg,act) uiMSG().error(msg); act;
uiStratUnitDlg::uiStratUnitDlg( uiParent* p, Setup& su ) 
    : uiDialog(p,uiDialog::Setup("Stratigraphic Unit Properties",
				 "Specify properties of a new unit",
				 "110.0.1"))
    , uistratmgr_(su.uistratmgr_)
{
    unitnmfld_ = new uiGenInput( this, "Name", StringInpSpec() );
    unitnmfld_->valuechanged.notify( mCB(this,uiStratUnitDlg,selNameCB) );
    unitdescfld_ = new uiGenInput( this, "Description", StringInpSpec() );
    unitdescfld_->attach( alignedBelow, unitnmfld_ );
    unitlithfld_ = new uiGenInput( this, "Lithology", StringInpSpec() );
    unitlithfld_->attach( alignedBelow, unitdescfld_ );
    CallBack cb = mCB(this,uiStratUnitDlg,selLithCB);
    uiPushButton* sellithbut = new uiPushButton( this, "&Select", cb, false );
    sellithbut->attach( rightTo, unitlithfld_ );
    colfld_ = new uiColorInput( this,
			           uiColorInput::Setup(getRandStdDrawColor() ).
				   lbltxt("Color") );
    colfld_->attach( alignedBelow, unitdescfld_ );
    colfld_->attach( ensureBelow, sellithbut );
    uiLabeledSpinBox* lblbox1 = new uiLabeledSpinBox( this, "Time range (My)" );
    agestartfld_ = lblbox1->box();
    agestartfld_->setInterval( su.timerg_ );
    agestartfld_->setValue( su.timerg_.start );
    lblbox1->attach( ensureBelow, colfld_ );
    lblbox1->attach( alignedBelow, unitlithfld_ );
    
    uiLabeledSpinBox* lblbox2 = new uiLabeledSpinBox( this, "" );
    agestopfld_ = lblbox2->box();
    agestopfld_->setValue( su.timerg_.stop );
    agestopfld_->setInterval( su.timerg_ );
    lblbox2->attach( rightOf, lblbox1 );

    lvlnmfld_ = new uiGenInput( this, "Level (top) name", StringInpSpec() );
    lvlnmfld_->attach( alignedBelow, lblbox1 );
}


void uiStratUnitDlg::selNameCB( CallBacker* )
{
    BufferString lvlnm( unitnmfld_->text() );
    lvlnm += " Level";
    lvlnmfld_->setText( lvlnm );
}


void uiStratUnitDlg::selLithCB( CallBacker* )
{
    uiStratLithoDlg lithdlg( this, uistratmgr_ );
    if ( lithdlg.go() )
	unitlithfld_->setText( lithdlg.getLithName() );
} 


void uiStratUnitDlg::setUnitProps( const Strat::UnitRef::Props& props ) 
{
    unitnmfld_->setText( props.code_ );
    //TODO: rename unit needs extra work to update all the paths to the subunits
    unitnmfld_->setSensitive(false);
    agestartfld_->setValue( props.timerg_.start );
    agestopfld_->setValue( props.timerg_.stop );
    colfld_->setColor( props.color_ );
    unitdescfld_->setText( props.desc_ );
    unitlithfld_->setText( props.lithnm_ );
    unitlithfld_->setSensitive( props.isleaf_ );
    lvlnmfld_->setText( props.lvlname_ );
}



void uiStratUnitDlg::getUnitProps( Strat::UnitRef::Props& props) const
{
    props.code_ = unitnmfld_->text();
    props.desc_ = unitdescfld_->text();
    const char* txt = unitlithfld_->text();
    props.lithnm_ = !strcmp( txt, sNoLithoTxt ) ? 0 : txt;
    props.timerg_ = Interval<float> ( agestartfld_->getValue(), 
				      agestopfld_->getValue() );
    props.color_ = colfld_->color();
    props.lvlname_ = lvlnmfld_->text();
}



bool uiStratUnitDlg::acceptOK( CallBacker* )
{
    if ( agestartfld_->getValue() >= agestopfld_->getValue() )
	{ mErrRet( "Please specify a valid time range", return false ) }
    if ( !strcmp( unitnmfld_->text(), "" ) )
	{ mErrRet( "Please specify the unit name", return false ) }
    if ( !strcmp( lvlnmfld_->text(), "" ) )
	{ mErrRet( "Please specify a name for the unit level", return false ) }
    return true;
}



uiStratLithoDlg::uiStratLithoDlg( uiParent* p, uiStratMgr* uistratmgr )
    : uiDialog(p,uiDialog::Setup("Select Lithology",mNoDlgTitle,"110.0.4"))
    , uistratmgr_(uistratmgr)
    , prevlith_(0)
    , nmfld_(0)
{
    selfld_ = new uiListBox( this, "Lithology", false );
    const CallBack cb( mCB(this,uiStratLithoDlg,selChg) );
    selfld_->selectionChanged.notify( cb );
    fillLiths();

    uiGroup* rightgrp = new uiGroup( this, "right group" );
    nmfld_ = new uiGenInput( rightgrp, "Name", StringInpSpec() );
    isporbox_ = new uiCheckBox( rightgrp, "Porous" );
    isporbox_->attach( alignedBelow, nmfld_ );
    uiPushButton* newlithbut = new uiPushButton( rightgrp, "&Add as new",
	    		mCB(this,uiStratLithoDlg,newLith), true );
    newlithbut->attach( alignedBelow, isporbox_ );

    uiSeparator* sep = new uiSeparator( this, "Sep", false );
    sep->attach( rightTo, selfld_ );
    sep->attach( heightSameAs, selfld_ );
    rightgrp->attach( rightTo, sep );

    uiButton* renamebut = new uiPushButton( this, "Re&name selected",
				    mCB(this,uiStratLithoDlg,renameCB), true );
    renamebut->attach( alignedBelow, rightgrp );

    uiButton* rmbut = new uiPushButton( this, "&Remove selected",
	    				mCB(this,uiStratLithoDlg,rmSel), true );
    rmbut->attach( alignedBelow, renamebut );

    finaliseDone.notify( cb );
}


void uiStratLithoDlg::fillLiths()
{
    BufferStringSet nms;
    nms.add( sNoLithoTxt );
    uistratmgr_->getLithoNames( nms );
    selfld_->empty();
    selfld_->addItems( nms );
}
    


void uiStratLithoDlg::newLith( CallBacker* )
{
    const BufferString nm( nmfld_->text() );
    if ( nm.isEmpty() ) return;

    if ( selfld_->isPresent( nm ) )
	{ mErrRet( "Please specify a new, unique name", return ) }

    const Strat::Lithology* lith =
		    uistratmgr_->createNewLith( nm, isporbox_->isChecked() );
    if ( !lith ) lith = &Strat::Lithology::undef();
    prevlith_ = const_cast<Strat::Lithology*>( lith );

    selfld_->addItem( nm );
    selfld_->setCurrentItem( nm );
}


void uiStratLithoDlg::selChg( CallBacker* )
{
    if ( !nmfld_ ) return;

    if ( prevlith_ )
    {
	const bool newpor = isporbox_->isChecked();
	if ( newpor != prevlith_->porous_ && !prevlith_->isUdf() )
	{
	    prevlith_->porous_ = isporbox_->isChecked();
	    uistratmgr_->lithChanged.trigger();
	}
    }
    const BufferString nm( selfld_->getText() );
    const Strat::Lithology* lith = uistratmgr_->getLith( nm );
    if ( !lith ) lith = &Strat::Lithology::undef();
    nmfld_->setText( lith->name() );
    isporbox_->setChecked( lith->porous_ );
    prevlith_ = const_cast<Strat::Lithology*>( lith );
}


void uiStratLithoDlg::renameCB( CallBacker* )
{
    Strat::Lithology* lith = const_cast<Strat::Lithology*>(
				uistratmgr_->getLith( selfld_->getText() ) );
    if ( !lith || lith->isUdf() ) return;

    lith->setName( nmfld_->text() );
    selfld_->setItemText( selfld_->currentItem(), nmfld_->text() );
    uistratmgr_->lithChanged.trigger();
    prevlith_ = lith;
}


void uiStratLithoDlg::rmSel( CallBacker* )
{
    int selidx = selfld_->currentItem();
    if ( selidx < 0 ) return;

    const Strat::Lithology* lith =
			uistratmgr_->getLith( selfld_->textOfItem(selidx) );
    if ( !lith || lith->isUdf() ) return;

    prevlith_ = 0;
    uistratmgr_->deleteLith( lith->id_ );
    fillLiths();

    if ( selidx >= selfld_->size() )
	selidx = selfld_->size() - 1;

    if ( selidx < 0 )
	nmfld_->setText( "" );
    else
    {
	selfld_->setCurrentItem( selidx );
	selChg( 0 );
    }
}


const char* uiStratLithoDlg::getLithName() const
{
    const char* txt = selfld_->getText();
    return !strcmp( txt, sNoLithoTxt ) ? 0 : txt;
}


void uiStratLithoDlg::setSelectedLith( const char* lithnm )
{
    const Strat::Lithology* lith = uistratmgr_->getLith( lithnm );
    if ( !lith ) return;
    selfld_->setCurrentItem( lithnm );
}


bool uiStratLithoDlg::acceptOK( CallBacker* )
{
    selChg( 0 );
    return true;
}

