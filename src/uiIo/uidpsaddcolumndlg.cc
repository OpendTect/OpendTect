/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          July 2011
 RCS:           $Id: uidpsaddcolumndlg.cc,v 1.5 2012-08-10 03:50:05 cvsaneesh Exp $: 
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "";


#include "uidpsaddcolumndlg.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uitable.h"

#include "mathexpression.h"

uiDPSAddColumnDlg::uiDPSAddColumnDlg( uiParent* p, bool withmathop )
    : uiDialog(p,uiDialog::Setup("Add Column","",""))
    , mathobj_(0)
    , withmathop_(withmathop)
    , inpfld_(0)
    , setbut_(0)
    , vartable_(0)
{
    nmfld_ = new uiGenInput( this, "Column Name" );

    if ( withmathop )
    {
	uiLabel* label = new uiLabel( this, "Define Mathematical Operation" );
	label->attach( alignedBelow, nmfld_ ); 
	
	inpfld_ = new uiGenInput( this, "Define Math Operation" );
	inpfld_->setElemSzPol( uiObject::WideMax );
	inpfld_->updateRequested.notify(
		mCB(this,uiDPSAddColumnDlg,parsePush) );
	inpfld_->valuechanging.notify(
		mCB(this,uiDPSAddColumnDlg,checkMathExpr) );
	inpfld_->attach( alignedBelow, label ); 

	setbut_ = new uiPushButton( this, "Set", true );
	setbut_->activated.notify( mCB(this,uiDPSAddColumnDlg,parsePush) );
	setbut_->attach( rightTo, inpfld_ );

	vartable_ = new uiTable( this,uiTable::Setup().rowdesc("X")
					.minrowhgt(1.5) .maxrowhgt(2)
					.mincolwdt(3*uiObject::baseFldSize())
					.maxcolwdt(3.5f*uiObject::baseFldSize())
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
    }
}


void uiDPSAddColumnDlg::setColInfos( const BufferStringSet& colnames,
				     const TypeSet<int>& colids )
{
    if ( !withmathop_ ) return;

    colnames_ = colnames;
    colids_ = colids;
    updateDisplay();
}


void uiDPSAddColumnDlg::checkMathExpr( CallBacker* )
{
    if ( !withmathop_ ) return;

    if ( mathexprstring_ != inpfld_->text() )
	setbut_->setSensitive( true );
    else
	setbut_->setSensitive( false );
}


void uiDPSAddColumnDlg::parsePush( CallBacker* )
{
    if ( !withmathop_ ) return;

    mathexprstring_ = inpfld_->text();
    MathExpressionParser mep( mathexprstring_ );
    mathobj_ = mep.parse();
    if ( !mathobj_ )
    {
	if ( mep.errMsg() ) uiMSG().error( mep.errMsg() );
	vartable_->display( false );
	return;
    }

    setbut_->setSensitive( false );
    updateDisplay();
}


void uiDPSAddColumnDlg::updateDisplay()
{
    if ( !withmathop_ || !mathobj_ ) return;

    const int nrvars = mathobj_->nrVariables();
    vartable_->setNrRows( nrvars );
    for ( int idx=0; idx<nrvars; idx++ )
    {
	uiComboBox* varsel = new uiComboBox( 0, colnames_, "Variable");
	vartable_->setRowLabel( idx, mathobj_->uniqueVarName(idx) );
	vartable_->setCellObject( RowCol(idx,0), varsel );
    }

    vartable_->display( true );
}


bool uiDPSAddColumnDlg::acceptOK( CallBacker* )
{
    if ( !withmathop_ || (withmathop_ && !mathobj_) )
	return true;

    usedcolids_.erase();

    int nrvars = mathobj_->nrVariables();
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
{ return nmfld_->text(); }

