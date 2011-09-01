/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimathexpression.cc,v 1.2 2011-09-01 15:27:02 cvsbert Exp $";

#include "uimathexpression.h"
#include "mathexpression.h"

#include "uibutton.h"
#include "uilineedit.h"
#include "uicombobox.h"
#include "uilabel.h"



uiMathExpression::uiMathExpression( uiParent* p, bool withfns )
    : uiGroup(p,"MathExpression editor")
    , formSet(this)
    , grpfld_(0)
{
    txtfld_ = new uiLineEdit( this, "Formula" );
    txtfld_->setStretch( 2, 0 );
    txtfld_->returnPressed.notify( mCB(this,uiMathExpression,retPressCB) );

    if ( !withfns ) return;

    uiGroup* insgrp = new uiGroup( this, "insert group" );

    grpfld_ = new uiComboBox( insgrp, "Formula group" );
    const ObjectSet<const MathExpressionOperatorDescGroup>& grps =
		MathExpressionOperatorDescGroup::supported();
    for ( int idx=0; idx<grps.size(); idx++ )
	grpfld_->addItem( grps[idx]->name_ );
    grpfld_->setCurrentItem( 2 );
    grpfld_->selectionChanged.notify( mCB(this,uiMathExpression,grpSel) );
    new uiLabel( insgrp, "   \\", grpfld_ );
    grpfld_->setHSzPol( uiObject::Medium );
    grpfld_->setStretch( 0, 0 );

    fnfld_ = new uiComboBox( insgrp, "Formula desc" );
    fnfld_->attach( rightOf, grpfld_ );
    grpSel( 0 );

    uiPushButton* but = new uiPushButton( insgrp, "&Insert",
	    	mCB(this,uiMathExpression,doIns), true );
    but->attach( rightOf, fnfld_ );
    but->setStretch( 0, 0 );

    insgrp->attach( alignedBelow, txtfld_ );
    txtfld_->attach( widthSameAs, insgrp );
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
