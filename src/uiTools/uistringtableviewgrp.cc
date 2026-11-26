/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistringtableviewgrp.h"

#include "uicheckselgrp.h"
#include "uigeninput.h"
#include "uistrings.h"

#include <QEvent>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QObject>
#include <QWidget>

mUseQtnamespace

static const int cIconSz = 16;


class StringTableCheckKeyFilter : public QObject
{
public:
StringTableCheckKeyFilter( uiCheckSelGrp& grp, QObject* parent )
    : QObject(parent)
    , grp_(grp)
{}

protected:
bool eventFilter( QObject* obj, QEvent* ev ) override
{
    if ( !ev || ev->type()!=QEvent::KeyPress )
	return QObject::eventFilter( obj, ev );

    auto* qkeyev = static_cast<QKeyEvent*>( ev );
    const bool ctrl = qkeyev->modifiers() == Qt::ControlModifier;
    if ( !grp_.handleShortcut(OD::KeyboardKey(qkeyev->key()),ctrl) )
	return QObject::eventFilter( obj, ev );

    qkeyev->accept();
    return true;
}

    uiCheckSelGrp&	grp_;
};


uiStringTableViewGrp::Setup::Setup()
{}


uiStringTableViewGrp::Setup::~Setup()
{}


uiStringTableViewGrp::uiStringTableViewGrp( uiParent* p, TableModel* mdl,
					   const Setup& setup,
					   const char* nm )
    : uiGroup(p,nm && *nm ? nm : "String table")
    , selectionChanged(this)
    , filtergrp_(nullptr)
    , view_(nullptr)
    , model_(mdl)
    , setup_(setup)
    , filterfld_(nullptr)
    , checkselgrp_(nullptr)
{
    filtergrp_ = new uiGroup( this, "Filter group" );
    filterfld_ = new uiGenInput( filtergrp_, uiStrings::sFilter() );
    filterfld_->setElemSzPol( uiObject::SmallVar );
    mAttachCB( filterfld_->valueChanged, uiStringTableViewGrp::filterCB );

    const char* viewnm = nm && *nm ? nm : "String table";
    view_ = new uiTableView( filtergrp_, viewnm );
    view_->setSelectionBehavior( uiTableView::SelectRows );
    const bool usechecks = hasCheckBoxes();
    if ( model_ && usechecks )
	model_->setRowCheckable( true );

    view_->setSelectionMode( setup_.multiselection_ && !usechecks
				? uiTableView::ExtendedSelection
				: uiTableView::SingleSelection );
    view_->setHeaderVisible( OD::Vertical, setup_.showverticalheader_ );
    view_->setAlternatingRowColors( setup_.alternatingrowcolors_ );
    view_->setSortingEnabled( setup_.sorting_ );
    view_->setHSzPol( uiObject::WideMax );
    view_->setStretch( 2, 2 );
    if ( model_ )
	view_->setModel( model_ );

    mAttachCB( view_->selectionChanged,
	       uiStringTableViewGrp::selectionChangedCB );
    if ( usechecks && model_ )
	mAttachCB( model_->checkStateChanged,
		   uiStringTableViewGrp::checkStateCB );

    mAttachCB( postFinalize(), uiStringTableViewGrp::finalizeCB );
    if ( setup_.compactrows_ )
	view_->setRowHeightPolicy( uiTableView::Compact );

    if ( usechecks )
    {
	checkselgrp_ = new uiCheckSelGrp( filtergrp_, "Check selection" );
	checkselgrp_->attach( alignedAbove, view_ );
	mAttachCB( checkselgrp_->beforeShowMenu,
		   uiStringTableViewGrp::aboutToShowCheckMenuCB );
	mAttachCB( checkselgrp_->checkAllClicked,
		   uiStringTableViewGrp::chooseAllReqCB );
	mAttachCB( checkselgrp_->invertSelection,
		   uiStringTableViewGrp::invertReqCB );
	mAttachCB( checkselgrp_->showCheckedOnly,
		   uiStringTableViewGrp::showCheckedOnlyReqCB );
	mAttachCB( view_->rightClicked, uiStringTableViewGrp::menuCB );
	auto* keyfilt = new StringTableCheckKeyFilter( *checkselgrp_,
							view_->qwidget() );
	view_->qwidget()->installEventFilter( keyfilt );
	updateSelButtons();
    }

    view_->attach( rightAlignedBelow, filterfld_->attachObj() );
    filtergrp_->setHAlignObj( view_ );
    setHAlignObj( view_ );
}


