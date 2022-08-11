/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2003
________________________________________________________________________

-*/

#include "uibuildlistfromlist.h"
#include "uieditobjectlist.h"

#include "uibuttongroup.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uitoolbutton.h"


static void chckYPlural( BufferString& str )
{
    const int len = str.size();
    if ( len < 3 ) return;
    char* ptr = str.getCStr() + len - 2;
    if ( *ptr == 'y' )
    {
	*ptr = '\0';
	str += "ies";
    }
}

uiEditObjectList::uiEditObjectList( uiParent* p, const char* itmtyp,
				    bool movable, bool compact )
    : uiGroup(p,"Object list build group")
    , selectionChange(this)
{
    if ( !itmtyp ) itmtyp = "object";
    BufferString listnm( "Defined ", itmtyp, "s" );
    chckYPlural( listnm );
    listfld_ = new uiListBox( this, listnm );
    listfld_->doubleClicked.notify( mCB(this,uiEditObjectList,edCB) );
    listfld_->deleteButtonPressed.notify( mCB(this,uiEditObjectList,rmCB) );
    listfld_->selectionChanged.notify( mCB(this,uiEditObjectList,selChgCB) );

    bgrp_ = new uiButtonGroup( this, "Buttons", OD::Vertical );
    const uiString sMoveUp = uiStrings::sMoveUp();
    const uiString sMoveDown = uiStrings::sMoveDown();
    if ( compact )
    {
#define mDefBut(butnm,txt,pm,cb,imm) \
	butnm##but_ = new uiToolButton( bgrp_, pm, txt, \
		mCB(this,uiEditObjectList,cb) )
	//-- Copy the following code exactly to the 'else' branch
	//  (if you want to be purist, put it in a separate file and include it)
	mDefBut( add, uiStrings::phrAdd(mToUiStringTodo(itmtyp)),
						    "addnew", addCB, false );
	mDefBut( ed, uiStrings::phrEdit(uiStrings::sProperties()),
						    "edit", edCB, false );
	mDefBut( rm, uiStrings::phrJoinStrings(uiStrings::sRemove(),
			    mToUiStringTodo(itmtyp)), "remove", rmCB, true );
	if ( movable )
	{
	    mDefBut( up, sMoveUp, "uparrow", upCB, true );
	    mDefBut( down, sMoveDown, "downarrow", downCB, true );
	}
	//--
    }
    else
    {
	int butsz = StringView(itmtyp).size() + 8;
	if ( butsz < 20 ) butsz = 20;

#undef mDefBut
#define mDefBut(butnm,txt,pm,cb,imm) \
	butnm##but_ = new uiPushButton( bgrp_, txt, uiPixmap(pm), \
			    mCB(this,uiEditObjectList,cb), imm ); \
	butnm##but_->setPrefWidthInChar( butsz )

	//-- Make sure this code is exactly a copy of above
	mDefBut( add, uiStrings::phrAdd(mToUiStringTodo(itmtyp)),
						    "addnew", addCB, false );
	mDefBut( ed, uiStrings::phrEdit(uiStrings::sProperties()),
						    "edit", edCB, false );
	mDefBut( rm, uiStrings::phrJoinStrings(uiStrings::sRemove(),
			    mToUiStringTodo(itmtyp)), "remove", rmCB, true );
	if ( movable )
	{
	    mDefBut( up, sMoveUp, "uparrow", upCB, true );
	    mDefBut( down, sMoveDown, "downarrow", downCB, true );
	}
	//--

    }
    bgrp_->attach( rightOf, listfld_ );
}


int uiEditObjectList::currentItem() const
{
    return listfld_->currentItem();
}


void uiEditObjectList::setItems( const BufferStringSet& itms, int newcur )
{
    const int newsz = itms.size();
    if ( newcur < 0 ) newcur = currentItem();
    if ( newcur < 0 ) newcur = 0;
    if ( newcur >= newsz ) newcur = newsz-1;
    NotifyStopper ns( listfld_->selectionChanged );
    listfld_->setEmpty();
    if ( newcur < 0 ) return;

    listfld_->addItems( itms );
    listfld_->setCurrentItem( newcur );
    manButSt();
}


void uiEditObjectList::manButSt()
{
    const int curidx = currentItem();
    edbut_->setSensitive( curidx >= 0 );
    rmbut_->setSensitive( curidx >= 0 );
    upbut_->setSensitive( curidx > 0 );
    downbut_->setSensitive( curidx < listfld_->size() - 1 );
}


