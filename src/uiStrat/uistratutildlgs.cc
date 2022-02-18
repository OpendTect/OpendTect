/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          August 2007
________________________________________________________________________

-*/

#include "uistratutildlgs.h"

#include "iopar.h"
#include "od_helpids.h"
#include "randcolor.h"
#include "stratlevel.h"
#include "stratlith.h"
#include "stratreftree.h"
#include "stratunitrefiter.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uieditobjectlist.h"
#include "uifillpattern.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uistrings.h"
#include "uitoolbutton.h"

static const char* sNoLevelTxt      = "--Undefined--";

#define mErrRet(msg,act) uiMSG().error(msg); act;
uiStratUnitEditDlg::uiStratUnitEditDlg( uiParent* p, Strat::NodeUnitRef& unit )
    : uiDialog(p,uiDialog::Setup(tr("Stratigraphic Unit Editor"),
				 tr("Edit the unit properties"),
				 mODHelpKey(mStratUnitDlgHelpID) ))
    , unit_(unit)
    , entrancename_(unit.code())
{
    unitnmfld_ = new uiGenInput( this, uiStrings::sName(), StringInpSpec() );
    unitdescfld_ = new uiGenInput( this, tr("Description"), StringInpSpec() );
    unitdescfld_->attach( alignedBelow, unitnmfld_ );

    colfld_ = new uiColorInput( this,
		uiColorInput::Setup(getRandStdDrawColor())
				   .lbltxt(uiStrings::sColor()) );
    colfld_->attach( alignedBelow, unitdescfld_ );

    const Strat::NodeUnitRef* upnode = unit.upNode();
    Interval<float> limitrg = upnode ? upnode->timeRange() : unit.timeRange();
    uiLabeledSpinBox* lblbox1 =
			new uiLabeledSpinBox( this, tr("Time range (Ma)") );
    agestartfld_ = lblbox1->box();
    agestartfld_->setNrDecimals( 3 );
    agestartfld_->setInterval( limitrg );
    lblbox1->attach( alignedBelow, colfld_ );

    uiLabeledSpinBox* lblbox2 =
			new uiLabeledSpinBox( this, uiString::emptyString() );
    agestopfld_ = lblbox2->box();
    agestopfld_->setNrDecimals( 3 );
    agestopfld_->setInterval( limitrg );
    lblbox2->attach( rightOf, lblbox1 );

    if ( unit_.isLeaved() )
    {
	uiSeparator* sep = new uiSeparator( this, "HorSep" );
	sep->attach( stretchedBelow, lblbox1 );

	uiListBox::Setup lbsetup( OD::ChooseZeroOrMore, tr("Lithologies") );
	unitlithfld_ = new uiStratLithoBox( this, lbsetup );
	unitlithfld_->attach( alignedBelow, lblbox1 );
	unitlithfld_->attach( ensureBelow, sep );

	const CallBack cb( mCB(this,uiStratUnitEditDlg,selLithCB) );
	uiButton* sellithbut =
		uiButton::getStd( unitlithfld_, OD::Edit, cb, false );
	sellithbut->attach( rightTo, unitlithfld_->box() );

	lithids_.erase();
	TypeSet<int> lithidxs;
	for ( int idx=0; idx<unit.nrRefs(); idx++ )
	{
	    const Strat::LeafUnitRef& lur =
		sCast(Strat::LeafUnitRef&,unit.ref(idx));
	    const int lithoidx =
		Strat::RT().lithologies().indexOf( lur.lithology() );
	    if ( lithoidx >= 0 )
	    {
		lithidxs += lithoidx;
		lithids_ += lur.lithology();
	    }
	}

	if ( lithidxs.size() )
	    unitlithfld_->setChosen( lithidxs );
	else if ( !unitlithfld_->isEmpty() )
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
}


void uiStratUnitEditDlg::getFromScreen()
{
    unit_.setCode( unitnmfld_->text() );
    unit_.setDescription( unitdescfld_->text() );
    unit_.setColor( colfld_->color() );

    Interval<float> rg( agestartfld_->getFValue(), agestopfld_->getFValue() );
    unit_.setTimeRange( rg );

    lithids_.erase();
    if ( !unit_.isLeaved() )
	return;

    TypeSet<int> lithidxs;
    unitlithfld_->getChosen( lithidxs );
    for ( int idx=0; idx<lithidxs.size(); idx++ )
    {
	const int lithidx = lithidxs[idx];
	if ( lithidx<0 || lithidx>=Strat::RT().lithologies().size() )
	    continue;

	lithids_ += Strat::RT().lithologies().getLith(lithidx).id();
    }
}


bool uiStratUnitEditDlg::checkWrongChar(char* buf)
{
    uiString strnm;
    char* ptr = buf;
    while ( *ptr )
    {
	if ( iswspace(*ptr) || (*ptr) == '.' )
	    *ptr = '_';
	if ( (*ptr) == '>' || (*ptr) == '<')
	    strnm = tr("Name contains strange characters !");
	ptr++;
    }
    if ( !strnm.isEmpty() )
    {
	strnm = tr("%1 \n Continue anyway ?").arg(strnm);
	if ( !uiMSG().askContinue( strnm ) )
	   return false;
    }
    return true;
}


bool uiStratUnitEditDlg::acceptOK( CallBacker* )
{
    getFromScreen();
    BufferString unnm( unitnmfld_->text() );
    if ( unnm.isEmpty() || unnm == Strat::RefTree::sKeyNoCode() )
    {
	mErrRet( tr("Please specify a valid unit name"), return false )
    }
    else
    {
	if ( !checkWrongChar(unnm.getCStr()) )
	    return false;
    }

    const char* oldcode = unit_.code();
    unit_.setCode( unnm.buf() );
    if ( unnm != entrancename_ )
    {
	Strat::UnitRefIter it( Strat::RT() );
	while ( it.next() )
	{
	    if ( unit_.fullCode() == it.unit()->fullCode() &&
		 it.unit() != &unit_ )
	    {
		unit_.setCode( oldcode );
		mErrRet( tr("Unit name already used"), return false )
	    }
	}
    }

    if ( unit_.isLeaved() && lithids_.size() <= 0 )
    {
	mErrRet( tr("Please specify at least one lithology"),
		 if ( !unitlithfld_->size() )
		     selLithCB( 0 );
		 return false; );
    }

    return true;
}


void uiStratUnitEditDlg::selLithCB( CallBacker* )
{
    uiStratLithoDlg lithdlg( this );
    lithdlg.go();
}


// uiStratLithoBox
uiStratLithoBox::uiStratLithoBox( uiParent* p )
    : uiListBox(p,"Lithologies")
{
    box()->setHSzPol( uiObject::MedVar );
    fillLiths( nullptr );
    Strat::LithologySet& lithos = Strat::eRT().lithologies();
    mAttachCB( lithos.anyChange, uiStratLithoBox::fillLiths );
}


uiStratLithoBox::uiStratLithoBox( uiParent* p, const uiListBox::Setup& setup )
    : uiListBox(p,setup,"Lithologies")
{
    box()->setHSzPol( uiObject::MedVar );
    fillLiths( nullptr );
    Strat::LithologySet& lithos = Strat::eRT().lithologies();
    mAttachCB( lithos.anyChange, uiStratLithoBox::fillLiths );
}


uiStratLithoBox::~uiStratLithoBox()
{
    detachAllNotifiers();
}


void uiStratLithoBox::fillLiths( CallBacker* )
{
    BufferStringSet selected;
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( isChosen(idx) )
	    selected.add( textOfItem(idx) );
    }

    selectionChanged.disable();
    setEmpty();
    const Strat::LithologySet& lithos = Strat::RT().lithologies();
    for ( int idx=0; idx<lithos.size(); idx++ )
    {
	const Strat::Lithology& litho = lithos.getLith( idx );
	addItem( toUiString(litho.name()), litho.color() );
    }

    bool dotrigger = false; int firstsel = -1;
    for ( int idx=0; idx<selected.size(); idx++ )
    {
	const int selidx = indexOf( selected.get(idx) );
	if ( selidx < 0 )
	    dotrigger = true;
	else
	{
	    setChosen( selidx, true );
	    firstsel = selidx;
	}
    }

    if ( firstsel < 0 )
    {
	dotrigger = true;
	if ( !isEmpty() )
	    firstsel = 0;
    }
    setCurrentItem( firstsel );
    selectionChanged.enable();
    if ( dotrigger )
	selectionChanged.trigger();
}


