/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          August 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratutildlgs.cc,v 1.40 2010-10-13 14:52:37 cvsbruno Exp $";

#include "uistratutildlgs.h"

#include "iopar.h"
#include "randcolor.h"
#include "stratlith.h"
#include "stratlevel.h"
#include "stratunitrefiter.h"
#include "stratreftree.h"
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

static const char* sNoLithoTxt      = "--Undefined--";
static const char* sNoLevelTxt      = "--Undefined--";

#define mErrRet(msg,act) uiMSG().error(msg); act;
uiStratUnitEditDlg::uiStratUnitEditDlg( uiParent* p, Strat::NodeUnitRef& unit ) 
    : uiDialog(p,uiDialog::Setup("Stratigraphic Unit Editor",
				 "Edit the unit properties",
				 "110.0.1"))
    , unit_(unit)
    , entrancename_(unit.code())		 
{
    unitnmfld_ = new uiGenInput( this, "Name", StringInpSpec() );
    unitdescfld_ = new uiGenInput( this, "Description", StringInpSpec() );
    unitdescfld_->attach( alignedBelow, unitnmfld_ );

    colfld_ = new uiColorInput( this,
			           uiColorInput::Setup(getRandStdDrawColor() ).
				   lbltxt("Color") );
    colfld_->attach( alignedBelow, unitdescfld_ );

    const Strat::NodeUnitRef* upnode = unit.upNode();
    Interval<float> limitrg = upnode ? upnode->timeRange() : unit.timeRange(); 
    uiLabeledSpinBox* lblbox1 = new uiLabeledSpinBox( this, "Time range (My)" );
    agestartfld_ = lblbox1->box();
    agestartfld_->setInterval( limitrg );
    lblbox1->attach( alignedBelow, colfld_ );
    
    uiLabeledSpinBox* lblbox2 = new uiLabeledSpinBox( this, "" );
    agestopfld_ = lblbox2->box();
    agestopfld_->setInterval( limitrg );
    lblbox2->attach( rightOf, lblbox1 );

    if ( unit_.isLeaved() )
    {
	uiSeparator* sep = new uiSeparator( this, false );
	sep->attach( stretchedBelow, lblbox1 );

	unitlithfld_ = new uiStratLithoBox( this );
	unitlithfld_->setMultiSelect( true );
	unitlithfld_->attach( alignedBelow, lblbox1 );
	unitlithfld_->attach( ensureBelow, sep );

	CallBack cb = mCB(this,uiStratUnitEditDlg,selLithCB);
	uiPushButton* sellithbut = new uiPushButton( this, "&Edit", cb, false );
	sellithbut->attach( rightTo, unitlithfld_ );

	lithids_.erase();
	for ( int idx=0; idx<unit.nrRefs(); idx++ )
	{
	    const Strat::LeafUnitRef& l = (Strat::LeafUnitRef&)(unit.ref(idx));
	    if ( l.lithology() > 0 )
		lithids_ += l.lithology();
	}
	if ( lithids_.size() )
	    unitlithfld_->setSelectedItems( lithids_ );
	else
	    unitlithfld_->setCurrentItem( 0 );
    }

    putToScreen();
}


void uiStratUnitEditDlg::putToScreen()
{
    BufferString code( unit_.code() );
    unitnmfld_->setText( code.isEmpty() ? "<New Unit>" : unit_.code() );
    unitdescfld_->setText( unit_.description() );
    colfld_->setColor( unit_.color() );
    agestartfld_->setValue( unit_.timeRange().start );
    agestopfld_->setValue( unit_.timeRange().stop );
    if ( unit_.isLeaved() )
	unitlithfld_->setSelectedItems( lithids_ );
}


void uiStratUnitEditDlg::getFromScreen()
{
    unit_.setCode( unitnmfld_->text() );
    unit_.setDescription( unitdescfld_->text() );
    unit_.setColor( colfld_->color() );

    Interval<float> rg( agestartfld_->getValue(), agestopfld_->getValue() );
    unit_.setTimeRange( rg );

    lithids_.erase();
    if ( unit_.isLeaved() )
    {
	if ( unitlithfld_->nrSelected() == 0 )
	    unitlithfld_->setCurrentItem( 0 );
	unitlithfld_->getSelectedItems( lithids_ );
    }
}

#define mPreventWrongChar(buf)\
    char* ptr = buf;\
    while ( *ptr )\
    {\
	if ( isspace(*ptr) || (*ptr) == '.' )\
	    *ptr = '_';\
	ptr++;\
    }\

