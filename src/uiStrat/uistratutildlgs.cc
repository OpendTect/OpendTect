/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          August 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratutildlgs.cc,v 1.29 2010-09-08 07:57:32 cvsbruno Exp $";

#include "uistratutildlgs.h"

#include "iopar.h"
#include "randcolor.h"
#include "stratlith.h"
#include "stratunitrepos.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uicolor.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uispinbox.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitable.h"

static const char* sNoLithoTxt      = "---None---";
static const char* sNoLevelTxt      = "--Undefined--";

#define mErrRet(msg,act) uiMSG().error(msg); act;
uiStratUnitEditDlg::uiStratUnitEditDlg( uiParent* p, Strat::UnitRef& unit ) 
    : uiDialog(p,uiDialog::Setup("Stratigraphic Unit Editor",
				 "Edit the unit properties",
				 "110.0.1"))
    , unit_(unit)
    , entrancename_(unit.code())		 
{
    unitnmfld_ = new uiGenInput( this, "Name", StringInpSpec() );
    unitdescfld_ = new uiGenInput( this, "Description", StringInpSpec() );
    unitdescfld_->attach( alignedBelow, unitnmfld_ );
    unitlithfld_ = new uiGenInput( this, "Lithology", StringInpSpec() );
    unitlithfld_->attach( alignedBelow, unitdescfld_ );
    CallBack cb = mCB(this,uiStratUnitEditDlg,selLithCB);
    uiPushButton* sellithbut = new uiPushButton( this, "&Select", cb, false );
    sellithbut->attach( rightTo, unitlithfld_ );
    colfld_ = new uiColorInput( this,
			           uiColorInput::Setup(getRandStdDrawColor() ).
				   lbltxt("Color") );
    colfld_->attach( alignedBelow, unitdescfld_ );
    colfld_->attach( ensureBelow, sellithbut );

    uiLabeledSpinBox* lblbox1 = new uiLabeledSpinBox( this, "Time range (My)" );
    agestartfld_ = lblbox1->box();
    agestartfld_->setInterval( unit_.timeRange() );
    lblbox1->attach( ensureBelow, colfld_ );
    lblbox1->attach( alignedBelow, unitlithfld_ );
    
    uiLabeledSpinBox* lblbox2 = new uiLabeledSpinBox( this, "" );
    agestopfld_ = lblbox2->box();
    agestopfld_->setInterval( unit_.timeRange() );
    lblbox2->attach( rightOf, lblbox1 );

    putToScreen();
}


void uiStratUnitEditDlg::selLithCB( CallBacker* )
{
    uiStratLithoDlg lithdlg( this );
    if ( lithdlg.go() )
    {
	unitlithfld_->setText( lithdlg.getLithName() );
	lithnm_ = lithdlg.getLithName();
    }
} 


void uiStratUnitEditDlg::putToScreen()
{
    unitnmfld_->setText( unit_.code() );
    unitdescfld_->setText( unit_.description() );
    unitlithfld_->setText( lithnm_.buf() );
    unitlithfld_->setSensitive( unit_.isLeaf() );

    IOPar iop; unit_.putTo( iop );
    Color col; iop.get( sKey::Color, col );
    Interval<float> timerg; iop.get( sKey::Time, timerg );
    colfld_->setColor( col );
    agestartfld_->setValue( timerg.start );
    agestopfld_->setValue( timerg.stop );
}



void uiStratUnitEditDlg::getFromScreen()
{
    unit_.setCode( unitnmfld_->text() );
    unit_.setDescription( unitdescfld_->text() );
    const char* txt = unitlithfld_->text();
    lithnm_ = !strcmp( txt, sNoLithoTxt ) ? 0 : txt;

    IOPar iop; iop.set( sKey::Color, colfld_->color() );
    Interval<float> rg( agestartfld_->getValue(), agestopfld_->getValue() );
    iop.set( sKey::Time, rg);
    unit_.getFrom( iop );
}


bool uiStratUnitEditDlg::acceptOK( CallBacker* )
{
    if ( agestartfld_->getValue() >= agestopfld_->getValue() )
	{ mErrRet( "Please specify a valid time range", return false ) }
    if ( !strcmp( unitnmfld_->text(), "" ) )
	{ mErrRet( "Please specify the unit name", return false ) }

    BufferString namemsg( "Unit name already used. Please specify a new name" );
    if ( strcmp(unitnmfld_->text(),entrancename_.buf()) )
    {
	if ( !Strat::UnRepo().isNewUnitName(unitnmfld_->text()) )
	    { mErrRet( namemsg, return false ) }
    }

    getFromScreen();

    return true;
}



