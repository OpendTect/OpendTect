/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uimathexpression.h"
#include "mathexpression.h"

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
	new uiLabel( this, setup_.label_, txtfld_ );

    if ( setup_.withsetbut_ )
    {
	setbut_ = new uiPushButton( this, "&Set",
			mCB(this,uiMathExpression,setButCB),
			!setup_.setcb_.willCall() );
	setbut_->attach( rightOf, txtfld_ );
    }

    if ( setup_.withfns_ )
    {
	uiGroup* insgrp = new uiGroup( this, "insert group" );

	grpfld_ = new uiComboBox( insgrp, "Formula group" );
	const ObjectSet<const MathExpressionOperatorDescGroup>& grps =
		    MathExpressionOperatorDescGroup::supported();
	for ( int idx=0; idx<grps.size(); idx++ )
	    grpfld_->addItem( grps[idx]->name_ );
	grpfld_->setCurrentItem( 2 );
	grpfld_->selectionChanged.notify( mCB(this,uiMathExpression,grpSel) );
	new uiLabel( insgrp, setup_.fnsbelow_ ? "   \\" : "   /", grpfld_ );
	grpfld_->setHSzPol( uiObject::Medium );
	grpfld_->setStretch( 0, 0 );

	fnfld_ = new uiComboBox( insgrp, "Formula desc" );
	fnfld_->attach( rightOf, grpfld_ );
	grpSel( 0 );

	uiPushButton* but = new uiPushButton( insgrp, "&Insert",
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


uiToolButton* uiMathExpression::addButton( const uiToolButtonSetup& tbsu )
{
    uiToolButton* newbut = new uiToolButton( this, tbsu );
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
    const MathExpressionOperatorDescGroup& grp =
		*MathExpressionOperatorDescGroup::supported()[grpidx];
    for ( int idx=0; idx<grp.opers_.size(); idx++ )
    {
	const MathExpressionOperatorDesc& oper = *grp.opers_[idx];
	BufferString str( oper.symbol_ );
	str.add( " (" ).add( oper.desc_ ).add( ")" );
	fnfld_->addItem( str );
    }
}


void uiMathExpression::doIns( CallBacker* )
{
    const int grpidx = grpfld_->currentItem();
    const int opidx = fnfld_->currentItem();
    const MathExpressionOperatorDescGroup& grp =
		*MathExpressionOperatorDescGroup::supported()[grpidx];
    const MathExpressionOperatorDesc& oper = *grp.opers_[opidx];

    BufferString txt( oper.symbol_ );
    if ( !oper.isoperator_ )
    {
	txt += "( ";
	for ( int idx=1; idx<oper.nrargs_; idx++ )
	    txt += ", ";
	txt += ")";
    }
    insertText( txt );
}