bool uiStratUnitEditDlg::acceptOK( CallBacker* )
{
    getFromScreen();
    BufferString name( unitnmfld_->text() );
    if ( name.isEmpty() )
	{ mErrRet( "Please specify the unit name", return false ) }
    else
    {
	mPreventWrongChar( name.buf() );
    }

    BufferString namemsg( "Unit name already used. Please specify a new name");
    if ( strcmp(name.buf(),entrancename_.buf()) )
    {
	Strat::UnitRefIter it( Strat::RT() );
	while ( it.next() )
	{
	    if ( !strcmp(name.buf(),it.unit()->code()) && it.unit() != &unit_ )
		{ mErrRet( namemsg, return false ) }
	}
    }
    unit_.setCode( name.buf() );

    if ( lithids_.size() > 1 && lithids_.isPresent(0) )
	lithids_.remove( lithids_.indexOf(0) );

    return true;
}


void uiStratUnitEditDlg::selLithCB( CallBacker* )
{
    uiStratLithoDlg lithdlg( this );
    lithdlg.go();
} 



uiStratLithoBox::uiStratLithoBox( uiParent* p )
    : uiListBox( p, "Lithologies",false )
{
    fillLiths( 0 );
    Strat::LithologySet& lithos = Strat::eRT().lithologies();
    lithos.anyChange.notify( mCB( this, uiStratLithoBox, fillLiths ) );
    selectionChanged.notify( mCB( this, uiStratLithoBox, selChanged ) );
}


uiStratLithoBox::~uiStratLithoBox()
{
    Strat::LithologySet& lithos = Strat::eRT().lithologies();
    lithos.anyChange.remove( mCB( this, uiStratLithoBox, fillLiths ) );
}


void uiStratLithoBox::fillLiths( CallBacker* )
{
    empty();

    BufferStringSet nms;
    nms.add( sNoLithoTxt );
    const Strat::LithologySet& lithos = Strat::RT().lithologies();
    for ( int idx=0; idx<lithos.size(); idx++ )
	nms.add( lithos.getLith( idx ).name() );
    addItems( nms );
}
    

void uiStratLithoBox::selChanged( CallBacker* )
{
    NotifyStopper ns( selectionChanged );
    if ( nrSelected() == 0 && size() )
	setSelected( 0, true );
    else if ( nrSelected() > 1 && isSelected( 0 ) )
	setSelected( 0, false );
}



uiStratLithoDlg::uiStratLithoDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Select Lithology",mNoDlgTitle,"110.0.4"))
    , prevlith_(0)
    , nmfld_(0)
{
    selfld_ = new uiStratLithoBox( this );
    const CallBack cb( mCB(this,uiStratLithoDlg,selChg) );
    selfld_->selectionChanged.notify( cb );

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

    uiButton* renamebut = new uiPushButton( rightgrp, "Re&name selected",
			    mCB(this,uiStratLithoDlg,renameCB), true );
    renamebut->attach( alignedBelow, newlithbut );

    uiButton* rmbut = new uiPushButton( rightgrp, "&Remove Last",
				    mCB(this,uiStratLithoDlg,rmLast), true );
    rmbut->attach( alignedBelow, renamebut );

    finaliseDone.notify( cb );
}


void uiStratLithoDlg::newLith( CallBacker* )
{
    BufferString nm( nmfld_->text() );
    if ( nm.isEmpty() ) return;

    mPreventWrongChar( nm.buf() );

    Strat::LithologySet& lithos = Strat::eRT().lithologies();
    if ( selfld_->isPresent( nm ) || lithos.isPresent( nm.buf() ) )
	{ mErrRet( "Please specify a new, unique name", return ) }

    const int lithid = selfld_->size();
    const bool isporous = isporbox_->isChecked();
    Strat::Lithology* newlith = new Strat::Lithology(lithid,nm.buf(),isporous);

    const char* lithfailedmsg = lithos.add( newlith );
    if ( lithfailedmsg )
	{ mErrRet( lithfailedmsg, return; ) } 

    prevlith_ = const_cast<Strat::Lithology*>( &Strat::Lithology::undef() );

    lithos.reportAnyChange();
    selfld_->setCurrentItem( nm );
}