// uiStratLithoDlg
uiStratLithoDlg::uiStratLithoDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(
	     uiStrings::phrManage( uiStrings::sLithology(mPlural)),mNoDlgTitle,
	    mODHelpKey(mStratLithoDlgHelpID) ))
    , prevlith_(nullptr)
    , anychg_(false)
{
    setCtrlStyle( CloseOnly );

    selfld_ = new uiStratLithoBox( this );
    const CallBack selchgcb( mCB(this,uiStratLithoDlg,selChg) );
    selfld_->selectionChanged.notify( selchgcb );

    uiButtonGroup* butgrp = new uiButtonGroup( this, "Buttons", OD::Vertical );
    butgrp->attach( rightTo, selfld_ );
    new uiToolButton( butgrp, "addnew", tr("Add new Lithology"),
		      mCB(this,uiStratLithoDlg,newLith) );
    new uiToolButton( butgrp, "delete", tr("Remove selected Lithology"),
		      mCB(this,uiStratLithoDlg,rmLast) );

    const CallBack propchgcb( mCB(this,uiStratLithoDlg,propChg) );
    uiGroup* rightgrp = new uiGroup( this, "right group" );

    nmfld_ = new uiGenInput( rightgrp, uiStrings::sName(), StringInpSpec() );
    nmfld_->updateRequested.notify( mCB(this,uiStratLithoDlg,renameCB) );
    uiPushButton* applybut = new uiPushButton( rightgrp, uiStrings::sRename(),
		      mCB(this,uiStratLithoDlg,renameCB), true );
    applybut->attach( rightTo, nmfld_ );

    isporbox_ = new uiCheckBox( rightgrp, tr("Porous") );
    isporbox_->activated.notify( propchgcb );
    isporbox_->attach( alignedBelow, nmfld_ );

    uiColorInput::Setup csu( OD::Color::White() );
    csu.dlgtitle( tr("Default color for this lithology") );
    colfld_ = new uiColorInput( rightgrp, csu );
    colfld_->attach( alignedBelow, isporbox_ );
    colfld_->colorChanged.notify( propchgcb );

    uiSeparator* sep = new uiSeparator( this, "Sep", OD::Vertical );
    sep->attach( rightTo, butgrp );
    sep->attach( heightSameAs, selfld_ );
    rightgrp->attach( rightTo, sep );

    postFinalise().notify( selchgcb );
}


