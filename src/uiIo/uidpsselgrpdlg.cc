/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          June 2011
________________________________________________________________________

-*/

#include "uidpsselgrpdlg.h"

#include "uicolor.h"
#include "uidatapointsetcrossplot.h"
#include "uiimpexpselgrp.h"
#include "uimsg.h"
#include "uitable.h"
#include "uitoolbutton.h"

#include "randcolor.h"
#include "od_helpids.h"


uiDPSSelGrpDlg::uiDPSSelGrpDlg( uiDataPointSetCrossPlotter& p,
				const BufferStringSet& colnames )
    : uiDialog( p.parent(), uiDialog::Setup(tr("Selection Settings"),
				     uiString::empty(),
				     mODHelpKey(mSelectionSettDlgHelpID) )
				    .savebutton(!p.isADensityPlot())
				    .savetext(uiStrings::phrSelect(tr("on OK")))
				    .modal(false) )
    , plotter_( p )
    , selgrps_(p.selectionGrps())
{
    setPrefHeight( 500 );

    uiTable::Setup su( selgrps_.size(), 2 );
    su.rowdesc(tr("Selection Group")).selmode(uiTable::Single);
    tbl_ = new uiTable( this, su, "Selection Groups" );
    tbl_->setPrefHeight( 200 );
    tbl_->setColumnReadOnly( 1, true );
    tbl_->colorSelectionChanged.notify( mCB(this,uiDPSSelGrpDlg,changeColCB) );
    tbl_->rowInserted.notify( mCB(this,uiDPSSelGrpDlg,addSelGrp) );
    tbl_->valueChanged.notify( mCB(this,uiDPSSelGrpDlg,changeSelGrbNm) );
    tbl_->selectionChanged.notify( mCB(this,uiDPSSelGrpDlg,setCurSelGrp) );
    tbl_->setColumnLabel( 0, uiStrings::sName() );
    tbl_->setColumnLabel( 1, uiStrings::sColor() );

    for ( int idx=0; idx<selgrps_.size(); idx++ )
    {
	tbl_->setText( RowCol(idx,0), selgrps_[idx]->name() );
	setColorCell( idx, selgrps_[idx]->col_ );
    }

    curselgrp_ = tbl_->currentRow() < 0 ? 0 : tbl_->currentRow();
    uiPushButton* addgrpbut = new uiPushButton( this, tr("Add group"),
	    mCB(this,uiDPSSelGrpDlg,addSelGrp), true );
    addgrpbut->attach( alignedBelow, tbl_ );

    uiPushButton* remgrpbut = new uiPushButton( this, tr("Remove group"),
	    mCB(this,uiDPSSelGrpDlg,remSelGrp), true );
    remgrpbut->attach( rightTo, addgrpbut );

    uiPushButton* expgrpbut = new uiPushButton( this, tr("Save groups"),
	    mCB(this,uiDPSSelGrpDlg,exportSelectionGrps), true );
    expgrpbut->attach( rightTo, remgrpbut );

    uiPushButton* impgrpbut = new uiPushButton( this, tr("Open groups"),
	    mCB(this,uiDPSSelGrpDlg,importSelectionGrps), true );
    impgrpbut->attach( rightTo, expgrpbut );

    uiPushButton* scalesgbut =
	new uiPushButton( this, tr("Selectedness"),
			  mCB(this,uiDPSSelGrpDlg,calcSelectedness), false );
    scalesgbut->attach( rightTo, impgrpbut );

    tbl_->setSelected( RowCol(0,0), true );
    tbl_->setCurrentCell( RowCol(0,0), true );
}


void uiDPSSelGrpDlg::changeSelGrbNm( CallBacker* )
{

    if ( tbl_->currentRow() < 0 || !selgrps_.validIdx(tbl_->currentRow()) )
	return;

    for ( int idx=0; idx<tbl_->nrRows(); idx++ )
    {
	SelectionGrp* selgrp = selgrps_[ idx ];
	selgrp->setName( tbl_->text(RowCol(idx,0)) );
	selgrp->col_ = tbl_->getCellColor( RowCol(idx,1) );
    }

    setCurSelGrp(0);
}


void uiDPSSelGrpDlg::setCurSelGrp( CallBacker* )
{
    curselgrp_ = tbl_->currentRow();
    plotter_.setCurSelGrp( curselgrp_ < 0 ? 0 : curselgrp_ );
}


void uiDPSSelGrpDlg::setColorCell( int rowidx, const Color& col )
{
    tbl_->setColorSelectionCell( RowCol(rowidx,1), false );
    tbl_->setCellColor( RowCol(rowidx,1), col );
}


void uiDPSSelGrpDlg::addSelGrp( CallBacker* cb )
{
    tbl_->insertRows( tbl_->nrRows(), 1 );
    tbl_->setColumnReadOnly( 1, true );
    RowCol newcell = RowCol( tbl_->nrRows()-1, 1 );
    setColorCell( newcell.row(), getRandomColor() );
    BufferString selgrpnm( "No " );
    mDefineStaticLocalObject( int, selgrpnr, = 2 );
    selgrpnm += selgrpnr;
    selgrpnr++;
    tbl_->setText( RowCol(newcell.row(),0), selgrpnm );
    selgrps_ += new SelectionGrp( selgrpnm,
		    tbl_->getCellColor(RowCol(newcell.row(),1)) );

    setCurSelGrp(0);
}

void uiDPSSelGrpDlg::importSelectionGrps( CallBacker* )
{
    uiReadSelGrp dlg( this, plotter_ );
    NotifyStopper ns( tbl_->valueChanged );
    if ( dlg.go() )
    {
	while ( tbl_->nrRows() )
	    tbl_->removeRow(0);

	tbl_->setNrRows( selgrps_.size() );
	for ( int idx=0; idx<selgrps_.size(); idx++ )
	{
	    BufferString temp(selgrps_[idx]->name());
	    tbl_->setText( RowCol(idx,0), temp.buf() );
	    setColorCell( idx, selgrps_[idx]->col_ );
	}
    }

    setCurSelGrp(0);
}


void uiDPSSelGrpDlg::exportSelectionGrps( CallBacker* )
{
    const BufferString axisname0 = toString(
		    plotter_.axisHandler(0)->getCaption() );
    const BufferString axisname1 = toString(
		    plotter_.axisHandler(1)->getCaption() );
    auto axh2 = plotter_.axisHandler(2);
    const BufferString axisname2 = toString(
	    axh2 ? axh2->getCaption() : uiString::empty() );

    uiExpSelectionArea::Setup su( axisname0, axisname1, axisname2 );
    uiExpSelectionArea dlg( this, plotter_.selectionGrps(), su );
    dlg.go();
}


void uiDPSSelGrpDlg::remSelGrp( CallBacker* )
{
    if ( tbl_->currentRow()<0 || tbl_->nrRows()<=1 ) return;

    selgrps_.removeSingle( tbl_->currentRow() );
    tbl_->removeRow( tbl_->currentRow() );

    setCurSelGrp(0);
    plotter_.reDrawSelections();
}


void uiDPSSelGrpDlg::changeColCB( CallBacker* )
{
    plotter_.reDrawSelections();
}


void uiDPSSelGrpDlg::calcSelectedness( CallBacker* )
{
    if ( tbl_->currentRow() < 0 )
    {
       uiMSG().error(tr("Select a selection group"
		        " to map its likeliness."));
       return;
    }

    plotter_.uidps().calcSelectedness();
}
