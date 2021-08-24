/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : August 2014
-*/


#include "uiprestacktrimstatics.h"
#include "uiprestackprocessor.h"

#include "uibuttongroup.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitoolbutton.h"

#include "prestacktrimstatics.h"


namespace PreStack
{

void uiTrimStatics::initClass()
{
    uiPSPD().addCreator( create, TrimStatics::sFactoryKeyword() );
}


uiDialog* uiTrimStatics::create( uiParent* p, Processor* psp )
{
    mDynamicCastGet(TrimStatics*,trimstat,psp)
    if ( !trimstat ) return 0;

    return new uiTrimStatics( p, trimstat );
}


uiTrimStatics::uiTrimStatics( uiParent* p, TrimStatics* trimstat )
    : uiDialog(p,uiDialog::Setup(tr("Trim Statics"),mNoDlgTitle,
				 mODHelpKey(mPreStackMuteHelpID)))
    , processor_( trimstat )
{
    uiStringSet collbls;
    collbls.add( tr("Pilot trace\nOffset start") )
           .add( tr("Pilot trace\nOffset stop") )
           .add( tr("Trim\nOffset start") ).add( tr("Trim\nOffset stop") )
           .add( tr("Max shift") );
    uiTable::Setup ts; ts.defrowlbl(true).rowdesc("Iteration");
    table_ = new uiTable( this, ts, "Parameter table" );
    table_->setPrefWidth( 500 );
    table_->setColumnLabels( collbls );
    table_->resizeHeaderToContents( true );
    table_->resizeHeaderToContents( false );
    table_->setSelectionBehavior( uiTable::SelectRows );
    table_->valueChanged.notify( mCB(this,uiTrimStatics,changeCB) );
    table_->rowClicked.notify( mCB(this,uiTrimStatics,rowClickCB) );

    BufferStringSet outputs;
    outputs.add( "Pilot trace" ).add( "Shift" ).add( "Trim Statics" );
    outputfld_ = new uiGenInput( this, uiStrings::sOutput(),
		 StringListInpSpec(outputs) );
    outputfld_->setValue( processor_->getOutput() );
    outputfld_->attach( leftAlignedBelow, table_ );

    uiButtonGroup* grp = new uiButtonGroup( this, "Buttons", OD::Vertical );
    grp->attach( rightTo, table_ );
    new uiToolButton( grp, "addnew", uiStrings::phrAdd(tr("Iteration")),
					     mCB(this,uiTrimStatics,addCB) );
    rmbut_ = new uiToolButton( grp, "trashcan",	uiStrings::phrRemove(
			    tr("Iteration")), mCB(this,uiTrimStatics,rmCB) );
    upbut_ = new uiToolButton(grp, uiToolButton::UpArrow,
			uiStrings::sMoveUp(), mCB(this,uiTrimStatics,moveUpCB));
    downbut_ = new uiToolButton( grp, uiToolButton::DownArrow,
				      uiStrings::sMoveDown(),
				      mCB(this,uiTrimStatics,moveDownCB) );

    fillTable();
}


void uiTrimStatics::fillTable()
{
    const TypeSet<TrimStatics::Iteration>& its = processor_->getIterations();
    if ( its.isEmpty() )
	processor_->addIteration( TrimStatics::Iteration() );

    const int nrits = its.size();
    table_->setNrRows( nrits );
    for ( int idx=0; idx<nrits; idx++ )
    {
	table_->setValue( RowCol(idx,0), its[idx].ptoffsetrg_.start );
	table_->setValue( RowCol(idx,1), its[idx].ptoffsetrg_.stop );
	table_->setValue( RowCol(idx,2), its[idx].tsoffsetrg_.start );
	table_->setValue( RowCol(idx,3), its[idx].tsoffsetrg_.stop );
	table_->setValue( RowCol(idx,4), its[idx].maxshift_ );
    }

    updateButtons();
}


static Color getColor( bool sel )
{
    mDefineStaticLocalObject( Color, bgcol, = uiMain::theMain().windowColor() );
    mDefineStaticLocalObject( Color, selcol, = bgcol.darker(0.3f) );
    return sel ? selcol : bgcol;
}

void uiTrimStatics::rowClickCB( CallBacker* )
{
    const RowCol& rc = table_->notifiedCell();
    for ( int idx=0; idx<table_->nrRows(); idx++ )
	table_->setHeaderBackground( idx, getColor(idx==rc.row()), true );

    updateButtons();
}


void uiTrimStatics::changeCB( CallBacker* )
{
    const RowCol& rc = table_->notifiedCell();
    TypeSet<TrimStatics::Iteration>& its = processor_->getIterations();
    TrimStatics::Iteration& curit = its[rc.row()];
    const float val = table_->getFValue( rc );
    if ( rc.col()==0 )	curit.ptoffsetrg_.start = val;
    if ( rc.col()==1 )	curit.ptoffsetrg_.stop = val;
    if ( rc.col()==2 )	curit.tsoffsetrg_.start = val;
    if ( rc.col()==3 )	curit.tsoffsetrg_.stop = val;
    if ( rc.col()==4 )	curit.maxshift_ = val;
}


void uiTrimStatics::updateButtons()
{
    const int currow = table_->currentRow();
    const int nrrows = table_->nrRows();
    rmbut_->setSensitive( currow>=0 && currow<nrrows );
    upbut_->setSensitive( currow>0 && currow<nrrows );
    downbut_->setSensitive( currow>=0 && currow<nrrows-1 );
}


void uiTrimStatics::addCB( CallBacker* )
{
    processor_->addIteration( TrimStatics::Iteration() );
    fillTable();
}


void uiTrimStatics::rmCB( CallBacker* )
{
    const int currow = table_->currentRow();
    processor_->removeIteration( currow );
    fillTable();
}


void uiTrimStatics::moveUpCB( CallBacker* )
{
    const int currow = table_->currentRow();
    TypeSet<TrimStatics::Iteration>& its = processor_->getIterations();
    its.swap( currow-1, currow );
    fillTable();
}


void uiTrimStatics::moveDownCB( CallBacker* )
{
    const int currow = table_->currentRow();
    TypeSet<TrimStatics::Iteration>& its = processor_->getIterations();
    its.swap( currow, currow+1 );
    fillTable();
}


bool uiTrimStatics::acceptOK( CallBacker* )
{
    if ( !processor_ ) return true;

    processor_->setOutput( outputfld_->getIntValue() );
    return true;
}

} // namespace PreStack
