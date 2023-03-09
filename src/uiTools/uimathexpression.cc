/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimathexpression.h"
#include "mathexpression.h"
#include "mathspecvars.h"

#include "uitoolbutton.h"
#include "uilineedit.h"
#include "uicombobox.h"
#include "uilabel.h"



uiMathExpression::uiMathExpression( uiParent* p,
				    const uiMathExpression::Setup& su )
    : uiGroup(p,"MathExpression editor")
    , setup_(su)
    , grpfld_(0)
    , setbut_(0)
    , lastbut_(0)
    , formSet(this)
{
    txtfld_ = new uiLineEdit( this, "Formula" );
    txtfld_->setStretch( 2, 0 );
    txtfld_->returnPressed.notify( mCB(this,uiMathExpression,retPressCB) );
    if ( !setup_.label_.isEmpty() )
    {
	uiLabel* lbl = new uiLabel( this, setup_.label_ );
	lbl->attach( leftOf, txtfld_ );
    }

    if ( setup_.withsetbut_ )
    {
	setbut_ = new uiPushButton( this, tr("Set"),
			mCB(this,uiMathExpression,setButCB),
			!setup_.setcb_.willCall() );
	setbut_->attach( rightOf, txtfld_ );
    }

    if ( setup_.withfns_ )
    {
	uiGroup* insgrp = new uiGroup( this, "insert group" );

	grpfld_ = new uiComboBox( insgrp, "Formula group" );
	const ObjectSet<const Math::ExpressionOperatorDescGroup>& grps =
		    Math::ExpressionOperatorDescGroup::supported();
	for ( int idx=0; idx<grps.size(); idx++ )
	    grpfld_->addItem( mToUiStringTodo(grps[idx]->name_) );
	if ( setup_.specvars_ )
	    grpfld_->addItem( tr("Other") );
	grpfld_->setCurrentItem( 2 );
	grpfld_->selectionChanged.notify( mCB(this,uiMathExpression,grpSel) );
	grpfld_->setHSzPol( uiObject::Medium );
	grpfld_->setStretch( 0, 0 );

	fnfld_ = new uiComboBox( insgrp, "Formula desc" );
	fnfld_->attach( rightOf, grpfld_ );
	grpSel( 0 );

	uiPushButton* but = new uiPushButton( insgrp, tr("Insert"),
		    mCB(this,uiMathExpression,doIns), true );
	but->attach( rightOf, fnfld_ );
	but->setStretch( 0, 0 );

	if ( setup_.fnsbelow_ )
	    insgrp->attach( alignedBelow, txtfld_ );
	else
	    txtfld_->attach( alignedBelow, insgrp );
	txtfld_->attach( widthSameAs, insgrp );
    }

    setHAlignObj( txtfld_ );
}


uiMathExpression::~uiMathExpression()
{}


uiButton* uiMathExpression::addButton( const uiToolButtonSetup& tbsu )
{
    uiButton* newbut = tbsu.getButton( this );
    if ( lastbut_ )
	newbut->attach( rightOf, lastbut_ );
    else if ( setbut_ )
	newbut->attach( setup_.fnsbelow_?alignedBelow:alignedAbove, setbut_ );
    // else attach it yourself
    lastbut_ = newbut;
    return newbut;
}


void uiMathExpression::setText( const char* txt )
{
    txtfld_->setText( txt );
}


void uiMathExpression::insertText( const char* txt )
{
    txtfld_->insert( txt );
}


const char* uiMathExpression::text()
{
    return txtfld_->text();
}


void uiMathExpression::setButCB( CallBacker* cb )
{
    if ( setup_.setcb_.willCall() )
	setup_.setcb_.doCall( this );
    else
	retPressCB( cb );
}


void uiMathExpression::retPressCB( CallBacker* )
{
    formSet.trigger();
}


void uiMathExpression::grpSel( CallBacker* )
{
    fnfld_->setEmpty();
    const int grpidx = grpfld_->currentItem();
    const ObjectSet<const Math::ExpressionOperatorDescGroup>& grps =
		Math::ExpressionOperatorDescGroup::supported();
    if ( grpidx == grps.size() )
    {
	for ( int idx=0; idx<setup_.specvars_->size(); idx++ )
	{
	    const Math::SpecVar& specvar = (*setup_.specvars_)[idx];
	    uiString str = tr("%1 (%2)").arg(mToUiStringTodo(specvar.varnm_)).
			   arg(mToUiStringTodo(specvar.dispnm_));
	    fnfld_->addItem( str );
	}
    }
    else
    {
	const Math::ExpressionOperatorDescGroup& grp = *grps[grpidx];
	for ( int idx=0; idx<grp.opers_.size(); idx++ )
	{
	    const Math::ExpressionOperatorDesc& oper = *grp.opers_[idx];
	    uiString str = tr("%1 (%2)").arg(mToUiStringTodo( oper.symbol_)).
			   arg(mToUiStringTodo(oper.desc_));
	    fnfld_->addItem( str );
	}
    }
}


void uiMathExpression::doIns( CallBacker* )
{
    const int grpidx = grpfld_->currentItem();
    const int opidx = fnfld_->currentItem();
    const ObjectSet<const Math::ExpressionOperatorDescGroup>& grps =
		Math::ExpressionOperatorDescGroup::supported();

    BufferString txt;
    if ( grpidx == grps.size() )
    {
	const Math::SpecVar& specvar = (*setup_.specvars_)[opidx];
	txt.set( specvar.varnm_ );
    }
    else
    {
	const Math::ExpressionOperatorDescGroup& grp = *grps[grpidx];
	const Math::ExpressionOperatorDesc& oper = *grp.opers_[opidx];
	txt.set( oper.symbol_ );
	if ( !oper.isoperator_ )
	{
	    txt += "( ";
	    for ( int idx=1; idx<oper.nrargs_; idx++ )
		txt += ", ";
	    txt += ")";
	}
    }

    insertText( txt );
}


void uiMathExpression::setPlaceholderText( const uiString& txt )
{
    txtfld_->setPlaceholderText( txt );
}
