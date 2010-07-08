/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          August 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratutildlgs.cc,v 1.23 2010-07-08 08:40:49 cvsbruno Exp $";

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
#include "uitable.h"

static const char* sNoLithoTxt      = "---None---";

#define mErrRet(msg,act) uiMSG().error(msg); act;
uiStratUnitDlg::uiStratUnitDlg( uiParent* p, Setup& su ) 
    : uiDialog(p,uiDialog::Setup("Stratigraphic Unit Properties",
				 "Specify properties of a new unit",
				 "110.0.1"))
    , uistratmgr_(su.uistratmgr_)
    , entrancename_(su.entrancename_)				 
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
    agestopfld_->setInterval( su.timerg_ );
    agestopfld_->setValue( su.timerg_.stop );
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
    //unitnmfld_->setSensitive(false);
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

    BufferString namemsg( "Unit name already used. Please specify a new name" );
    if ( strcmp(unitnmfld_->text(),entrancename_.buf()) )
    {
	if ( uistratmgr_ && !uistratmgr_->isNewUnitName(unitnmfld_->text()) ) 
	{ mErrRet( namemsg, return false ) }
    }

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



static const char* unitcollbls[] = { "[Name]", "[Color]", 
				     "Start(my)", "Stop(my)", 0 };
static const int cNrEmptyRows = 2;

static const int cNameCol  = 0;
static const int cColorCol = 1;
static const int cStartCol = 2;
static const int cStopCol = 3;

uiStratUnitDivideDlg::uiStratUnitDivideDlg( uiParent* p, const uiStratMgr& mgr, 
				const Strat::UnitRef::Props& parentprop ) 
    : uiDialog(p,uiDialog::Setup("Subdivide Stratigraphic Unit",
			     "Specify number and properties of the new units",
			     mNoHelpID))
    , parentprop_(parentprop)
    , uistratmgr_(mgr)		     
{
    table_ = new uiTable( this, uiTable::Setup().rowdesc("Unit")
						.rowgrow(true)
						.defrowlbl("")
						,"Subdivide Unit Table" );
    table_->setColumnLabels( unitcollbls );
    table_->setColumnReadOnly( cColorCol, true );
    table_->setColumnResizeMode( uiTable::ResizeToContents );
    table_->setNrRows( cNrEmptyRows );
    table_->leftClicked.notify( mCB(this,uiStratUnitDivideDlg,mouseClick) );
    table_->rowInserted.notify( mCB(this,uiStratUnitDivideDlg,resetUnits) );
    table_->rowDeleted.notify( mCB(this,uiStratUnitDivideDlg,resetUnits) );
    table_->setMinimumWidth( 450 );
    
    if ( table_->nrRows() )
	setUnit( 0, parentprop_ );

    resetUnits( 0 );
}


void uiStratUnitDivideDlg::mouseClick( CallBacker* )
{
    RowCol rc = table_->notifiedCell();
    if ( rc.col != cColorCol || table_->isCellReadOnly(rc) ) return;

    Color newcol = table_->getColor( rc );
    if ( selectColor(newcol,this,"Unit color") )
    table_->setColor( rc, newcol );
}


void uiStratUnitDivideDlg::resetUnits( CallBacker* cb ) 
{
    Interval<float> timerg = parentprop_.timerg_;
    ObjectSet<Strat::UnitRef::Props> pps;
    gatherProps( pps );
    const int nrrows = table_->nrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	Strat::UnitRef::Props* pp = pps[idx];
	if ( !pp )
	    break;
	BufferString bs( pp->code_ );
	if ( bs.isEmpty() )
	{
	    BufferString code( "New Unit" );
	    code += idx+1;
	    pp->code_ = code;
	    pp->color_ = getRandStdDrawColor();
	}
	pp->timerg_.start = timerg.start + (float)idx*timerg.width()/(nrrows);
	pp->timerg_.stop = timerg.start +(float)(idx+1)*timerg.width()/(nrrows);
	table_->setRowReadOnly( idx, false );
	setUnit( idx, *pp );
    }
    deepErase( pps );
    table_->setCellReadOnly( RowCol( 0, cStartCol ), true );
    table_->setCellReadOnly( RowCol( nrrows-1, cStopCol ), true );
}


void uiStratUnitDivideDlg::setUnit( int irow, const Strat::UnitRef::Props& pp ) 
{
    table_->setText( RowCol(irow,cNameCol), pp.code_ );
    table_->setValue( RowCol(irow,cStartCol), pp.timerg_.start  );
    table_->setValue( RowCol(irow,cStopCol), pp.timerg_.stop  );
    table_->setColor( RowCol(irow,cColorCol), pp.color_ );	       
}


void uiStratUnitDivideDlg::gatherProps( ObjectSet<Strat::UnitRef::Props>& pps ) 
{
    const int nrrows = table_->nrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	Strat::UnitRef::Props* pp = new Strat::UnitRef::Props();
	pp->code_ = table_->text( RowCol(idx,cNameCol) ); 
	pp->timerg_.start = table_->getfValue( RowCol(idx,cStartCol) );
	pp->timerg_.stop = table_->getfValue( RowCol(idx,cStopCol) );
	pp->color_ = table_->getColor( RowCol(idx,cColorCol) );
	pps += pp;
    }
}


bool uiStratUnitDivideDlg::areTimesOK( 
				ObjectSet<Strat::UnitRef::Props>& pps ) const
{
    if ( pps.size() > 1 )
	return true;
    for ( int idx=0; idx<pps.size()-1; idx++ )
    {
	const Strat::UnitRef::Props& curpp = *pps[idx];
	const Strat::UnitRef::Props& nextpp = *pps[idx+1];
	if ( curpp.timerg_.width() < 1 || nextpp.timerg_.width() < 1 )
	    return false;
	if ( curpp.timerg_.stop > nextpp.timerg_.start )
	    return false;
    }
    return ( pps[0]->timerg_.width() >= 1 );
}


bool uiStratUnitDivideDlg::acceptOK( CallBacker* )
{
    BufferStringSet bfs;
    ObjectSet<Strat::UnitRef::Props> pps;
    gatherProps( pps );
    for ( int idx=0; idx<pps.size(); idx++ )
    {
	BufferString code( pps[idx]->code_ );
	BufferString errmsg;
	if ( code.isEmpty() )
	{
	    errmsg += "Empty unit name. ";
	}
	if ( errmsg.isEmpty() && strcmp( code.buf() , parentprop_.code_ ) )
	{
	    if ( !uistratmgr_.isNewUnitName(pps[idx]->code_) ) 
		errmsg += "Unit name already used. ";
	}
	bfs.addIfNew( code );
	if ( errmsg.isEmpty() && bfs.size() < idx+1 )
	{
	    errmsg += "Unit name previously used in the list. ";
	}
	if ( !errmsg.isEmpty() )
	{
	    errmsg += "Please specify a new name for the unit number ";
	    errmsg += idx+1;
	    mErrRet( errmsg, deepErase( pps ); return false )	
	}
    }
    if ( !areTimesOK( pps ) )
	{ mErrRet( "No valid times specified", deepErase(pps); return false; ) }

    deepErase( pps );
    return true;
}
