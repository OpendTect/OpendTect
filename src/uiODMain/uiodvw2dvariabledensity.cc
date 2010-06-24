/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: uiodvw2dvariabledensity.cc,v 1.1 2010-06-24 08:57:00 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiodvw2dvariabledensity.h"

#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uilistview.h"
#include "uiodviewer2d.h"

#include "visvw2dseismic.h"
#include "visvw2ddataman.h"


uiODVW2DVariableDensityTreeItem::uiODVW2DVariableDensityTreeItem()
    : uiODVw2DTreeItem( "Variable Density" )
    , dpid_(DataPack::cNoID())
    , dummyview_(0)
    , viachkbox_(false)
{}


uiODVW2DVariableDensityTreeItem::~uiODVW2DVariableDensityTreeItem()
{
    if ( viewer2D()->viewwin()->nrViewers() )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
	vwr.dataChanged.remove(
		mCB(this,uiODVW2DVariableDensityTreeItem,dataChangedCB) );
    }

    viewer2D()->dataMgr()->removeObject( dummyview_ );
}


bool uiODVW2DVariableDensityTreeItem::init()
{
    if ( !viewer2D()->viewwin()->nrViewers() )
	return false;

    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);

    const DataPack* fdpw = vwr.pack( true );

    const DataPack* fdpv = vwr.pack( false );
    if ( fdpv )
	dpid_ = fdpv->id();

    vwr.dataChanged.notify(
	    mCB(this,uiODVW2DVariableDensityTreeItem,dataChangedCB) );

    uilistviewitem_->setCheckable( true );
    uilistviewitem_->setChecked( fdpw );
    uilistviewitem_->setCheckable( fdpw && fdpv );

    checkStatusChange()->notify(
	    mCB(this,uiODVW2DVariableDensityTreeItem,checkCB) );

    dummyview_ = new VW2DSeis();
    viewer2D()->dataMgr()->addObject( dummyview_ );

    return true;
}


bool uiODVW2DVariableDensityTreeItem::select()
{
    if ( !uilistviewitem_->isSelected() )
	return false;

    viewer2D()->dataMgr()->setSelected( dummyview_ );

    return true;
}


void uiODVW2DVariableDensityTreeItem::checkCB( CallBacker* )
{
    viachkbox_ = true;

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	DataPack::ID id = DataPack::cNoID();

	if ( isChecked() )
	    id = dpid_;

	viewer2D()->viewwin()->viewer(ivwr).usePack( false, id, false );
    }
}


void uiODVW2DVariableDensityTreeItem::dataChangedCB( CallBacker* )
{
    if ( !viewer2D()->viewwin()->nrViewers() )
	return;

    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    const DataPack* fdpw = vwr.pack( true );
    const DataPack* fdpv = vwr.pack( false );

    if ( !viachkbox_ )
    {
	uilistviewitem_->setCheckable( true );
	uilistviewitem_->setChecked( fdpv );
	uilistviewitem_->setCheckable( fdpw && fdpv );

	if ( fdpv )
	    dpid_ = fdpv->id();
    }

    viachkbox_ = false;
}
