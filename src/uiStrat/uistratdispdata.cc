/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratdispdata.cc,v 1.8 2010-07-14 10:05:13 cvsbruno Exp $";

#include "uistratdispdata.h"
#include "uistratmgr.h"
#include "uistratreftree.h"
#include "uilistview.h"

#include "bufstringset.h"
#include "color.h"
#include "stratunitrepos.h"


#define mAskStratMgrNotif(nm)\
    uistratmgr_.nm.notify( mCB(this,uiStratAnnotGather,triggerDataChange) );
uiStratAnnotGather::uiStratAnnotGather( AnnotData& ad, const uiStratMgr& mgr )
    : CallBacker(CallBacker::CallBacker()) 	
    , data_(ad) 
    , uistratmgr_(mgr) 
    , newtreeRead(this)  
{
    mAskStratMgrNotif(unitCreated)
    mAskStratMgrNotif(unitChanged)
    mAskStratMgrNotif(unitRemoved)
    mAskStratMgrNotif(lithChanged)
    mAskStratMgrNotif(lithRemoved)

    readFromTree();
}


void uiStratAnnotGather::triggerDataChange( CallBacker* )
{
    readFromTree();
}


void uiStratAnnotGather::readFromTree()
{
    data_.eraseData();
    static const char* colnms[] = { "Super Group", "Group", 
    				 "Formation", "Member", "Type", 0 };
#define mAddCol( title )\
    data_.addCol( new AnnotData::Column( title ) );
    for ( int idcol=0; colnms[idcol]; idcol++ )
	mAddCol( colnms[idcol] );

    addUnits( *((Strat::NodeUnitRef*)uistratmgr_.getCurTree()), 0 );
    newtreeRead.trigger();
}


void uiStratAnnotGather::addUnits( const Strat::NodeUnitRef& nur, int order ) 
{
    if ( order == 0 )
	order++;

    for ( int iref=0; iref<nur.nrRefs(); iref++ )
    {
	const Strat::UnitRef& ref = nur.ref( iref );
	if ( ref.isLeaf() )
	{
	    mDynamicCastGet(const Strat::LeafUnitRef*,lur,&ref);
	    if ( !lur ) continue;
	    if ( order )
		addUnit( *lur, order-1 );
	}
	else
	{
	    mDynamicCastGet(const Strat::NodeUnitRef*,chldnur,&ref);
	    if ( chldnur )
	    { 
		if ( order )
		    addUnit( *chldnur, order-1 );
		addUnits( *chldnur, order+1 ); 
	    }
	}
    }
}


void uiStratAnnotGather::addUnit( const Strat::UnitRef& uref, int order )
{
    Interval<float> timerg = uref.props().timerg_;
    AnnotData::Unit* unit = new AnnotData::Unit( uref.code(), 
	    				 	 timerg.start, 
						 timerg.stop );
    unit->col_ = uref.props().color_;
    unit->colidx_ = order;
    BufferString nm;
    Color col;
    const int lvlid = uref.props().lvlid_;
    if ( lvlid >= 0 )
	uistratmgr_.getLvlPropsByID( lvlid, nm, col );
    else 
	col = Color::Black();
    unit->annots_.add( nm );
    unit->annots_.add( toString( col.rgb() ) );
    unit->annots_.add( uref.description() );
    mDynamicCastGet(const Strat::LeafUnitRef*,un,&uref)
    unit->annots_.add( un ? uistratmgr_.getLithName( *un ) : "" );
    unit->id_ = uref.getID();
    if ( order >= data_.nrCols() ) return;
    data_.getCol(order)->units_ += unit;
}




uiStratTreeWriter::uiStratTreeWriter( uiStratRefTree& tree ) 
    : uitree_(tree)
{}

#define mGetLItem(t) uiListViewItem* lit = uitree_.listView()->findItem(t,0,false); if ( lit ) { uitree_.listView()->setCurrentItem(lit); } else return;

void uiStratTreeWriter::handleUnitMenu( const char* txt )
{
    if ( txt )
    {
	mGetLItem( txt )
	uitree_.handleMenu( lit );
    }
}


void uiStratTreeWriter::handleLvlMenu( int unid )
{
    if ( unid >=0 )
	uitree_.setUnitLvl( unid );
}


void uiStratTreeWriter::fillUndef( CallBacker* cb )
{
    uitree_.doSetUnconformities( cb );
}