void uiStratLithoDlg::selChg( CallBacker* )
{
    Strat::LithologySet& lithos = Strat::eRT().lithologies();
    if ( !nmfld_ ) return;

    if ( prevlith_ )
    {
	const bool newpor = isporbox_->isChecked();
	if ( newpor != prevlith_->porous() && !prevlith_->isUdf() )
	{
	    prevlith_->porous() = isporbox_->isChecked();
	    lithos.reportAnyChange();
	}
    }
    const BufferString nm( selfld_->getText() );
    const Strat::Lithology* lith = lithos.get( nm );
    if ( !lith ) lith = &Strat::Lithology::undef();
    nmfld_->setText( lith->name() );
    isporbox_->setChecked( lith->porous() );
    prevlith_ = const_cast<Strat::Lithology*>( lith );
}


void uiStratLithoDlg::renameCB( CallBacker* )
{
    Strat::LithologySet& lithos = Strat::eRT().lithologies();
    Strat::Lithology* lith = const_cast<Strat::Lithology*>(
					 lithos.get( selfld_->getText() ) );
    if ( !lith || lith->isUdf() ) return;

    lith->setName( nmfld_->text() );
    selfld_->setItemText( selfld_->currentItem(), nmfld_->text() );
    lithos.reportAnyChange();
    prevlith_ = lith;
}


void uiStratLithoDlg::rmLast( CallBacker* )
{
    int selidx = selfld_->size()-1;
    if ( selidx < 0 ) return;

    Strat::LithologySet& lithos = Strat::eRT().lithologies();
    const Strat::Lithology* lith = lithos.get( selfld_->textOfItem(selidx) );
    lithos.get( selfld_->textOfItem(selidx) );
    if ( !lith || lith->isUdf() ) return;

    prevlith_ = 0;
    delete lithos.lithologies().remove( lithos.indexOf( lith->id() ) );
    lithos.reportAnyChange();

    selfld_->setCurrentItem( selidx-1 );
    selChg( 0 );
}


const char* uiStratLithoDlg::getLithName() const
{
    const char* txt = selfld_->getText();
    return !strcmp( txt, sNoLithoTxt ) ? 0 : txt;
}


void uiStratLithoDlg::setSelectedLith( const char* lithnm )
{
    const Strat::LithologySet& lithos = Strat::RT().lithologies();
    const Strat::Lithology* lith = lithos.get( lithnm );
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


void uiStratLevelDlg::setLvlInfo( const char* lvlnm, const Color& col  )
{
    lvlnmfld_->setText( lvlnm );
    lvlcolfld_->setColor( col );
}


void uiStratLevelDlg::getLvlInfo( BufferString& lvlnm, Color& col ) const
{
    lvlnm = lvlnmfld_->text();
    col = lvlcolfld_->color();
}



static const char* unitcollbls[] = { "[Name]", "[Color]", 
				     "Start(my)", "Stop(my)", 0 };
static const int cNrEmptyRows = 2;

static const int cNameCol  = 0;
static const int cColorCol = 1;
static const int cStartCol = 2;
static const int cStopCol = 3;

uiStratUnitDivideDlg::uiStratUnitDivideDlg( uiParent* p, 
					    const Strat::LeavedUnitRef& unit ) 
    : uiDialog(p,uiDialog::Setup("Subdivide Stratigraphic Unit",
			     "Specify number and properties of the new units",
			     mNoHelpID))
    , rootunit_(unit)
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
	addUnitToTable( 0, rootunit_ );

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
    Interval<float> timerg = rootunit_.timeRange();
    ObjectSet<Strat::LeavedUnitRef> units;
    gatherUnits( units );
    const int nrrows = table_->nrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	Strat::LeavedUnitRef& unit = *units[idx];
	BufferString bs( unit.code() );
	if ( bs.isEmpty() )
	{
	    BufferString code( "<New Unit>" );
	    code += idx+1;
	    unit.setCode( code );
	}
	Interval<float> rg;
       	rg.set( timerg.start + (float)idx*timerg.width()/(nrrows),
	        timerg.start + (float)(idx+1)*timerg.width()/(nrrows) );
	table_->setRowReadOnly( idx, false );
	unit.setTimeRange( rg ); 
	unit.setColor( getRandStdDrawColor() ); 
	addUnitToTable( idx, unit );
    }
    deepErase( units );
    table_->setCellReadOnly( RowCol( 0, cStartCol ), true );
    table_->setCellReadOnly( RowCol( nrrows-1, cStopCol ), true );
}