uiStratLithoDlg::uiStratLithoDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Select Lithology",mNoDlgTitle,"110.0.4"))
    , stratrepos_(Strat::eUnRepo())
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
    stratrepos_.getLithoNames( nms );
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
		    stratrepos_.createNewLith( nm, isporbox_->isChecked() );
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
	    stratrepos_.lithoChanged.trigger();
	}
    }
    const BufferString nm( selfld_->getText() );
    const Strat::Lithology* lith = stratrepos_.getLith( nm );
    if ( !lith ) lith = &Strat::Lithology::undef();
    nmfld_->setText( lith->name() );
    isporbox_->setChecked( lith->porous_ );
    prevlith_ = const_cast<Strat::Lithology*>( lith );
}


void uiStratLithoDlg::renameCB( CallBacker* )
{
    Strat::Lithology* lith = const_cast<Strat::Lithology*>(
			 stratrepos_.getLith( selfld_->getText() ) );
    if ( !lith || lith->isUdf() ) return;

    lith->setName( nmfld_->text() );
    selfld_->setItemText( selfld_->currentItem(), nmfld_->text() );
    stratrepos_.lithoChanged.trigger();
    prevlith_ = lith;
}


void uiStratLithoDlg::rmSel( CallBacker* )
{
    int selidx = selfld_->currentItem();
    if ( selidx < 0 ) return;

    const Strat::Lithology* lith =
		 stratrepos_.getLith( selfld_->textOfItem(selidx) );
    if ( !lith || lith->isUdf() ) return;

    prevlith_ = 0;
    stratrepos_.deleteLith( lith->id_ );
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
    const Strat::Lithology* lith = stratrepos_.getLith( lithnm );
    if ( !lith ) return;
    selfld_->setCurrentItem( lithnm );
}


bool uiStratLithoDlg::acceptOK( CallBacker* )
{
    selChg( 0 );
    return true;
}


uiStratLevelDlg::uiStratLevelDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Create/Edit level",mNoDlgTitle,"110.0.2"))
{
    lvlnmfld_ = new uiGenInput( this, "Name", StringInpSpec() );
    lvlcolfld_ = new uiColorInput( this,
    uiColorInput::Setup(getRandStdDrawColor() ).
    lbltxt("Color") );
    lvlcolfld_->attach( alignedBelow, lvlnmfld_ );
}


void uiStratLevelDlg::setLvlInfo( const char* lvlnm )
{
    Color lvlcol;
    if ( !lvlnm || !*lvlnm || !Strat::UnRepo().getLvlPars(lvlnm,lvlcol) )
	return;

    lvlnmfld_->setText( lvlnm );
    lvlcolfld_->setColor( lvlcol );
    oldlvlnm_ = lvlnm;
}


bool uiStratLevelDlg::acceptOK( CallBacker* )
{
    BufferString newlvlnm = lvlnmfld_->text();
    Color newlvlcol = lvlcolfld_->color();
    Strat::eUnRepo().setLvlPars( oldlvlnm_, newlvlnm, newlvlcol );
    return true;
}





static const char* unitcollbls[] = { "[Name]", "[Color]", 
				     "Start(my)", "Stop(my)", 0 };
static const int cNrEmptyRows = 2;

static const int cNameCol  = 0;
static const int cColorCol = 1;
static const int cStartCol = 2;
static const int cStopCol = 3;

uiStratUnitDivideDlg::uiStratUnitDivideDlg( uiParent* p, 
					    const Strat::UnitRef& unit ) 
    : uiDialog(p,uiDialog::Setup("Subdivide Stratigraphic Unit",
			     "Specify number and properties of the new units",
			     mNoHelpID))
    , parentunit_(unit)
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
	setUnit( 0, parentunit_ );

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
    Interval<float> timerg = parentunit_.timeRange();
    ObjectSet<Strat::UnitRef> units;
    gatherUnits( units );
    const int nrrows = table_->nrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	Strat::UnitRef& unit = *units[idx];
	BufferString bs( unit.code() );
	if ( bs.isEmpty() )
	{
	    BufferString code( "New Unit" );
	    code += idx+1;
	    unit.setCode( code );
	}
	Interval<float> rg;
       	rg.set( timerg.start + (float)idx*timerg.width()/(nrrows),
	        timerg.start + (float)(idx+1)*timerg.width()/(nrrows) );
	unit.setTimeRange( rg );
	table_->setRowReadOnly( idx, false );
	setUnit( idx, unit );
    }
    deepErase( units );
    table_->setCellReadOnly( RowCol( 0, cStartCol ), true );
    table_->setCellReadOnly( RowCol( nrrows-1, cStopCol ), true );
}


