/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uibuildlistfromlist.cc,v 1.2 2011-06-15 09:03:52 cvsbert Exp $";

#include "uibuildlistfromlist.h"
#include "uilistbox.h"
#include "uitoolbutton.h"


uiBuildListFromList::Setup::Setup( bool sing, const char* itmtp, bool mv )
    : singleuse_(sing)
    , movable_(mv)
    , itemtype_(itmtp)
{
    if ( itemtype_.isEmpty() )
	itemtype_ = "item";
    addtt_.add( "Add " ).add( itemtype_ );
    edtt_.add( "Edit " ).add( itemtype_ );
    rmtt_.add( "Remove " ).add( itemtype_ );
}


uiBuildListFromList::uiBuildListFromList( uiParent* p,
			const uiBuildListFromList::Setup& su, const char* nm )
    : uiGroup(p,nm?nm:"List-from-list build group")
    , setup_(su)
    , usrchg_(false)
    , movedownbut_(0)
{
    BufferString curtxt( "Available " ); curtxt.add(setup_.itemtype_).add("s");
    avfld_ = new uiListBox( this, curtxt );
    avfld_->doubleClicked.notify( mCB(this,uiBuildListFromList,addCB) );

    uiToolButton* addbut = new uiToolButton( this, uiToolButton::RightArrow,
		    setup_.addtt_, mCB(this,uiBuildListFromList,addCB) );
    addbut->attach( centeredRightOf, avfld_ );

    curtxt = "Defined "; curtxt.add(setup_.itemtype_).add("s");
    deffld_ = new uiListBox( this, curtxt );
    deffld_->attach( rightTo, avfld_ );
    deffld_->attach( ensureRightOf, addbut );
    deffld_->selectionChanged.notify(
	    		mCB(this,uiBuildListFromList,defSelChg) );
    deffld_->doubleClicked.notify( mCB(this,uiBuildListFromList,edCB) );

    edbut_ = new uiToolButton( this, "edit.png",
		    setup_.edtt_, mCB(this,uiBuildListFromList,edCB) );
    edbut_->attach( rightOf, deffld_ );
    rmbut_ = new uiToolButton( this, "trashcan.png",
		    setup_.rmtt_, mCB(this,uiBuildListFromList,rmCB) );
    rmbut_->attach( alignedBelow, edbut_ );

    //TODO implement move buttons and handling

    setHAlignObj( deffld_ );
    defSelChg(0);
}


void uiBuildListFromList::setAvailable( const BufferStringSet& avnms )
{
    avfld_->setEmpty();
    avfld_->addItems( avnms );
}


void uiBuildListFromList::defSelChg( CallBacker* )
{
    const int selidx = deffld_->currentItem();
    if ( !setup_.singleuse_ && selidx >= 0 )
	avfld_->setCurrentItem( avFromDef(deffld_->getText()) );

    const bool havesel = selidx >= 0;
    edbut_->setSensitive( havesel );
    rmbut_->setSensitive( havesel );
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
    if ( itmidx < 0 ) return;
    if ( setup_.singleuse_ )
	avfld_->insertItem( avFromDef(deffld_->textOfItem(itmidx)), 0 );

    deffld_->removeItem( itmidx );
    usrchg_ = true;

    if ( !dosignals )
	return;

    itmidx--; if ( itmidx < 0 ) itmidx = 0;
    deffld_->setCurrentItem( itmidx );
    defSelChg( 0 ); // explicit because the UI selection hasn't changed
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
	const bool islast = sz == 1;
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
	deffld_->setItemText( itmidx, newnm );
	usrchg_ = true;
    }
}


void uiBuildListFromList::addItem( const char* itmnm )
{
    deffld_->addItem( itmnm );
    const int itmidx = deffld_->size() - 1;
    if ( setup_.singleuse_ )
    {
	const int avidx = avfld_->indexOf( avFromDef(itmnm) );
	avfld_->removeItem( avidx );
    }
    deffld_->setCurrentItem( itmidx );
    usrchg_ = true;
}


uiToolButton* uiBuildListFromList::lowestStdBut()
{
    return movedownbut_ ? movedownbut_ : rmbut_;
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