uiObject* uiStringTableViewGrp::selectionGroupObj()
{
    return checkselgrp_ ? checkselgrp_->attachObj() : nullptr;
}


uiStringTableViewGrp::~uiStringTableViewGrp()
{
    detachAllNotifiers();
    if ( view_ )
	view_->setModel( nullptr );
}


bool uiStringTableViewGrp::hasCheckBoxes() const
{
    return setup_.multiselection_ && setup_.checkboxes_;
}


int uiStringTableViewGrp::selectedRow() const
{
    TypeSet<int> rows;
    getSelectedRows( rows );
    return rows.isEmpty() ? -1 : rows[0];
}


void uiStringTableViewGrp::getSelectedRows( TypeSet<int>& rows ) const
{
    rows.setEmpty();
    if ( view_ )
	view_->getSelectedRows( rows, true );
}


bool uiStringTableViewGrp::isRowChecked( int row ) const
{
    return model_ && model_->isChecked( row, 0 )==1;
}


void uiStringTableViewGrp::setRowChecked( int row, bool yn )
{
    if ( model_ )
	model_->setChecked( row, 0, yn ? 1 : 0 );
}


void uiStringTableViewGrp::getCheckedRows( TypeSet<int>& rows ) const
{
    rows.setEmpty();
    if ( !model_ )
	return;

    for ( int idx=0; idx<model_->nrRows(); idx++ )
    {
	if ( model_->isChecked(idx,0)==1 )
	    rows += idx;
    }
}


int uiStringTableViewGrp::nrChosen() const
{
    if ( !hasCheckBoxes() )
    {
	TypeSet<int> rows;
	getSelectedRows( rows );
	if ( !setup_.multiselection_ )
	    return rows.isEmpty() ? 0 : 1;

	return rows.size();
    }

    TypeSet<int> checked;
    getCheckedRows( checked );
    if ( checked.isEmpty() && setup_.choicemode_==OD::ChooseAtLeastOne )
	return selectedRow()<0 ? 0 : 1;

    return checked.size();
}


bool uiStringTableViewGrp::isChosen( int row ) const
{
    if ( !model_ || row<0 || row>=model_->nrRows() )
	return false;

    if ( !hasCheckBoxes() )
    {
	TypeSet<int> rows;
	getSelectedRows( rows );
	return rows.isPresent( row );
    }

    if ( isRowChecked(row) )
	return true;

    if ( setup_.choicemode_!=OD::ChooseAtLeastOne ||
	 row!=selectedRow() )
	return false;

    TypeSet<int> checked;
    getCheckedRows( checked );
    return checked.isEmpty();
}


void uiStringTableViewGrp::setChosen( int row, bool yn )
{
    if ( hasCheckBoxes() )
    {
	setRowChecked( row, yn );
	if ( yn && view_ )
	    view_->setCellSelected( RowCol(row,0), true, true );

	return;
    }

    if ( view_ )
	view_->setCellSelected( RowCol(row,0), yn, true );
}


void uiStringTableViewGrp::setCheckedRows( const TypeSet<int>& rows )
{
    if ( !model_ )
	return;

    for ( int idx=0; idx<model_->nrRows(); idx++ )
    {
	const int cur = model_->isChecked( idx, 0 );
	if ( cur < 0 )
	    continue;

	const bool want = rows.isPresent( idx );
	if ( (cur==1) != want )
	    model_->setChecked( idx, 0, want ? 1 : 0 );
    }
}


void uiStringTableViewGrp::offerReadWriteSelection( const CallBack& readcb,
						   const CallBack& writecb )
{
    opencb_ = readcb;
    savecb_ = writecb;
    if ( checkselgrp_ )
    {
	checkselgrp_->offerReadWriteSelection(
		opencb_.willCall() ? mCB(this,uiStringTableViewGrp,readSelCB)
				   : CallBack(),
		savecb_.willCall() ? mCB(this,uiStringTableViewGrp,saveSelCB)
				   : CallBack() );
    }
}


