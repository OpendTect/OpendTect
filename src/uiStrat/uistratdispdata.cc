/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratdispdata.cc,v 1.2 2010-05-07 12:50:46 cvsbruno Exp $";

#include "uistratdispdata.h"
#include "uistratmgr.h"
#include "uistratreftree.h"
#include "uilistview.h"

#include "bufstringset.h"
#include "color.h"
#include "stratlevel.h"
#include "stratunitrepos.h"


#define mAskStratMgrNotif(nm)\
    uistratmgr_.nm.notify( mCB(this,uiStratAnnotGather,triggerDataChange) );
uiStratAnnotGather::uiStratAnnotGather( AnnotData& ad, const uiStratMgr& mgr)
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
    addUnits( *uistratmgr_.getCurTree(), 0 );
    newtreeRead.trigger();
}


void uiStratAnnotGather::addUnits( const Strat::NodeUnitRef& nur, int order ) 
{
    for ( int iref=0; iref<nur.nrRefs(); iref++ )
    {
	const Strat::UnitRef& ref = nur.ref( iref );
	if ( ref.isLeaf() )
	{
	    addUnit( nur, order-1 );
	    mDynamicCastGet(const Strat::LeafUnitRef*,lur,&ref);
	    if ( !lur ) continue;
	    addUnit( *lur, order );
	}
	else
	{
	    mDynamicCastGet(const Strat::NodeUnitRef*,chldnur,&ref);
	    if ( chldnur )
	    { addUnits( *chldnur, order+1 ); }
	}
    }
}


void uiStratAnnotGather::addUnit( const Strat::UnitRef& uref, int order )
{
    if ( order<0 ) return;
    while ( order > (data_.nrCols()-1) )
    {
	BufferString title = "Unit";
	if ( order == 0 )
	    title =  "Formation";
	else if ( order == 1 )
	    title =  "Member";
	else if ( order == 2 ) 
	    title =  "Type";

	data_.addCol( new AnnotData::Column( title ) );
    }

    Color tcol, bcol; 
    Interval<float> ttimerg, btimerg;
    BufferString& tnm = *new BufferString(); 
    BufferString& bnm = *new BufferString();
    uistratmgr_.getNmsTBLvls( &uref, tnm, bnm );
    uistratmgr_.getLvlPars( tnm, ttimerg, tcol );
    uistratmgr_.getLvlPars( bnm, btimerg, bcol );
    Interval<float> timerg = uref.props().timerg_;

    AnnotData::Unit* unit = new AnnotData::Unit( uref.code(), 
	    				 	 timerg.start, 
						 timerg.stop );
    unit->col_ = uref.props().color_;
    unit->colidx_ = order;
    unit->annots_.add( BufferString(tnm) );
    unit->annots_.add( toString( tcol.rgb() ) );
    unit->annots_.add( BufferString(bnm) );
    unit->annots_.add( toString( bcol.rgb() ) );
    unit->annots_.add( uref.description() );
    mDynamicCastGet(const Strat::LeafUnitRef*,un,&uref)
    unit->annots_.add( un ? uistratmgr_.getLithName( *un ) : "" );
    data_.getCol(order)->units_ += unit;
}




uiStratTreeWriter::uiStratTreeWriter( uiStratRefTree& tree ) 
    : uitree_(tree)
{}


#define mGetLItem(t) uiListViewItem* lit = uitree_.listView()->findItem(t,0,false); if ( !lit ) return;
void uiStratTreeWriter::selBoundary( const char* txt )
{
    mGetLItem( txt )
    uitree_.listView()->setCurrentItem( lit );
    uitree_.selBoundary();
}


void uiStratTreeWriter::addUnit( const char* txt, bool subunit )
{
    if ( subunit )
    {
	mGetLItem( txt )
	uitree_.insertSubUnit(  lit );
    }
    else
	uitree_.insertSubUnit( 0 );
}


void uiStratTreeWriter::removeUnit( const char* txt )
{
    mGetLItem( txt )
    uitree_.removeUnit(  lit );
}


void uiStratTreeWriter::updateUnitProperties( const char* txt )
{
    mGetLItem( txt )
    uitree_.updateUnitProperties( lit );
}

