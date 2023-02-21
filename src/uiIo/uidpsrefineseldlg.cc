/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidpsrefineseldlg.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uidatapointsetcrossplot.h"
#include "uilabel.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uitable.h"
#include "od_helpids.h"

#include "mathexpression.h"

uiDPSRefineSelDlg::uiDPSRefineSelDlg( uiDataPointSetCrossPlotter& p )
    : uiDialog(p.parent(),uiDialog::Setup(tr("Refine selection"),
					  tr("Define mathematical operation"),
					  mODHelpKey(mSelectionSettDlgHelpID) )
			    .savebutton(!p.isADensityPlot())
			    .savetext(tr("Select on OK")).modal(false) )
    , plotter_(p)
    , mathobj_(0)
    , mathexprstring_(plotter_.mathObjStr())
    , dcolids_(plotter_.modifiedColIds())
{
    uiLabel* label =
	new uiLabel( this, tr("Ranges (e.g. 0>x0 && x0>1.5 && -6125<x1)") );

    inpfld_ = new uiGenInput( this, tr("Enter Ranges") );
    inpfld_->setElemSzPol( uiObject::WideMax );
    inpfld_->updateRequested.notify( mCB(this,uiDPSRefineSelDlg,parsePush) );
    inpfld_->valueChanging.notify( mCB(this,uiDPSRefineSelDlg,checkMathExpr) );
    label->attach( leftAlignedAbove, inpfld_ );

    setbut_ = new uiPushButton( this, tr("Set"), true );
    setbut_->activated.notify( mCB(this,uiDPSRefineSelDlg,parsePush) );
    setbut_->attach( rightTo, inpfld_ );

    uiGroup* tblgrp = new uiGroup( this );
    tblgrp->attach( alignedBelow, inpfld_ );
    vartable_ = new uiTable( tblgrp,uiTable::Setup(2,1).rowdesc(uiStrings::sX())
					.minrowhgt(1.5) .maxrowhgt(2)
					.mincolwdt(3.0f*uiObject::baseFldSize())
					.maxcolwdt(3.5f*uiObject::baseFldSize())
					.defrowlbl("") .fillcol(true)
					.fillrow(true) .defrowstartidx(0),
					"Variable X attribute table" );
    vartable_->setColumnLabel( 0, uiStrings::sInput() );
    vartable_->display( false );
    if ( !mathexprstring_.isEmpty() )
    {
	inpfld_->setText( mathexprstring_ );
	parsePush(0);
    }
}


uiDPSRefineSelDlg::~uiDPSRefineSelDlg()
{}


void uiDPSRefineSelDlg::checkMathExpr( CallBacker* )
{
    if ( mathexprstring_ != inpfld_->text() )
	setbut_->setSensitive( true );
    else
	setbut_->setSensitive( false );
}


void uiDPSRefineSelDlg::parsePush( CallBacker* )
{
    mathexprstring_ = inpfld_->text();
    Math::ExpressionParser mep( mathexprstring_ );
    mathobj_ = mep.parse();
    if ( !mathobj_ )
    {
	if ( mep.errMsg() ) uiMSG().error( mToUiStringTodo(mep.errMsg()) );
	dcolids_.erase();
	vartable_->display( false );
	return;
    }

    setbut_->setSensitive( false );
    updateDisplay();
}


int uiDPSRefineSelDlg::cColIds( int dcolid )
{ return dcolid + 3; }


void uiDPSRefineSelDlg::updateDisplay()
{
    const int nrvars = mathobj_->nrVariables();
    vartable_->setNrRows( nrvars );

    const DataPointSet& dps = plotter_.dps();
    uiDataPointSet::DColID dcid = -dps.nrFixedCols()+1;
    colnms_.setEmpty();
    for ( ; dcid<dps.nrCols(); dcid++ )
	colnms_.add( plotter_.uidps().userName(dcid) );


    for ( int idx=0; idx<nrvars; idx++ )
    {
	uiComboBox* varsel = new uiComboBox( 0, colnms_, "Variable");
	if ( !dcolids_.isEmpty() && dcolids_.validIdx(idx) )
	   varsel->setCurrentItem( cColIds(dcolids_[idx]) );
	vartable_->setRowLabel( idx, toUiString(mathobj_->uniqueVarName(idx)) );
	vartable_->setCellObject( RowCol(idx,0), varsel );
    }

    vartable_->display( true );
}


void uiDPSRefineSelDlg::setPlotter()
{
    plotter_.setMathObj( mathobj_ );
    plotter_.setMathObjStr( mathexprstring_ );
    plotter_.setModifiedColIds( dcolids_ );
}


bool uiDPSRefineSelDlg::acceptOK( CallBacker* )
{
    dcolids_.erase();
    if ( !mathobj_ )
    {
	setPlotter();
	return true;
    }

    TypeSet<int> colids;
    const DataPointSet& dps = plotter_.dps();
    uiDataPointSet::DColID dcid = -dps.nrFixedCols()+1;
    for ( ; dcid<dps.nrCols(); dcid++ )
	colids += dcid;

    int nrvars = mathobj_->nrVariables();
    for ( int idx=0; idx<nrvars; idx++ )
    {
	uiObject* obj = vartable_->getCellObject( RowCol(idx,0) );
	mDynamicCastGet( uiComboBox*, box, obj );
	if ( !box )
	    continue;

	dcolids_ += colids[box->currentItem()];
    }

    PtrMan<Math::Expression> testexpr = mathobj_->clone();
    nrvars = testexpr->nrVariables();
    for ( int idx=0; idx<nrvars; idx++ )
	testexpr->setVariableValue( idx, 100 );

    if ( !mIsZero(testexpr->getValue(),mDefEps) &&
	 !mIsZero(testexpr->getValue()-1,mDefEps) )
    {
	uiMSG().error( tr("Equation should return true or false") );
	return false;
    }

    setPlotter();
    return true;
}
