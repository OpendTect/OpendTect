/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
________________________________________________________________________

-*/

#include "view2dhorizon3d.h"

#include "emseedpicker.h"
#include "emtracker.h"
#include "flatauxdataeditor.h"
#include "horflatvieweditor3d.h"
#include "mpeengine.h"
#include "seisdatapack.h"

#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatauxdataeditor.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"


mImplStd( Vw2DHorizon3D )

Vw2DHorizon3D::Vw2DHorizon3D( uiFlatViewWin* fvw,
			const ObjectSet<uiFlatViewAuxDataEditor>& auxdataedtors)
    : Vw2DEMDataObject(fvw,auxdataedtors)
    , deselted_(this)
    , vdselspec_(0)
    , wvaselspec_(0)
{
    horeds_.allowNull();
}


void Vw2DHorizon3D::setEditors()
{
    deepErase( horeds_ );
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	ConstRefMan<FlatDataPack> fdp =
			viewerwin_->viewer(ivwr).getPack( true, true ).get();
	mDynamicCastGet(const RegularFlatDataPack*,regfdp,fdp.ptr());
	mDynamicCastGet(const RandomFlatDataPack*,randfdp,fdp.ptr());
	if ( !regfdp && !randfdp )
	{
	    horeds_ += 0;
	    continue;
	}

	MPE::HorizonFlatViewEditor3D* hored =
	    new MPE::HorizonFlatViewEditor3D(
	     const_cast<uiFlatViewAuxDataEditor*>(auxdataeditors_[ivwr]),emid_);
	horeds_ += hored;
    }
}


Vw2DHorizon3D::~Vw2DHorizon3D()
{
    deepErase(horeds_);
}


void Vw2DHorizon3D::setTrcKeyZSampling( const TrcKeyZSampling& cs, bool upd )
{
    if ( upd )
	draw();
}


void Vw2DHorizon3D::setSelSpec( const Attrib::SelSpec* as, bool wva )
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( wva )
	{
	    wvaselspec_ = as;
	    if ( horeds_[ivwr] )
		horeds_[ivwr]->setSelSpec( wvaselspec_, true );
	}
	else
	{
	    vdselspec_ = as;
	    if ( horeds_[ivwr] )
		horeds_[ivwr]->setSelSpec( vdselspec_, false );
	}
    }
}


void Vw2DHorizon3D::draw()
{
    bool trackerenbed = false;
    if ( MPE::engine().getTrackerByObject(emid_) != -1 )
	trackerenbed = true;

    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	ConstRefMan<FlatDataPack> fdp = vwr.getPack( true, true ).get();
	mDynamicCastGet(const RegularFlatDataPack*,regfdp,fdp.ptr());
	mDynamicCastGet(const RandomFlatDataPack*,randfdp,fdp.ptr());
	if ( !regfdp && !randfdp ) continue;

	if ( horeds_[ivwr] )
	{
	    if ( regfdp )
		horeds_[ivwr]->setTrcKeyZSampling( regfdp->sampling() );

	    if ( randfdp )
	    {
		TrcKeyZSampling tkzs( false );
		const TrcKeyPath& tkpath = randfdp->getPath();
		for ( int ipath=0; ipath<tkpath.size(); ipath++ )
		    tkzs.hsamp_.include( tkpath[ipath] );

		tkzs.zsamp_ = randfdp->zRange();
		horeds_[ivwr]->setTrcKeyZSampling( tkzs );
		horeds_[ivwr]->setPath( randfdp->getPath() );
		horeds_[ivwr]->setFlatPosData( &randfdp->posData() );
	    }

	    horeds_[ivwr]->setSelSpec( wvaselspec_, true );
	    horeds_[ivwr]->setSelSpec( vdselspec_, false );
	    horeds_[ivwr]->paint();
	    horeds_[ivwr]->enableSeed( trackerenbed );
	}
    }
}


void Vw2DHorizon3D::enablePainting( bool yn )
{
    for ( int idx=0; idx<horeds_.size(); idx++ )
    {
	if ( horeds_[idx] )
	{
	    horeds_[idx]->enableLine( yn );
	    horeds_[idx]->enableSeed( yn &&
		    (MPE::engine().getTrackerByObject(emid_) != -1) );
	}
    }
}


void Vw2DHorizon3D::selected( bool enabled )
{
    bool setenableseed = true;
    MPE::EMTracker* activetracker = MPE::engine().getActiveTracker();
    if ( activetracker )
    {
	MPE::EMSeedPicker* seedpicker = activetracker->getSeedPicker(true);
	if ( seedpicker &&
	     (seedpicker->getTrackMode()==MPE::EMSeedPicker::DrawBetweenSeeds ||
	      seedpicker->getTrackMode()==MPE::EMSeedPicker::DrawAndSnap) )
	    setenableseed = false;
	else
	    setenableseed = true;
    }
    else
	setenableseed = false;

    bool trackerenbed = false;
    if (  MPE::engine().getTrackerByObject(emid_) != -1 )
	trackerenbed = true;

    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( horeds_[ivwr] )
	{
	    uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	    if ( enabled )
		horeds_[ivwr]->setMouseEventHandler(
			&vwr.rgbCanvas().scene().getMouseEventHandler() );
	    else
		horeds_[ivwr]->setMouseEventHandler( 0 );
	    if ( setenableseed )
		horeds_[ivwr]->enableSeed( trackerenbed && enabled );
	    else
		horeds_[ivwr]->enableSeed( horeds_[ivwr]->seedEnable() );
	}
    }

    const int trackerid = MPE::engine().getTrackerByObject(emid_);
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );

    if ( !tracker ) return;
}


void Vw2DHorizon3D::setSeedPicking( bool ison )
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( horeds_[ivwr] )
	    horeds_[ivwr]->setSeedPicking( ison );
    }
}


void Vw2DHorizon3D::setTrackerSetupActive( bool ison )
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( horeds_[ivwr] )
	    horeds_[ivwr]->setTrackerSetupActive( ison );
    }
}


void Vw2DHorizon3D::triggerDeSel()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( horeds_[ivwr] )
	{
	    horeds_[ivwr]->setMouseEventHandler( 0 );
	    horeds_[ivwr]->enableSeed( horeds_[ivwr]->seedEnable() );
	}
    }

    deselted_.trigger();
}


void Vw2DHorizon3D::getHorEditors(
		    ObjectSet<const MPE::HorizonFlatViewEditor3D>& eds ) const
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( horeds_[ivwr] )
	eds += horeds_[ivwr];
    }
}