void uiStratLithoDlg::newLith( CallBacker* )
{
    BufferString nm = "<New Lithology>";

    Strat::LithologySet& lithos = Strat::eRT().lithologies();
    const int lithid = lithos.getFreeID();
    const bool isporous = false;
    Strat::Lithology* newlith = new Strat::Lithology(lithid,nm.buf(),isporous);
    newlith->color() = getRandStdDrawColor();

    const char* lithfailedmsg = lithos.add( newlith );
    if ( lithfailedmsg )
	{ mErrRet( toUiString(lithfailedmsg), return; ) }

    anychg_ = true;
    prevlith_ = nullptr;
    lithos.reportAnyChange();
    selfld_->setCurrentItem( nm );
}


void uiStratLithoDlg::propChg( CallBacker* )
{
    if ( !prevlith_ || prevlith_->isUdf() )
	return;

    Strat::LithologySet& lithos = Strat::eRT().lithologies();
    const bool newpor = isporbox_->isChecked();
    const OD::Color newcol = colfld_->color();
    if ( (newpor != prevlith_->porous() || newcol != prevlith_->color()) )
    {
	prevlith_->porous() = newpor;
	prevlith_->color() = newcol;
	lithos.reportAnyChange();
	anychg_ = true;
    }
}


void uiStratLithoDlg::selChg( CallBacker* )
{
    Strat::LithologySet& lithos = Strat::eRT().lithologies();
    const BufferString nm( selfld_->getText() );
    const Strat::Lithology* lith = lithos.get( nm );
    if ( !lith )
	return; // can only happen when no lithologies defined at all

    NotifyStopper nspor( isporbox_->activated );
    NotifyStopper nscol( colfld_->colorChanged );
    nmfld_->setText( lith->name() );
    isporbox_->setChecked( lith->porous() );
    colfld_->setColor( lith->color() );
    prevlith_ = const_cast<Strat::Lithology*>( lith );
}