void uiStringTableViewGrp::filterCB( CallBacker* )
{
    if ( view_ && filterfld_ )
	view_->setRowTextFilter( filterfld_->text() );
}


void uiStringTableViewGrp::finalizeCB( CallBacker* )
{
    if ( !view_ || !model_ )
	return;

    const int nrcols = model_->nrCols();
    if ( nrcols < 1 )
	return;

    view_->setColumnResizeMode( uiTableView::ResizeToContents );
    view_->resizeColumnsToContents();

    TypeSet<int> widths;
    int others = 0;
    for ( int col=0; col<nrcols; col++ )
    {
	widths += view_->columnWidth( col );
	if ( col > 0 )
	    others += widths[col];
    }

    int namew = widths[0];
    int linespace = 0;
    if ( const QWidget* widget = view_->qwidget() )
    {
	const QFontMetrics fm( widget->font() );
	linespace = fm.lineSpacing();
	const int slack = cIconSz * 2 + linespace;
	const int minname = fm.horizontalAdvance( "Name" ) + slack;
	const int maxname =
		fm.horizontalAdvance( QString(32,QChar('n')) ) + slack;
	namew = mMAX( minname, mMIN(namew,maxname) );
    }

    widths[0] = namew;
    view_->setColumnResizeMode( uiTableView::Interactive );
    for ( int col=0; col<nrcols; col++ )
	view_->setColumnWidth( col, widths[col] );

    view_->setPrefWidth( namew + others + linespace );
    view_->setColumnStretchable( 0, true );
}


void uiStringTableViewGrp::readSelCB( CallBacker* )
{
    if ( opencb_.willCall() )
	opencb_.doCall( this );
}


void uiStringTableViewGrp::saveSelCB( CallBacker* )
{
    if ( savecb_.willCall() )
	savecb_.doCall( this );
}


void uiStringTableViewGrp::selectionChangedCB( CallBacker* )
{
    updateSelButtons();
    selectionChanged.trigger();
}


void uiStringTableViewGrp::checkStateCB( CallBacker* )
{
    updateSelButtons();
    if ( checkselgrp_ && !checkselgrp_->allShown() && view_ )
	view_->setShowCheckedOnly( true );

    selectionChanged.trigger();
}


void uiStringTableViewGrp::updateSelButtons()
{
    if ( !checkselgrp_ )
	return;

    TypeSet<int> rows;
    getCheckedRows( rows );
    const int nritems = model_ ? model_->nrRows() : 0;
    checkselgrp_->updateState( rows.size(), nritems );
}


void uiStringTableViewGrp::menuCB( CallBacker* )
{
    if ( checkselgrp_ )
	checkselgrp_->showMenu();
}


void uiStringTableViewGrp::aboutToShowCheckMenuCB( CallBacker* )
{
    updateSelButtons();
}


static void restoreRow( uiTableView* view, int row )
{
    if ( view && row>=0 )
	view->setCellSelected( RowCol(row,0), true, true );
}


void uiStringTableViewGrp::chooseAllReqCB( CallBacker* cb )
{
    mCBCapsuleUnpack( bool, yn, cb );
    const int cur = selectedRow();
    TypeSet<int> rows;
    if ( yn && model_ )
    {
	for ( int idx=0; idx<model_->nrRows(); idx++ )
	    rows += idx;
    }

    setCheckedRows( rows );
    restoreRow( view_, cur );
    updateSelButtons();
}


void uiStringTableViewGrp::invertReqCB( CallBacker* )
{
    const int cur = selectedRow();
    TypeSet<int> checked;
    getCheckedRows( checked );
    TypeSet<int> flipped;
    if ( model_ )
    {
	for ( int idx=0; idx<model_->nrRows(); idx++ )
	{
	    if ( model_->isChecked(idx,0) < 0 )
		continue;
	    if ( !checked.isPresent(idx) )
		flipped += idx;
	}
    }

    setCheckedRows( flipped );
    restoreRow( view_, cur );
    updateSelButtons();
}


void uiStringTableViewGrp::showCheckedOnlyReqCB( CallBacker* cb )
{
    mCBCapsuleUnpack( bool, onlychecked, cb );
    const int cur = selectedRow();
    if ( view_ )
	view_->setShowCheckedOnly( onlychecked );

    restoreRow( view_, cur );
    updateSelButtons();
}