uiBuildListFromList::Setup::Setup( bool mv, const char* avitmtp,
				   const char* defitmtp )
    : movable_(mv)
    , avitemtype_(avitmtp)
    , defitemtype_(defitmtp)
    , withio_(true)
    , singleuse_(false)
    , withtitles_(false)
{
    if ( avitemtype_.isEmpty() )
	avitemtype_ = "ingredient";
    if ( defitemtype_.isEmpty() )
	defitemtype_ = "definition";
    addtt_.add( "Add " ).add( defitemtype_ );
    edtt_.add( "Edit " ).add( defitemtype_ );
    rmtt_.add( "Remove " ).add( defitemtype_ );
    avtitle_.add( "Available " ).add( avitemtype_ ).add( "s" );
    deftitle_.add( "Defined " ).add( defitemtype_ ).add( "s" );
    chckYPlural( avtitle_ ); chckYPlural( deftitle_ );
}


uiBuildListFromList::uiBuildListFromList( uiParent* p,
			const uiBuildListFromList::Setup& su, const char* nm )
    : uiGroup(p,nm?nm:"List-from-list build group")
    , setup_(su)
    , usrchg_(false)
    , savebut_(0)
    , movedownbut_(0)
{
    avfld_ = new uiListBox( this, setup_.avtitle_ );
    avfld_->box()->setHSzPol( uiObject::Wide );
    avfld_->doubleClicked.notify( mCB(this,uiBuildListFromList,addCB) );
    if ( setup_.withtitles_ && !setup_.avtitle_.isEmpty() )
    {
	uiLabel* lbl = new uiLabel( this, mToUiStringTodo(setup_.avtitle_) );
	lbl->attach( centeredAbove, avfld_ );
    }

    uiToolButton* addbut = new uiToolButton( this, uiToolButton::RightArrow,
	mToUiStringTodo(setup_.addtt_), mCB(this,uiBuildListFromList,addCB) );
    addbut->attach( centeredRightOf, avfld_ );

    deffld_ = new uiListBox( this, setup_.deftitle_ );
    deffld_->box()->setHSzPol( uiObject::Wide );
    deffld_->attach( rightTo, avfld_ );
    deffld_->attach( ensureRightOf, addbut );
    deffld_->selectionChanged.notify( mCB(this,uiBuildListFromList,defSelCB) );
    deffld_->doubleClicked.notify( mCB(this,uiBuildListFromList,edCB) );
    if ( setup_.withtitles_ && !setup_.deftitle_.isEmpty() )
    {
	uiLabel* lbl = new uiLabel( this, mToUiStringTodo(setup_.deftitle_) );
	lbl->attach( centeredAbove, deffld_ );
    }

    edbut_ = new uiToolButton( this, "edit", mToUiStringTodo(setup_.edtt_),
			mCB(this,uiBuildListFromList,edCB) );
    edbut_->attach( rightOf, deffld_ );
    rmbut_ = new uiToolButton( this, "remove", mToUiStringTodo(setup_.rmtt_),
			mCB(this,uiBuildListFromList,rmCB) );
    rmbut_->attach( alignedBelow, edbut_ );

    if ( setup_.withio_ )
    {
	uiToolButton* openbut = new uiToolButton( this, "open",
		tr("Open stored set"), mCB(this,uiBuildListFromList,openCB) );
	openbut->attach( alignedBelow, rmbut_ );
	savebut_ = new uiToolButton( this, "save", uiStrings::phrSave(
		    uiStrings::sSet()),	mCB(this,uiBuildListFromList,saveCB) );
	savebut_->attach( alignedBelow, openbut );
    }

    if ( setup_.movable_ )
    {
	moveupbut_ = new uiToolButton( this, uiToolButton::UpArrow,
				       uiStrings::sMoveUp(),
				       mCB(this,uiBuildListFromList,moveCB) );
	moveupbut_->attach( alignedBelow, savebut_ ? savebut_ : rmbut_ );
	movedownbut_ = new uiToolButton( this, uiToolButton::DownArrow,
					 uiStrings::sMoveDown(),
					 mCB(this,uiBuildListFromList,moveCB) );
	movedownbut_->attach( alignedBelow, moveupbut_ );
    }

    setHAlignObj( deffld_ );
    defSelChg();
}


