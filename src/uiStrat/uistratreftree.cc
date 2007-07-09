/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          June 2007
 RCS:		$Id: uistratreftree.cc,v 1.2 2007-07-09 10:12:07 cvshelene Exp $
________________________________________________________________________

-*/

#include "uistratreftree.h"
#include "uilistview.h"
#include "stratunitrepos.h"

#define mAddCol(nm,wdth,nr) \
    lv_->addColumn( nm ); \
    lv_->setColumnWidthMode( nr, uiListView::Manual ); \
    lv_->setColumnWidth( nr, wdth )

uiStratRefTree::uiStratRefTree( uiParent* p, const Strat::RefTree* rt )
	: tree_(0)
{
    lv_ = new uiListView( p, "RefTree viewer" );
    mAddCol( "Unit", 300, 0 );
    mAddCol( "Name", 200, 1 );
    lv_->setPrefWidth( 500 );
    lv_->setStretch( 2, 2 );
    lv_->setTreeStepSize(30);

    setTree( rt );
}


uiStratRefTree::~uiStratRefTree()
{
    delete lv_;
}


void uiStratRefTree::setTree( const Strat::RefTree* rt )
{
    if ( rt == tree_ ) return;

    //TODO Empty the listview. How?

    tree_ = rt;
    if ( !tree_ ) return;

    addNode( 0, *tree_ );
}


void uiStratRefTree::addNode( uiListViewItem* parlvit,
			      const Strat::NodeUnitRef& nur )
{
    uiListViewItem* lvit = parlvit
		? new uiListViewItem( parlvit, uiListViewItem::Setup()
			.label(nur.code()).label(nur.description()) )
		: new uiListViewItem( lv_, nur.code() );

    for ( int iref=0; iref<nur.nrRefs(); iref++ )
    {
	const Strat::UnitRef& ref = nur.ref( iref );
	if ( ref.isLeaf() )
	{
	    mDynamicCastGet(const Strat::LeafUnitRef&,lur,ref);
	    new uiListViewItem( lvit, uiListViewItem::Setup()
				.label(lur.code()).label(lur.description()) );
	}
	else
	{
	    mDynamicCastGet(const Strat::NodeUnitRef&,chldnur,ref);
	    addNode( lvit, chldnur );
	}
    }
}


const Strat::UnitRef* uiStratRefTree::findUnit( const char* s ) const
{
    return tree_->find( s );
}
