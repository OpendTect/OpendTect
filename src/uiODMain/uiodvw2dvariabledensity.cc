/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: uiodvw2dvariabledensity.cc,v 1.3 2010-08-18 06:57:51 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiodvw2dvariabledensity.h"

#include "uicolortable.h"
#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewcoltabed.h"
#include "uilistview.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"


#include "coltabsequence.h"
#include "pixmap.h"
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
    uilistviewitem_->setChecked( fdpv );
    uilistviewitem_->setCheckable( fdpw && fdpv );

    checkStatusChange()->notify(
	    mCB(this,uiODVW2DVariableDensityTreeItem,checkCB) );

    dummyview_ = new VW2DSeis();
    viewer2D()->dataMgr()->addObject( dummyview_ );

    if ( fdpv )
    {
	if ( !vwr.control() )
	    displayMiniCtab(0);
	
	mDynamicCastGet( uiFlatViewStdControl*, fltvwctrl, vwr.control() );
	if ( fltvwctrl && !fltvwctrl->colTabEd() )
	    displayMiniCtab(0);

	uiFlatViewColTabEd* ctabed = fltvwctrl->colTabEd();
	if ( !ctabed->colTabGrp() )
	    displayMiniCtab(0);

	mDynamicCastGet( uiColorTable*, uicoltab, ctabed->colTabGrp() );

	if ( uicoltab )
	    displayMiniCtab( &uicoltab->colTabSeq() );
    }

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

    displayMiniCtab(0);

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

    if ( !fdpv )
	displayMiniCtab(0);
    else
    {
	if ( !vwr.control() )
	    displayMiniCtab(0);

	ColTab::Sequence seq( vwr.appearance().ddpars_.vd_.ctab_ );
	displayMiniCtab( &seq );
    }

    viachkbox_ = false;
}


void uiODVW2DVariableDensityTreeItem::displayMiniCtab(
						const ColTab::Sequence* seq )
{
    if ( !seq )
    {
	uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );
	return;
    }

    PtrMan<ioPixmap> pixmap = new ioPixmap( *seq, cPixmapWidth(),
	    				    cPixmapHeight(), true );
    uilistviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(), *pixmap );
}
