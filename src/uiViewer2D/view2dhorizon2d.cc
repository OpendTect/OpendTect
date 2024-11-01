/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "view2dhorizon2d.h"

#include "emseedpicker.h"
#include "flatauxdataeditor.h"
#include "flatposdata.h"
#include "horflatvieweditor2d.h"
#include "mpeengine.h"
#include "seisdatapack.h"
#include "view2ddataman.h"

#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatauxdataeditor.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"

namespace View2D
{

mImplStd( Horizon2D )

Horizon2D::Horizon2D( uiFlatViewWin* fvw,
			const ObjectSet<uiFlatViewAuxDataEditor>& auxdataedtors)
    : EMDataObject(fvw,auxdataedtors)
    , deselected_(this)
{
    horeds_.setNullAllowed();
}


Horizon2D::~Horizon2D()
{
    deepErase( horeds_ );
}


void Horizon2D::setEditors()
{
    deepErase( horeds_ );
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	const uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	ConstRefMan<RegularFlatDataPack> regfdp =
					 vwr.getPack( true, true ).get();
	if ( !regfdp || !regfdp->is2D() )
	{
	    horeds_ += nullptr;
	    continue;
	}
	else
	    geomid_ = regfdp->getTrcKey(0).geomID();

	auto* hored = new MPE::HorizonFlatViewEditor2D(
	     const_cast<uiFlatViewAuxDataEditor*>(auxdataeditors_[ivwr]),emid_);
	hored->setLine2DInterSectionSet( line2dintersectionset_ );
	horeds_ += hored;
    }
}


void Horizon2D::setTrcKeyZSampling( const TrcKeyZSampling& cs, bool upd )
{
    if ( upd )
	draw();
}


void Horizon2D::setSelSpec( const Attrib::SelSpec* as, bool wva )
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


void Horizon2D::setGeomID( const Pos::GeomID& geomid )
{
    geomid_ = geomid;
}


void Horizon2D::draw()
{
    const bool trackerenbed = MPE::engine().hasTracker( emid_ );
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	ConstRefMan<RegularFlatDataPack> regfdp =
					 vwr.getPack( true, true ).get();
	if ( !regfdp )
	    continue;

	horeds_[ivwr]->setTrcKeyZSampling( regfdp->sampling() );
	horeds_[ivwr]->setSelSpec( wvaselspec_, true );
	horeds_[ivwr]->setSelSpec( vdselspec_, false );
	horeds_[ivwr]->setGeomID( regfdp->getTrcKey(0).geomID() );

	TypeSet<int>& trcnrs = horeds_[ivwr]->getPaintingCanvTrcNos();
	TypeSet<float>& dists = horeds_[ivwr]->getPaintingCanDistances();
	trcnrs.erase(); dists.erase();
	for ( int idx=0; idx<regfdp->nrTrcs(); idx++ )
	{
	    const TrcKey tk = regfdp->getTrcKey(idx);
	    trcnrs += tk.trcNr();
	    dists += mCast(float,regfdp->posData().position(true,idx));
	}

	horeds_[ivwr]->setLine2DInterSectionSet( line2dintersectionset_ );
	horeds_[ivwr]->paint();
	horeds_[ivwr]->enableSeed( trackerenbed );
	horeds_[ivwr]->enableIntersectionMarker( true );
    }
}


void Horizon2D::enablePainting( bool yn )
{
    const bool trackerenbed = MPE::engine().hasTracker( emid_ );
    for ( int idx=0; idx<horeds_.size(); idx++ )
    {
	if ( horeds_[idx] )
	{
	    horeds_[idx]->enableLine( yn );
	    horeds_[idx]->enableSeed( yn && trackerenbed );
	    horeds_[idx]->enableIntersectionMarker( yn );
	}
    }
}


void Horizon2D::selected( bool enabled )
{
    const bool trackerenbed = MPE::engine().hasTracker( emid_ );
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( horeds_[ivwr] )
	{
	    uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	    if ( enabled )
		horeds_[ivwr]->setMouseEventHandler(
			&vwr.rgbCanvas().scene().getMouseEventHandler() );
	    else
		horeds_[ivwr]->setMouseEventHandler( nullptr );

	    horeds_[ivwr]->enableSeed( trackerenbed && enabled );
	}
    }
}


void Horizon2D::setSeedPicking( bool ison )
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( horeds_[ivwr] )
	    horeds_[ivwr]->setSeedPicking( ison );
    }
}


void Horizon2D::setTrackerSetupActive( bool ison )
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( horeds_[ivwr] )
	    horeds_[ivwr]->setTrackerSetupActive( ison );
    }
}


void Horizon2D::triggerDeSel()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( horeds_[ivwr] )
	{
	    horeds_[ivwr]->setMouseEventHandler( nullptr );
	    horeds_[ivwr]->enableSeed( horeds_[ivwr]->seedEnable() );
	}
    }

    deselected_.trigger();
}


void Horizon2D::getHorEditors(
		    ObjectSet<const MPE::HorizonFlatViewEditor2D>& eds ) const
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( horeds_[ivwr] )
	    eds += horeds_[ivwr];
    }
}

} // namespace View2D