void uiStratUnitDivideDlg::setUnit( int irow, const Strat::UnitRef& unit ) 
{
    table_->setText( RowCol(irow,cNameCol), unit.code() );
    table_->setValue( RowCol(irow,cStartCol), unit.timeRange().start );
    table_->setValue( RowCol(irow,cStopCol), unit.timeRange().stop );
    table_->setColor( RowCol(irow,cColorCol), getRandStdDrawColor() );
}


void uiStratUnitDivideDlg::gatherUnits( ObjectSet<Strat::UnitRef>& units ) 
{
    const int nrrows = table_->nrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	const bool isleaf = parentunit_.isLeaf();
	const char* code = table_->text( RowCol(idx,cNameCol) );
	Strat::UnitRef* un = (Strat::UnitRef*)new Strat::LeafUnitRef( 0, code );
	IOPar iop; 
	iop.set( sKey::Color, table_->getColor( RowCol(idx,cColorCol) ) );
	iop.set( sKey::Time, 
		Interval<float>(table_->getfValue( RowCol(idx,cStartCol) ),
			        table_->getfValue( RowCol(idx,cStopCol) ) ) );
	un->getFrom( iop );
	units += un;
    }
}


bool uiStratUnitDivideDlg::areTimesOK( ObjectSet<Strat::UnitRef>& units ) const
{
    if ( units.size() > 1 )
	return true;
    for ( int idx=0; idx<units.size()-1; idx++ )
    {
	const Strat::UnitRef& curunit = *units[idx];
	const Strat::UnitRef& nextunit = *units[idx+1];
	if ( curunit.timeRange().width()<1 || nextunit.timeRange().width()<1 )
	    return false;
	if ( curunit.timeRange().stop > nextunit.timeRange().start )
	    return false;
    }
    return ( units[0]->timeRange().width() >= 1 );
}


bool uiStratUnitDivideDlg::acceptOK( CallBacker* )
{
    BufferStringSet bfs;
    ObjectSet<Strat::UnitRef> units;
    gatherUnits( units );
    for ( int idx=0; idx<units.size(); idx++ )
    {
	BufferString code( units[idx]->code() );
	BufferString errmsg;
	if ( code.isEmpty() )
	{
	    errmsg += "Empty unit name. ";
	}
	if ( errmsg.isEmpty() && strcmp( code.buf(), parentunit_.code() ) )
	{
	    if ( !Strat::UnRepo().isNewUnitName(units[idx]->code()) ) 
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
	    mErrRet( errmsg, deepErase( units); return false )	
	}
    }
    if ( !areTimesOK( units) )
	{ mErrRet( "No valid times specified", deepErase(units); return false; ) }

    deepErase( units );
    return true;
}



uiStratLinkLvlUnitDlg::uiStratLinkLvlUnitDlg( uiParent* p, Strat::UnitRef* ur ) 
    : uiDialog(p,uiDialog::Setup("Link markers and stratigraphic unit boundary",
		mNoDlgTitle,"110.0.3"))
    , lvlid_(-1)		
    , unit_(ur)	 			
{
    BufferStringSet lvlnms;
    lvlnms.add( sNoLevelTxt );
    TypeSet<Color> colors;
    lvlid_ = ur ? ur->getLvlID() : Strat::UnRepo().botLvlID();

    Strat::UnRepo().getLvlsPars( lvlnms, colors, &ids_ );
    BufferString bs = "Select marker";
    lvllistfld_ = new uiGenInput( this, bs, StringListInpSpec( lvlnms ) );
    if ( lvlid_ >=0 )
	lvllistfld_->setValue( ids_.indexOf( lvlid_ ) +1 );
}


bool uiStratLinkLvlUnitDlg::acceptOK( CallBacker* )
{
    const int lvlidx = lvllistfld_->getIntValue()-1;
    lvlid_ = lvlidx >=0 ? ids_[lvlidx] : -1;
    if ( unit_ ) 
	unit_->setLvlID( lvlid_ );
    else
	Strat::eUnRepo().setBotLvlID( lvlid_);

    Strat::eUnRepo().levelChanged.trigger();
    return true;
}