void uiStratUnitDivideDlg::addUnitToTable( int irow, 
					const Strat::LeavedUnitRef& unit ) 
{
    table_->setText( RowCol(irow,cNameCol), unit.code() );
    table_->setValue( RowCol(irow,cStartCol), unit.timeRange().start );
    table_->setValue( RowCol(irow,cStopCol), unit.timeRange().stop );
    table_->setColor( RowCol(irow,cColorCol), unit.color() );
}


void uiStratUnitDivideDlg::gatherUnits( ObjectSet<Strat::LeavedUnitRef>& units ) 
{
    const int nrrows = table_->nrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	const char* code = table_->text( RowCol(idx,cNameCol) );
	Strat::NodeUnitRef* par = 
	    		const_cast<Strat::NodeUnitRef*>( rootunit_.upNode() );
	Strat::LeavedUnitRef* un = 
	    		new Strat::LeavedUnitRef( par, code );
	un->setColor( table_->getColor( RowCol(idx,cColorCol) ) );
	Interval<float> rg( table_->getfValue( RowCol(idx,cStartCol) ),
			    table_->getfValue( RowCol(idx,cStopCol) )  );
	un->setTimeRange( rg ); 
	units += un;
    }
}


bool uiStratUnitDivideDlg::areTimesOK( 
				ObjectSet<Strat::LeavedUnitRef>& units ) const
{
    for ( int idx=0; idx<units.size()-1; idx++ )
    {
	const Strat::LeavedUnitRef& curunit = *units[idx];
	const Strat::LeavedUnitRef& nextunit = *units[idx+1];
	if ( !curunit.timeRange().width() || !nextunit.timeRange().width() )
	    return false;
	if ( curunit.timeRange().stop > nextunit.timeRange().start )
	    return false;
    }
    return ( units[0]->timeRange().width() >= 0 );
    return false;
}


bool uiStratUnitDivideDlg::acceptOK( CallBacker* )
{
    BufferStringSet bfs;
    ObjectSet<Strat::LeavedUnitRef> units;
    gatherUnits( units );
    for ( int idx=0; idx<units.size(); idx++ )
    {
	BufferString code( units[idx]->code() );
	BufferString errmsg;
	if ( code.isEmpty() )
	{
	    errmsg += "Empty unit name. ";
	}
	else
	{
	    mPreventWrongChar( code.buf() );
	    units[idx]->setCode( code.buf() );
	}
	if ( errmsg.isEmpty() && strcmp( code.buf(), rootunit_.code() ) )
	{
	    Strat::UnitRefIter it( Strat::RT() );
	    while ( it.next() )
	    {
		if ( !strcmp(code.buf(),it.unit()->code()) 
						&& it.unit() != &rootunit_ )
		    errmsg += "Unit name already used. ";
	    }
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
	{ mErrRet("No valid times specified", deepErase(units); return false;) }

    deepErase( units );
    return true;
}



uiStratLinkLvlUnitDlg::uiStratLinkLvlUnitDlg( uiParent* p, 
						Strat::LeavedUnitRef& ur ) 
    : uiDialog(p,uiDialog::Setup("",
		mNoDlgTitle,"110.0.3"))
    , lvlid_(-1)		
    , unit_(ur)	 			
{
    BufferString msg("Link Marker to "); msg += ur.code(); 
    setCaption( msg );
    BufferStringSet lvlnms;
    lvlnms.add( sNoLevelTxt );
    TypeSet<Color> colors;
    lvlid_ = ur.levelID();

    const Strat::LevelSet& lvls = Strat::LVLS();
    for ( int idx=0; idx<lvls.size(); idx++ )
    {
	const Strat::Level& lvl = *lvls.levels()[idx];
	lvlnms.add( lvl.name() );
	colors += lvl.color();
	ids_ += lvl.id();
    }
    BufferString bs = "Select marker";
    lvllistfld_ = new uiGenInput( this, bs, StringListInpSpec( lvlnms ) );
    if ( lvlid_ >=0 )
	lvllistfld_->setValue( ids_.indexOf( lvlid_ ) +1 );
}


bool uiStratLinkLvlUnitDlg::acceptOK( CallBacker* )
{
    const int lvlidx = lvllistfld_->getIntValue()-1;
    lvlid_ = lvlidx >=0 ? ids_[lvlidx] : -1;
    unit_.setLevelID( lvlid_ );

    Strat::eLVLS().levelChanged.trigger();
    return true;
}