void uiStratLithoDlg::renameCB( CallBacker* )
{
    Strat::LithologySet& lithos = Strat::eRT().lithologies();
    Strat::Lithology* lith = const_cast<Strat::Lithology*>(
					 lithos.get( selfld_->getText() ) );
    if ( !lith || lith->isUdf() ) return;

    BufferString nm = nmfld_->text();
    if ( nm == selfld_->getText() )
	return;

    if ( !uiStratUnitEditDlg::checkWrongChar(nm.getCStr()) )
	return;

    if ( selfld_->isPresent(nm) || lithos.isPresent(nm.buf()) )
    {
	mErrRet( tr("Please specify a new, unique name"), return )
    }

    lith->setName( nm );
    selfld_->setItemText( selfld_->currentItem(), toUiString(nm) );
    lithos.reportAnyChange();
    prevlith_ = lith;
    anychg_ = true;
}


void uiStratLithoDlg::rmLast( CallBacker* )
{
    // removes selected, not the last one
    if ( selfld_->size() <= 1 )
	return; // No need to ever delete the last lithology

    const int selidx = selfld_->currentItem();
    if ( selidx < 0 )
	return;

    Strat::LithologySet& lithos = Strat::eRT().lithologies();
    const Strat::Lithology* lith = lithos.get( selfld_->textOfItem(selidx) );
    if ( !lith || lith->isUdf() )
	return;

    uiString msg = tr("Lithology '%1' will be removed. "
		      "Do you want to continue?").arg( lith->name() );
    const bool res = uiMSG().askContinue( msg );
    if ( !res )
	return;

    const int lithoidx = lithos.indexOf( lith->id() );
    if ( lithos.lithologies().validIdx(lithoidx) )
	delete lithos.lithologies().removeSingle( lithoidx );
    lithos.reportAnyChange();

    prevlith_ = nullptr;
    selfld_->setCurrentItem( selidx-1 );
    selChg( nullptr );
    anychg_ = true;
}


const char* uiStratLithoDlg::getLithName() const
{
    return selfld_->getText();
}


void uiStratLithoDlg::setSelectedLith( const char* lithnm )
{
    const Strat::LithologySet& lithos = Strat::RT().lithologies();
    const Strat::Lithology* lith = lithos.get( lithnm );
    if ( !lith ) return;
    selfld_->setCurrentItem( lithnm );
}


