/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene
 Date:          November 2013
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimathpropeddlg.h"
#include "mathexpression.h"
#include "mathproperty.h"
#include "separstr.h"
#include "uicombobox.h"
#include "uilineedit.h"
#include "uilistbox.h"
#include "uimathexpression.h"
#include "uimathexpressionvariable.h"
#include "uimsg.h"
#include "uirockphysform.h"
#include "uitoolbutton.h"
#include "unitofmeasure.h"

static const int cMaxNrInps = 6;

uiMathPropEdDlg::uiMathPropEdDlg( uiParent* p, MathProperty& pr,
				  const PropertyRefSelection& prs )
    : uiDialog( p, Setup("Math property",
		BufferString("Value generation by formula for ",pr.name()),
		"dgb:108.1.4"))
    , prop_(pr)
    , expr_(0)
{
    uiGroup* formgrp = new uiGroup( this, "Formula group" );

    uiMathExpression::Setup mesu( "Formula" ); mesu.withsetbut( false );
    formfld_ = new uiMathExpression( formgrp, mesu );
    FileMultiString fms( prop_.def() );
    formfld_->setText( fms[0] );
    uiToolButtonSetup tbsu( "rockphys", "Choose rockphysics formula",
		    mCB(this,uiMathPropEdDlg,rockPhysReq), "&Rock Physics");
    formfld_->addButton( tbsu )->attach( centeredAbove, formfld_->textField() );
    formfld_->formSet.notify( mCB(this,uiMathPropEdDlg,updVarsOnScreen) );

    BufferStringSet availpropnms;
    for ( int idx=0; idx<prs.size(); idx++ )
    {
	const PropertyRef* ref = prs[idx];
	if ( ref != &pr.ref() )
	    availpropnms.add( ref->name() );
    }
    availpropnms.add( "Depth" );
    availpropnms.add( "XPos" );

    const CallBack inspropcb( mCB(this,uiMathPropEdDlg,insProp) );
    uiToolButton* but = new uiToolButton( formgrp, uiToolButton::LeftArrow,
				"Insert property in formula", inspropcb );
    but->attach( centeredRightOf, formfld_ );

    propfld_ = new uiListBox( formgrp, "Properties" );
    propfld_->attach( centeredRightOf, but );
    for ( int idx=0; idx<availpropnms.size(); idx++ )
	propfld_->addItem( availpropnms.get(idx) );

    propfld_->doubleClicked.notify( inspropcb );

    uiGroup* varsgrp = new uiGroup( this, "Variable selection group" );
    for ( int idx=0; idx<cMaxNrInps; idx++ )
    {
	uiMathExpressionVariable* fld = new uiMathExpressionVariable(
				varsgrp, idx, true, &availpropnms );
	if ( idx )
	    fld->attach( alignedBelow, inpdataflds_[idx-1] );
	inpdataflds_ += fld;
	const UnitOfMeasure* uom = prop_.inputUnit( idx );
	if ( uom )
	    fld->setUnit( uom->name() );
    }

    const CallBack replcb( mCB(this,uiMathPropEdDlg,replPushed) );
    replbut_ = new uiPushButton( this, "Replace variable names in Formula",
				 replcb, true );
    replbut_->display( false );

    const ObjectSet<const UnitOfMeasure>& uns( UoMR().all() );
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
						"Output unit of measure" );
    outunfld_ = lcb->box();
    outunfld_->addItem( "-" );
    for ( int idx=0; idx<uns.size(); idx++ )
	outunfld_->addItem( uns[idx]->name() );
    outunfld_->setText( fms[pr.nrConsts()+1] );
    lcb->attach( alignedBelow, replbut_ );

    varsgrp->attach( alignedBelow, formgrp );
    replbut_->attach( centeredBelow, varsgrp );

    updVarsOnScreen();
}


uiMathPropEdDlg::~uiMathPropEdDlg()
{
    if ( expr_ ) delete expr_;
}


void uiMathPropEdDlg::insProp( CallBacker* )
{
    formfld_->insertText( propfld_->getText() );
    updVarsOnScreen();
}


void uiMathPropEdDlg::rockPhysReq( CallBacker* )
{
    uiDialog dlg( this, uiDialog::Setup("Rock Physics",
		  "Use a rock physics formula", "dgb:108.1.5") );
    uiRockPhysForm* formgrp = new uiRockPhysForm( &dlg, prop_.ref().stdType() );
    if ( dlg.go() )
    {
	formfld_->setText( formgrp->getText(true) );
	updVarsOnScreen();
    }
}


void uiMathPropEdDlg::updVarsOnScreen( CallBacker* cb )
{
    updateMathExpr();
    nrvars_ = expr_ ? expr_->nrUniqueVarNames() : 0;
    for ( int idx=0; idx<inpdataflds_.size(); idx++ )
    {
	uiMathExpressionVariable* uimev = inpdataflds_[idx];
	uimev->use( expr_ );
	uimev->selectInput( uimev->varName() );
    }

    if ( nrvars_ )
	replbut_->display( true );
}


void uiMathPropEdDlg::updateMathExpr()
{
    delete expr_; expr_ = 0;
    if ( !formfld_ ) return;

    const BufferString inp( formfld_->text() );
    if ( inp.isEmpty() ) return;

    Math::ExpressionParser mep( inp );
    expr_ = mep.parse();

    if ( !expr_ )
	uiMSG().warning( "The provided expression cannot be used:\n",
			 mep.errMsg() );
}


bool uiMathPropEdDlg::acceptOK( CallBacker* )
{
    replaceInputsInFormula();
    updVarsOnScreen();

    FileMultiString fms( formfld_->text() );
    fms.add( outunfld_->text() ? outunfld_->text() : "" );
    for ( int idx=0; idx<nrvars_; idx++ )
    {
	const UnitOfMeasure* uom = inpdataflds_[idx]->getUnit();
	fms.add( uom ? uom->name() : "" );
    }

    prop_.setDef( fms.buf() );
    return true;
}


void uiMathPropEdDlg::replPushed( CallBacker* )
{
    replaceInputsInFormula();
}


void uiMathPropEdDlg::replaceInputsInFormula()
{
    BufferString formulastr( formfld_->text() );
    for ( int idx=0; idx<inpdataflds_.size(); idx++ )
    {
	if ( !inpdataflds_[idx]->attachObj()->isDisplayed() )
	    continue;

	formulastr.replace( inpdataflds_[idx]->varName(),
			    inpdataflds_[idx]->getInput() );
    }

    formfld_->setText( formulastr.buf() );
}


BufferString uiMathPropEdDlg::formulaStr() const
{
    return formfld_->text();
}
