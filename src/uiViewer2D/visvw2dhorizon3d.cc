/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id: visvw2dhorizon3d.cc,v 1.1 2010-06-24 08:41:01 cvsumesh Exp $
________________________________________________________________________

-*/

#include "visvw2dhorizon3d.h"

#include "attribdatapack.h"
#include "flatauxdataeditor.h"
#include "emhorizonpainter3d.h"
#include "horflatvieweditor3d.h"
#include "mpeengine.h"

#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatauxdataeditor.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"


Vw2DHorizon3D::Vw2DHorizon3D( const EM::ObjectID& oid, uiFlatViewWin* mainwin,
			const ObjectSet<uiFlatViewAuxDataEditor>& auxdataedtors)
    : Vw2DDataObject()
    , emid_(oid)
    , viewerwin_(mainwin)
    , auxdataeditors_(auxdataedtors)
    , deselted_(this)
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	const FlatDataPack* fdp = viewerwin_->viewer( ivwr ).pack( true );
	if ( !fdp )
	    fdp = viewerwin_->viewer( ivwr ).pack( false );
	if ( !fdp )
	{
	    horpainters_ += 0;
	    horeds_ += 0;
	    continue;
	}

	mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,fdp);
	if ( !dp3d )
	{
	    horpainters_ += 0;
	    horeds_ += 0;
	    continue;
	}

	EM::HorizonPainter3D* hor =
	    new EM::HorizonPainter3D( viewerwin_->viewer(ivwr), oid );
	horpainters_ += hor;

	MPE::HorizonFlatViewEditor3D* hored =
	    new MPE::HorizonFlatViewEditor3D(
	     const_cast<uiFlatViewAuxDataEditor*>(auxdataeditors_[ivwr]), oid );
	horeds_ += hored;
    }
}


Vw2DHorizon3D::~Vw2DHorizon3D()
{
    deepErase(horpainters_);
    deepErase(horeds_);
}


void Vw2DHorizon3D::setCubeSampling( const CubeSampling& cs, bool upd )
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
	const FlatDataPack* fdp = vwr.pack( true );
	if ( !fdp )
	    fdp = vwr.pack( false );
	if ( !fdp ) continue;

	mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,fdp);
	if ( !dp3d ) continue;

	if ( horpainters_[ivwr] )
	{
	    horpainters_[ivwr]->setCubeSampling( dp3d->cube().cubeSampling() );
	    horpainters_[ivwr]->paint();
	    horpainters_[ivwr]->enableSeed( trackerenbed );
	}

	if ( horeds_[ivwr] )
	{
	    horeds_[ivwr]->setMouseEventHandler(
		    	&vwr.rgbCanvas().scene().getMouseEventHandler() );
	    horeds_[ivwr]->setCubeSampling( dp3d->cube().cubeSampling() );
	    horeds_[ivwr]->setSelSpec( wvaselspec_, true );
	    horeds_[ivwr]->setSelSpec( vdselspec_, false );
	}
    }
}


void Vw2DHorizon3D::enablePainting( bool yn )
{
    for ( int idx=0; idx<horpainters_.size(); idx++ )
    {
	if ( horpainters_[idx] )
	{
	    horpainters_[idx]->enableLine( yn );
	    horpainters_[idx]->enableSeed( yn );
	}
    }
}


void Vw2DHorizon3D::selected()
{
    bool trackerenbed = false;
    if (  MPE::engine().getTrackerByObject(emid_) != -1 )
	trackerenbed = true;

    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( horeds_[ivwr] )
	{
	    uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	    horeds_[ivwr]->setMouseEventHandler(
		    	&vwr.rgbCanvas().scene().getMouseEventHandler() );
	}

	if ( horpainters_[ivwr] )
	    horpainters_[ivwr]->enableSeed( trackerenbed );
    }
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
	    horeds_[ivwr]->setMouseEventHandler( 0 );

	if ( horpainters_[ivwr] )
	    horpainters_[ivwr]->enableSeed( false );
    }

    deselted_.trigger();
}