class uiStratSingleContentDlg : public uiDialog
{ mODTextTranslationClass(uiStratSingleContentDlg);
public:

uiStratSingleContentDlg( uiParent* p, Strat::Content& c, bool isadd, bool& chg)
    : uiDialog(p,uiDialog::Setup(isadd ? tr("Add content") : tr("Edit Content"),
		isadd ? tr("Add content") : tr("Edit content properties"),
				  mODHelpKey(mStratContentsDlgHelpID) ))
    , cont_(c)
    , anychg_(chg)
{
    nmfld_ = new uiGenInput( this, uiStrings::sName(), StringInpSpec(c.name()));

    fillfld_ = new uiFillPattern( this );
    fillfld_->set( cont_.pattern_ );
    new uiLabel( this, tr("Pattern"), fillfld_ );
    fillfld_->attach( alignedBelow, nmfld_ );

    uiColorInput::Setup su( cont_.color_ );
    su.lbltxt( tr("Outline color") );
    colfld_ = new uiColorInput( this, uiColorInput::Setup(cont_.color_) );
    colfld_->attach( alignedBelow, fillfld_ );
    new uiLabel( this, su.lbltxt_, colfld_ );
}

bool acceptOK( CallBacker* )
{
    BufferString nm( nmfld_->text() );
    nm.clean( BufferString::NoSpecialChars );
    if ( nm.isEmpty() )
    {
	uiMSG().error( uiStrings::sEnterValidName() );
	return false;
    }
    cont_.setName( nm );
    cont_.pattern_ = fillfld_->get();
    cont_.color_ = colfld_->color();
    anychg_ = true;
    return true;
}

    Strat::Content&	cont_;
    uiGenInput*		nmfld_;
    uiFillPattern*	fillfld_;
    uiColorInput*	colfld_;
    bool&		anychg_;

};


class uiStratContentsEd : public uiEditObjectList
{ mODTextTranslationClass(uiStratContentsEd);
public:

uiStratContentsEd( uiParent* p, bool& chg )
    : uiEditObjectList(p,"content",true)
    , anychg_(chg)
{
    fillList( 0 );
}

void fillList( int newcur )
{
    const Strat::ContentSet& conts = Strat::RT().contents();
    BufferStringSet nms;
    for ( int idx=0; idx<conts.size(); idx++ )
	nms.add( conts[idx]->name() );
    setItems( nms, newcur );
}

void editReq( bool isadd )
{
    Strat::ContentSet& conts = Strat::eRT().contents();
    Strat::Content* newcont = isadd ? new Strat::Content( "" ) : 0;
    Strat::Content* cont = newcont;
    int selidx = currentItem();
    if ( cont )
	selidx++;
    else
    {
	if ( selidx < 0 ) return;
	cont = conts[selidx];
    }
    uiStratSingleContentDlg dlg( this, *cont, isadd, anychg_ );
    if ( !dlg.go() )
	delete newcont;
    else
    {
	if ( newcont )
	{
	    anychg_ = true;
	    conts += newcont;
	}
	fillList( selidx );
    }
}

void removeReq()
{
    Strat::ContentSet& conts = Strat::eRT().contents();
    const int selidx = currentItem();
    if ( selidx < 0 ) return;

    anychg_ = true;
    delete conts.removeSingle( selidx );
    fillList( selidx );
}

void itemSwitch( bool up )
{
    Strat::ContentSet& conts = Strat::eRT().contents();
    const int selidx = currentItem();
    const int newselidx = up ? selidx-1 : selidx+1;
    if ( selidx < 0 || newselidx < 0 || newselidx > conts.size() - 1 )
	return;

    anychg_ = true;
    conts.swap( selidx, newselidx );
    fillList( newselidx );
}

    bool&	anychg_;

};


uiStratContentsDlg::uiStratContentsDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrManage( tr("Contents")),
		tr("Define special layer contents"),
		mODHelpKey(mStratContentsDlgHelpID) ))
    , anychg_(false)
{
    setCtrlStyle( CloseOnly );
    (void)new uiStratContentsEd( this, anychg_ );
}


uiStratLevelDlg::uiStratLevelDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Create/Edit level"),mNoDlgTitle,
				 mODHelpKey(mStratLevelDlgHelpID) ))
{
    lvlnmfld_ = new uiGenInput( this, uiStrings::sName(), StringInpSpec() );
    lvlcolfld_ = new uiColorInput( this,
				uiColorInput::Setup(getRandStdDrawColor() ).
				lbltxt(uiStrings::sColor()) );
    lvlcolfld_->attach( alignedBelow, lvlnmfld_ );
}


void uiStratLevelDlg::setLvlInfo( const char* lvlnm, const OD::Color& col  )
{
    lvlnmfld_->setText( lvlnm );
    lvlcolfld_->setColor( col );
}


