/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidpsaddcolumndlg.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uitable.h"

#include "mathexpression.h"

uiDPSAddColumnDlg::uiDPSAddColumnDlg( uiParent* p, bool withmathop )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrAdd(tr("Column")),
				 mNoDlgTitle, mNoHelpKey))
    , mathobj_(nullptr)
    , inpfld_(nullptr)
    , setbut_(nullptr)
    , vartable_(nullptr)
    , withmathop_(withmathop)
{
    nmfld_ = new uiGenInput( this, tr("Column name") );
    nmfld_->setStretch( 2, 0 );

    if ( !withmathop )
	return;

    uiLabel* label = new uiLabel( this,
	    tr("Define Mathematical expression to compute new column") );
    label->attach( alignedBelow, nmfld_ );

    inpfld_ = new uiGenInput( this, tr("Formula") );
    inpfld_->setElemSzPol( uiObject::WideMax );
    inpfld_->updateRequested.notify(
	    mCB(this,uiDPSAddColumnDlg,parsePush) );
    inpfld_->valueChanging.notify(
	    mCB(this,uiDPSAddColumnDlg,checkMathExpr) );
    inpfld_->attach( alignedBelow, label );

    setbut_ = new uiPushButton( this, tr("Set"), true );
    setbut_->activated.notify( mCB(this,uiDPSAddColumnDlg,parsePush) );
    setbut_->attach( rightTo, inpfld_ );

    uiGroup* tblgrp = new uiGroup( this );
    tblgrp->attach( alignedBelow, inpfld_ );
    vartable_ = new uiTable( tblgrp,uiTable::Setup(2,1)
				.rowdesc(uiStrings::sX())
				.minrowhgt(1.5) .maxrowhgt(2)
				.mincolwdt(3.0f*uiObject::baseFldSize())
				.maxcolwdt(3.5f*uiObject::baseFldSize())
				.defrowlbl("").fillcol(true)
				.fillrow(true).defrowstartidx(0),
			     "Variable X attribute table" );
    vartable_->setColumnLabel( 0, uiStrings::sInput() );
}


uiDPSAddColumnDlg::~uiDPSAddColumnDlg()
{}


void uiDPSAddColumnDlg::setColInfos( const BufferStringSet& colnames,
				     const TypeSet<int>& colids )
{
    if ( !withmathop_ )
	return;

    colnames_ = colnames;
    colids_ = colids;
    updateDisplay();
}


void uiDPSAddColumnDlg::checkMathExpr( CallBacker* )
{
    if ( !withmathop_ )
	return;

    if ( mathexprstring_ != inpfld_->text() )
	setbut_->setSensitive( true );
    else
	setbut_->setSensitive( false );
}


void uiDPSAddColumnDlg::parsePush( CallBacker* )
{
    if ( !withmathop_ )
	return;

    mathexprstring_ = inpfld_->text();
    Math::ExpressionParser mep( mathexprstring_ );
    mathobj_ = mep.parse();
    if ( !mathobj_ )
    {
	if ( !mep.errMsg().isEmpty() )
	    uiMSG().error( mep.errMsg() );

	return;
    }

    setbut_->setSensitive( false );
    updateDisplay();
}


void uiDPSAddColumnDlg::updateDisplay()
{
    if ( !withmathop_ || !mathobj_ )
	return;

    const int nrvars = mathobj_->nrUniqueVarNames();
    vartable_->setNrRows( nrvars );
    for ( int idx=0; idx<nrvars; idx++ )
    {
	uiComboBox* varsel = new uiComboBox( 0, colnames_, "Variable");
	vartable_->setRowLabel( idx, toUiString(mathobj_->uniqueVarName(idx)) );
	vartable_->setCellObject( RowCol(idx,0), varsel );
    }
}


bool uiDPSAddColumnDlg::acceptOK( CallBacker* )
{
    if ( !withmathop_ || (withmathop_ && !mathobj_) )
	return true;

    const StringView attrnm = newAttribName();
    if ( attrnm.isEmpty() )
    {
	uiMSG().error( tr("Please provide a Column name") );
	return false;
    }

    usedcolids_.erase();

    const int nrvars = mathobj_->nrUniqueVarNames();
    for ( int idx=0; idx<nrvars; idx++ )
    {
	uiObject* obj = vartable_->getCellObject( RowCol(idx,0) );
	mDynamicCastGet( uiComboBox*, box, obj );
	if ( !box )
	    continue;

	usedcolids_ += colids_[box->currentItem()];
    }

    return true;
}


const char* uiDPSAddColumnDlg::newAttribName() const
{
    return nmfld_->text();
}
