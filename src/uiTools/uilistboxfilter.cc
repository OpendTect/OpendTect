/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uilistboxfilter.h"
#include "uilistbox.h"
#include "globexpr.h"


uiListBoxFilter::uiListBoxFilter( uiListBox& lb, bool above )
    : uiGenInput(&lb,uiStrings::sFilter(),StringInpSpec(""))
    , newFilter(this)
    , lb_(lb)
{
    if ( lb_.isMultiChoice() && above )
	attach( rightOf, lb_.checkGroup() );
    else
	attach( above ? centeredAbove : centeredBelow, lb_.box() );

    valuechanged.notify( mCB(this,uiListBoxFilter,filtChg) );
}


uiListBoxFilter::~uiListBoxFilter()
{}


void uiListBoxFilter::setItems( const uiStringSet& itms )
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
    BufferStringSet chosennms;
    getChosen( chosennms );
    lb_.setEmpty();

    BufferString filt = text();
    GlobExpr::validateFilterString( filt );
    GlobExpr ge( filt.buf() );
    for ( int idx=0; idx<nms.size(); idx++ )
    {
	const char* itm = nms.get(idx).buf();
	if ( ge.matches(itm) )
	    lb_.addItem( toUiString(itm) );
    }

    if ( lb_.isPresent(cursel) )
	lb_.setCurrentItem( cursel.buf() );
    else if ( !lb_.isEmpty() )
	lb_.setCurrentItem( 0 );

    if ( !chosennms.isEmpty() )
	lb_.setChosen( chosennms );

    lb_.itemChosen.trigger( -1 );
}


void uiListBoxFilter::setFilter( const char* newfilt )
{
    setText( StringView(newfilt).isEmpty() ? "*" : newfilt );
    setItems( availitems_ );
}


void uiListBoxFilter::setEmpty()
{
    lb_.setEmpty();
    availitems_.erase();
    setFilter( "" );
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


void uiListBoxFilter::getChosen( BufferStringSet& nms ) const
{
    lb_.getChosen( nms );
}


int uiListBoxFilter::nrChosen() const
{
    return lb_.nrChosen();
}


void uiListBoxFilter::removeItem( int idx )
{
    availitems_.removeSingle( idx );
    setItems( availitems_ );
}
