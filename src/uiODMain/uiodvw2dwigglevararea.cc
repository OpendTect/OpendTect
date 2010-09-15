/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: uiodvw2dwigglevararea.cc,v 1.2 2010-09-15 10:13:56 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiodvw2dwigglevararea.h"

#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uilistview.h"
#include "uiodviewer2d.h"

#include "visvw2dseismic.h"
#include "visvw2ddataman.h"


uiODVW2DWiggleVarAreaTreeItem::uiODVW2DWiggleVarAreaTreeItem()
    : uiODVw2DTreeItem( "Wiggle Var Area" )
    , dpid_(DataPack::cNoID())
    , dummyview_(0)
{}


uiODVW2DWiggleVarAreaTreeItem::~uiODVW2DWiggleVarAreaTreeItem()
{
    if ( viewer2D()->viewwin()->nrViewers() )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
	vwr.dataChanged.remove(
		mCB(this,uiODVW2DWiggleVarAreaTreeItem,dataChangedCB) );
    }

    viewer2D()->dataMgr()->removeObject( dummyview_ );
}


bool uiODVW2DWiggleVarAreaTreeItem::init()
{
    if ( !viewer2D()->viewwin()->nrViewers() )
	return false;

    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);

    const DataPack* fdpw = vwr.pack( true );
    if ( fdpw )
	dpid_ = fdpw->id();
    const DataPack* fdpv = vwr.pack( false );

    vwr.dataChanged.notify(
	    mCB(this,uiODVW2DWiggleVarAreaTreeItem,dataChangedCB) );

    uilistviewitem_->setChecked( fdpw );
    uilistviewitem_->setCheckable( fdpv && dpid_!=DataPack::cNoID() );

    checkStatusChange()->notify(
	    mCB(this,uiODVW2DWiggleVarAreaTreeItem,checkCB) );

    dummyview_ = new VW2DSeis();
    viewer2D()->dataMgr()->addObject( dummyview_ );

    return true;
}


bool uiODVW2DWiggleVarAreaTreeItem::select()
{
    if ( !uilistviewitem_->isSelected() )
	return false;

    viewer2D()->dataMgr()->setSelected( dummyview_ );

    return true;
}


void uiODVW2DWiggleVarAreaTreeItem::checkCB( CallBacker* )
{
    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	DataPack::ID id = DataPack::cNoID();

	if ( isChecked() )
	    id = dpid_;

	viewer2D()->viewwin()->viewer(ivwr).usePack( true, id, false );
    }
}


void uiODVW2DWiggleVarAreaTreeItem::dataChangedCB( CallBacker* )
{
    if ( !viewer2D()->viewwin()->nrViewers() )
	return;

    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);

    const DataPack* fdpw = vwr.pack( true );

    const DataPack* fdpv = vwr.pack( false );
    
    uilistviewitem_->setChecked( fdpw );
    uilistviewitem_->setCheckable( fdpv &&
	    			   (dpid_!=DataPack::cNoID() || fdpw) );

    if ( fdpw )
	dpid_ = fdpw->id();
}