void uiStratLevelDlg::getLvlInfo( BufferString& lvlnm, OD::Color& col ) const
{
    lvlnm = lvlnmfld_->text();
    col = lvlcolfld_->color();
}



static const int cNrEmptyRows = 2;

static const int cNameCol  = 0;
static const int cColorCol = 1;
static const int cStartCol = 2;
static const int cStopCol = 3;

void uiStratUnitDivideDlg::uiDivideTable::popupMenu( CallBacker* cb )
{
    if ( currentRow() > 0 )
	uiTable::popupMenu( cb );
}


uiStratUnitDivideDlg::uiStratUnitDivideDlg( uiParent* p,
					    const Strat::LeavedUnitRef& unit )
    : uiDialog(p,Setup(tr("Subdivide Stratigraphic Unit"),
			tr("Specify number and properties of the new units"),
			mODHelpKey(mStratUnitDivideDlgHelpID)))
    , rootunit_(unit)
{
    table_ = new uiDivideTable( this, uiTable::Setup()
						.rowdesc(uiStrings::sUnit())
						.rowgrow(true)
					        .defrowlbl("")
						.selmode(uiTable::Multi));
    uiStringSet collbls;
    collbls.add( uiStrings::sName() )
           .add( uiStrings::sColor() )
           .add( uiStrings::sStart().withUnit("my") )
           .add( uiStrings::sStop().withUnit("my") );
    table_->setColumnLabels( collbls );
    table_->setColumnReadOnly( cColorCol, true );
    table_->setColumnResizeMode( uiTable::ResizeToContents );
    table_->setNrRows( cNrEmptyRows );
    table_->leftClicked.notify( mCB(this,uiStratUnitDivideDlg,mouseClick) );
    table_->rowInserted.notify( mCB(this,uiStratUnitDivideDlg,resetUnits) );
    table_->rowDeleted.notify( mCB(this,uiStratUnitDivideDlg,resetUnits) );
    table_->selectionDeleted.notify(mCB(this,uiStratUnitDivideDlg,resetUnits));
    table_->setMinimumWidth( 450 );

    if ( table_->nrRows() )
	addUnitToTable( 0, rootunit_ );

    resetUnits( 0 );
}


void uiStratUnitDivideDlg::mouseClick( CallBacker* )
{
    RowCol rc = table_->notifiedCell();
    if ( rc.col() != cColorCol || table_->isCellReadOnly(rc) ) return;

    OD::Color newcol = table_->getColor( rc );
    if ( selectColor(newcol,this,tr("Unit color")) )
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
	unit.setColor( unit.color() );
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


void uiStratUnitDivideDlg::gatherUnits( ObjectSet<Strat::LeavedUnitRef>& units)
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
	Interval<float> rg( table_->getFValue( RowCol(idx,cStartCol) ),
			    table_->getFValue( RowCol(idx,cStopCol) )  );
	un->setTimeRange( rg );
	units += un;
    }
}


bool uiStratUnitDivideDlg::areTimesOK( ObjectSet<Strat::LeavedUnitRef>& units,
				       uiString& errmsg ) const
{
    for ( int idx=0; idx<units.size()-1; idx++ )
    {
	const Strat::LeavedUnitRef& curunit = *units[idx];
	const Strat::LeavedUnitRef& nextunit = *units[idx+1];
	const bool iscurunitrgok = curunit.timeRange().width();
	if ( !iscurunitrgok || !nextunit.timeRange().width() )
	{
	    errmsg = tr("Time-span for unit '%1' is equal "
			"to 0. \nPlease correct it.")
		   .arg(!iscurunitrgok ? curunit.code() : nextunit.code());
	    return false;
	}
	if ( curunit.timeRange().stop > nextunit.timeRange().start )
	{
	    errmsg = tr("Time overlap detected between units "
			"'%1' and '%2'.\nPlease correct it.")
		   .arg(curunit.code())
		   .arg(nextunit.code());
	    return false;
	}
    }
    return ( units[0]->timeRange().width() >= 0 );
}