void uiBuildListFromList::setAvailable( const BufferStringSet& avnms )
{
    avfld_->setEmpty();
    avfld_->addItems( avnms );
}


void uiBuildListFromList::defSelChg()
{
    const int selidx = deffld_->currentItem();
    if ( !setup_.singleuse_ && selidx >= 0 )
	avfld_->setCurrentItem( avFromDef(deffld_->getText()) );

    const bool havesel = selidx >= 0;
    edbut_->setSensitive( havesel );
    rmbut_->setSensitive( havesel );
    if ( savebut_ )
	savebut_->setSensitive( havesel );
    if ( movedownbut_ )
    {
	const int sz = deffld_->size();
	const bool canmove = sz > 1;
	moveupbut_->setSensitive( canmove && selidx > 0 );
	movedownbut_->setSensitive( canmove && selidx < sz - 1 );
    }
}


void uiBuildListFromList::rmItm( int itmidx, bool dosignals )
{
    if ( itmidx < 0 || itmidx >= deffld_->size() )
	return;
    if ( setup_.singleuse_ )
	avfld_->insertItem(
		mToUiStringTodo(avFromDef(deffld_->textOfItem(itmidx))), 0 );

    deffld_->removeItem( itmidx );
    usrchg_ = true;

    if ( !dosignals )
	return;

    if ( itmidx >= deffld_->size() )
	itmidx = deffld_->size() - 1;
    if ( itmidx >= 0 )
	deffld_->setCurrentItem( itmidx );

    defSelChg(); // explicit because the UI selection hasn't changed
}


void uiBuildListFromList::removeItem()
{
    rmItm( deffld_->currentItem(), true );
}

void uiBuildListFromList::removeAll()
{
    while ( true )
    {
	const int sz = deffld_->size();
	const bool islast = sz < 2;
	rmItm( 0, islast );
	if ( islast )
	    break;
    }
}


void uiBuildListFromList::setItemName( const char* newnm )
{
    const int itmidx = deffld_->currentItem();
    if ( itmidx < 0 ) return;
    const BufferString orgnm( deffld_->textOfItem(itmidx) );
    if ( orgnm != newnm )
    {
	deffld_->setItemText( itmidx, mToUiStringTodo(newnm) );
	usrchg_ = true;
    }
}


void uiBuildListFromList::addItem( const char* itmnm )
{
    deffld_->addItem( mToUiStringTodo(itmnm) );
    const int itmidx = deffld_->size() - 1;
    if ( setup_.singleuse_ )
    {
	const int avidx = avfld_->indexOf( avFromDef(itmnm) );
	avfld_->removeItem( avidx );
    }
    deffld_->setCurrentItem( itmidx );
    usrchg_ = true;
}


void uiBuildListFromList::openCB( CallBacker* )
{
    if ( usrchg_ && !uiMSG().askGoOn(tr("Current work not saved. Continue?")) )
	return;

    if ( ioReq(false) )
	usrchg_ = false;
}


void uiBuildListFromList::saveCB( CallBacker* )
{
    if ( ioReq(true) )
	usrchg_ = false;
}


void uiBuildListFromList::moveCB( CallBacker* cb )
{
    const int sz = deffld_->size();
    if ( sz < 2 ) return;

    const int fromidx = deffld_->currentItem();
    const int toidx = cb == movedownbut_ ? fromidx + 1 : fromidx - 1;
    if ( toidx < 0 || toidx >= sz ) return;

    const char* fromtxt( deffld_->textOfItem(fromidx) );
    const char* totxt( deffld_->textOfItem(toidx) );
    deffld_->setItemText( fromidx, mToUiStringTodo(totxt) );
    deffld_->setItemText( toidx, mToUiStringTodo(fromtxt) );

    itemSwitch( fromtxt, totxt );
}



uiToolButton* uiBuildListFromList::lowestStdBut()
{
    return movedownbut_ ? movedownbut_ : (savebut_ ? savebut_ : rmbut_);
}


const char* uiBuildListFromList::curAvSel() const
{
    const int itmidx = avfld_->currentItem();
    return itmidx < 0 ? 0 : avfld_->textOfItem(itmidx);
}


const char* uiBuildListFromList::curDefSel() const
{
    const int itmidx = deffld_->currentItem();
    return itmidx < 0 ? 0 : deffld_->textOfItem(itmidx);
}


void uiBuildListFromList::setCurDefSel( const char* nm )
{
    deffld_->setCurrentItem( nm );
}
