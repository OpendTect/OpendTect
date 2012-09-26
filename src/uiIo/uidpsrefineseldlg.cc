/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          August 2009
 RCS:           $Id$: 
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uidpsrefineseldlg.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uidatapointsetcrossplot.h"
#include "uilabel.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uitable.h"

#include "mathexpression.h"

uiDPSRefineSelDlg::uiDPSRefineSelDlg( uiDataPointSetCrossPlotter& p )
    : uiDialog(p.parent(),uiDialog::Setup("Refine selection",
					  "Define mathematical operation",
					  "111.0.4")
			    .savebutton(!p.isADensityPlot())
			    .savetext("Select on Ok").modal(false) )
    , plotter_(p)
    , mathobj_(0)
    , mathexprstring_(plotter_.mathObjStr())
    , dcolids_(plotter_.modifiedColIds())
{
    uiLabel* label =
	new uiLabel( this, "Ranges (e.g. 0>x0 && x0>1.5 && -6125<x1)" );

    inpfld_ = new uiGenInput( this, "Enter Ranges" );
    inpfld_->setElemSzPol( uiObject::WideMax );
    inpfld_->updateRequested.notify( mCB(this,uiDPSRefineSelDlg,parsePush) );
    inpfld_->valuechanging.notify( mCB(this,uiDPSRefineSelDlg,checkMathExpr) );
    label->attach( leftAlignedAbove, inpfld_ ); 

    setbut_ = new uiPushButton( this, "Set", true );
    setbut_->activated.notify( mCB(this,uiDPSRefineSelDlg,parsePush) );
    setbut_->attach( rightTo, inpfld_ );

    vartable_ = new uiTable( this,uiTable::Setup().rowdesc("X")
					.minrowhgt(1.5) .maxrowhgt(2)
					.mincolwdt(3*uiObject::baseFldSize())
					.maxcolwdt(3.5*uiObject::baseFldSize())
					.defrowlbl("") .fillcol(true)
					.fillrow(true) .defrowstartidx(0),
					"Variable X attribute table" );
    const char* xcollbls[] = { "Select input for", 0 };
    vartable_->setColumnLabels( xcollbls );
    vartable_->setNrRows( 2 );
    vartable_->setStretch( 2, 0 );
    vartable_->setRowResizeMode( uiTable::Fixed );
    vartable_->setColumnResizeMode( uiTable::Fixed );
    vartable_->attach( alignedBelow, inpfld_ );
    vartable_->display( false );
    if ( !mathexprstring_.isEmpty() )
    {
	inpfld_->setText( mathexprstring_ );
	parsePush(0);
    }
}


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
    MathExpressionParser mep( mathexprstring_ );
    mathobj_ = mep.parse();
    if ( !mathobj_ )
    {
	if ( mep.errMsg() ) uiMSG().error( mep.errMsg() );
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
    for ( ; dcid<dps.nrCols(); dcid++ )
	colnms_.add( plotter_.uidps().userName(dcid) );


    for ( int idx=0; idx<nrvars; idx++ )
    {
	uiComboBox* varsel = new uiComboBox( 0, colnms_, "Variable");
	if ( !dcolids_.isEmpty() && dcolids_.validIdx(idx) )
	   varsel->setCurrentItem( cColIds(dcolids_[idx]) );
	vartable_->setRowLabel( idx, mathobj_->uniqueVarName(idx) );
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

    PtrMan<MathExpression> testexpr = mathobj_->clone();
    nrvars = testexpr->nrVariables();
    for ( int idx=0; idx<nrvars; idx++ )
	testexpr->setVariableValue( idx, 100 );

    if ( !mIsZero(testexpr->getValue(),mDefEps) &&
	 !mIsZero(testexpr->getValue()-1,mDefEps) )
    {
	uiMSG().error( "Equation should return true or false" );
	return false;
    }

    setPlotter();
    return true;
}