bool uiStratUnitDivideDlg::acceptOK( CallBacker* )
{
    BufferStringSet bfs;
    ObjectSet<Strat::LeavedUnitRef> units;
    gatherUnits( units );
    if ( !units.size() )
	{ mErrRet( tr("No valid unit present in the table "), return false ); }

    for ( int idx=0; idx<units.size(); idx++ )
    {
	BufferString code( units[idx]->code() );
	const BufferString fullcode = units[idx]->code();
	uiString errmsg;
	if ( code.isEmpty() )
	    errmsg = tr("Empty unit name. ");
	else
	{
	    if(!uiStratUnitEditDlg::checkWrongChar(code.getCStr()))
		return false;
	    units[idx]->setCode( code.buf() );
	}
	if ( errmsg.isEmpty() && code == rootunit_.code() )
	{
	    Strat::UnitRefIter it( Strat::RT() );
	    while ( it.next() )
	    {
		if ( fullcode == it.unit()->fullCode().buf()
			&& it.unit() != &rootunit_ )
		    errmsg = tr("Unit name already used. ");
	    }
	}
	bfs.addIfNew( code );
	if ( errmsg.isEmpty() && bfs.size() < idx+1 )
	     errmsg.append(tr("Unit name previously used in the list. "));
	if ( !errmsg.isEmpty() )
	{
	    errmsg.append(tr("Please specify a new name for the unit number %1")
			.arg(idx+1));
	    mErrRet( errmsg, deepErase( units); return false )
	}
    }
    uiString errmsg;
    if ( !areTimesOK( units, errmsg ) )
    {
	if ( errmsg.isEmpty() )
	    errmsg = tr("No valid times specified");

	mErrRet( errmsg, deepErase(units); return false;) }

    deepErase( units );
    return true;
}



uiStratLinkLvlUnitDlg::uiStratLinkLvlUnitDlg( uiParent* p,
						Strat::LeavedUnitRef& ur )
    : uiDialog(p,uiDialog::Setup(uiString::emptyString(),
		mNoDlgTitle, mODHelpKey(mStratLinkLvlUnitDlgHelpID) ))
    , lvlid_(-1)
    , unit_(ur)
{
    uiString msg = tr("Link Marker to %1").arg(ur.code());
    setCaption( msg );
    BufferStringSet lvlnms;
    lvlnms.add( sNoLevelTxt );
    TypeSet<OD::Color> colors;
    lvlid_ = ur.levelID();

    const Strat::LevelSet& lvls = Strat::LVLS();
    for ( int idx=0; idx<lvls.size(); idx++ )
    {
	const Strat::Level& lvl = *lvls.levels()[idx];
	lvlnms.add( lvl.name() );
	colors += lvl.color();
	ids_ += lvl.id();
    }
    uiString bs = tr("Select marker");
    lvllistfld_ = new uiGenInput( this, bs, StringListInpSpec( lvlnms ) );
    if ( lvlid_ >=0 )
	lvllistfld_->setValue( ids_.indexOf( lvlid_ ) +1 );
}


bool uiStratLinkLvlUnitDlg::acceptOK( CallBacker* )
{
    const int lvlidx = lvllistfld_->getIntValue()-1;
    lvlid_ = lvlidx >=0 ? ids_[lvlidx] : -1;

    Strat::RefTree& rt = Strat::eRT();
    Strat::LeavedUnitRef* lur = rt.getByLevel( lvlid_ );

    if ( lur && lvlid_>=0 )
    {
	uiString msg = tr( "This marker is already linked to %1" )
		     .arg(lur->code());
	uiString movemsg = tr( "Assign to %1 only" )
			 .arg(unit_.code());
	const int res =
	    uiMSG().question(msg,tr("Assign to both"), movemsg ,
			     uiStrings::sCancel());
	if ( res == -1 )
	    return false;
	if ( res == 0 )
	    lur->setLevelID( -1 );
    }

    unit_.setLevelID( lvlid_ );
    Strat::eLVLS().levelChanged.trigger();
    return true;
}
