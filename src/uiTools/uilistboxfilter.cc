/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jun 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uilistboxfilter.h"
#include "uilistbox.h"
#include "globexpr.h"


uiListBoxFilter::uiListBoxFilter( uiListBox& lb, bool above )
    : uiGenInput(lb.parent(),"Filter",StringInpSpec("*"))
    , lb_(lb)
    , newFilter(this)
{
    attach( above ? centeredAbove : centeredBelow, &lb_ );
    valuechanged.notify( mCB(this,uiListBoxFilter,filtChg) );
}


void uiListBoxFilter::setItems( const TypeSet<uiString>& itms )
{
    BufferStringSet nms;
    for ( int idx=0; idx<itms.size(); idx++ )
	nms.add( itms[idx].getFullString() );
    setItems( nms );
}


void uiListBoxFilter::setItems( const BufferStringSet& nms )
{
    if ( &nms != &availitems_ )
	availitems_ = nms;
    const BufferString cursel( lb_.getText() );
    lb_.setEmpty();

    const char* filt = text();
    GlobExpr ge( filt && *filt ? filt : "*" );
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	const char* itm = nms.get(idx).buf();
	if ( ge.matches(itm) )
	    lb_.addItem( itm );
    }

    if ( lb_.isPresent(cursel) )
	lb_.setCurrentItem( cursel );
    else if ( !lb_.isEmpty() )
	lb_.setCurrentItem( 0 );
}


void uiListBoxFilter::setFilter( const char* newfilt )
{
    setText( FixedString(newfilt).isEmpty() ? "*" : newfilt );
    setItems( availitems_ );
}


void uiListBoxFilter::filtChg( CallBacker* )
{
    setItems( availitems_ );
    newFilter.trigger();
}


int uiListBoxFilter::getCurrent() const
{
    const int selidx = lb_.currentItem();
    return selidx < 0 ? -1 : availitems_.indexOf( lb_.textOfItem(selidx) );
}


void uiListBoxFilter::getChosen( TypeSet<int>& idxs ) const
{
    idxs.setEmpty();
    TypeSet<int> chidxs;
    lb_.getChosen( chidxs );
    if ( chidxs.isEmpty() )
	return;

    for ( int idx=0; idx<chidxs.size(); idx++ )
    {
	const int chidx = chidxs[idx];
	idxs += availitems_.indexOf( lb_.textOfItem(chidx) );
    }
}
