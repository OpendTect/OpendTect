/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2001
________________________________________________________________________

-*/

#include "uiselsimple.h"
#include "uichecklist.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uilistbox.h"
#include "uilistboxfilter.h"
#include "uistrings.h"
#include "globexpr.h"
#include "bufstringset.h"


uiSelectFromList::uiSelectFromList( uiParent* p, const Setup& sup )
	: uiDialog(p,sup)
	, setup_(sup)
	, selfld_(0)
{
    const int sz = setup_.items_.size();
    if ( sz < 1 )
    {
	new uiLabel(this,tr("No items available for selection"));
	setCtrlStyle( CloseOnly );
	return;
    }

    selfld_ = new uiListBox( this );
    selfld_->setName("Select from List");

    filtfld_ = new uiListBoxFilter( *selfld_ );
    filtfld_->setItems( setup_.items_ );
    selfld_->resizeWidthToContents();

    if ( setup_.current_ < 1 )
	selfld_->setCurrentItem( 0 );
    else
	selfld_->setCurrentItem( setup_.current_ );

    selfld_->setHSzPol( uiObject::Wide );
    selfld_->doubleClicked.notify( mCB(this,uiDialog,accept) );
}


uiObject* uiSelectFromList::bottomFld()
{
    return selfld_->attachObj();
}


bool uiSelectFromList::acceptOK( CallBacker* )
{
    if ( !selfld_ ) return false;

    setup_.current_ = filtfld_->getCurrent();
    return setup_.current_ >= 0;
}


uiGetObjectName::uiGetObjectName( uiParent* p, const Setup& sup )
	: uiDialog(p,sup)
	, listfld_(0)
{
    if ( sup.items_.size() > 0 )
    {
	listfld_ = new uiListBox( this );
	for ( int idx=0; idx<sup.items_.size(); idx++ )
	    listfld_->addItem( mToUiStringTodo(sup.items_.get(idx)) );
	if ( !sup.deflt_.isEmpty() )
	    listfld_->setCurrentItem( sup.deflt_ );
	else
	    listfld_->setCurrentItem( 0 );
	listfld_->selectionChanged.notify( mCB(this,uiGetObjectName,selChg) );
	listfld_->doubleClicked.notify( mCB(this,uiDialog,accept) );
    }

    BufferString defnm( sup.deflt_ );
    if ( defnm.isEmpty() && listfld_ )
	defnm = sup.items_.get(0);
    inpfld_ = new uiGenInput( this, sup.inptxt_, defnm );

    mUseDefaultTextValidatorOnField(inpfld_);

    if ( listfld_ )
	inpfld_->attach( alignedBelow, listfld_ );
}


void uiGetObjectName::selChg( CallBacker* )
{
    if ( listfld_ )
	inpfld_->setText( listfld_->getText() );
}


const char* uiGetObjectName::text() const
{
    return inpfld_->text();
}


uiGroup* uiGetObjectName::bottomFld()
{
    return inpfld_;
}


bool uiGetObjectName::acceptOK( CallBacker* )
{
    const char* txt = text();
    return *txt ? true : false;
}


uiGetChoice::uiGetChoice( uiParent* p, const uiString& qn, bool wcncl,
			  const HelpKey& helpkey )
    : uiDialog(p,uiDialog::Setup(uiStrings::sSpecify(),qn,
	       helpkey))
    , allowcancel_(wcncl)
{
    inpfld_ = new uiCheckList( this, uiCheckList::OneOnly );
}


uiGetChoice::uiGetChoice( uiParent* p, const BufferStringSet& opts,
			  const uiString& qn, bool wcncl,
			  const HelpKey& helpkey )
    : uiDialog(p,uiDialog::Setup(uiStrings::sSpecify(),qn,
	       helpkey))
    , allowcancel_(wcncl)
{
    inpfld_ = new uiCheckList( this, uiCheckList::OneOnly );
    inpfld_->addItems( opts );
    setDefaultChoice( 0 );
}


uiGetChoice::uiGetChoice( uiParent* p, uiDialog::Setup s,
			  const BufferStringSet& opts, bool wc )
    : uiDialog(p,s)
    , allowcancel_(wc)
{
    inpfld_ = new uiCheckList( this, uiCheckList::OneOnly );
    inpfld_->addItems( opts );
    setDefaultChoice( 0 );
}


void uiGetChoice::addChoice( const uiString& txt, const char* iconnm )
{
    inpfld_->addItem( txt, iconnm );
}


void uiGetChoice::setDefaultChoice( int nr )
{
    inpfld_->setChecked( nr, true );
}


uiCheckList* uiGetChoice::checkList()
{
    return inpfld_;
}


uiGroup* uiGetChoice::bottomFld()
{
    return inpfld_;
}


bool uiGetChoice::rejectOK( CallBacker* )
{
    if ( !allowcancel_ )
	return false;
    choice_ = -1;
    return true;
}


bool uiGetChoice::acceptOK( CallBacker* )
{
    choice_ = inpfld_->firstChecked();
    return true;
}
