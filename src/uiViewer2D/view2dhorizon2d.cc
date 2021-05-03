/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
________________________________________________________________________

-*/

#include "view2dhorizon2d.h"

#include "emseedpicker.h"
#include "flatauxdataeditor.h"
#include "flatposdata.h"
#include "horflatvieweditor2d.h"
#include "mpeengine.h"
#include "seisdatapack.h"

#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatauxdataeditor.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"

mCreateVw2DFactoryEntry( Vw2DHorizon2D );

Vw2DHorizon2D::Vw2DHorizon2D( const EM::ObjectID& oid, uiFlatViewWin* win,
			const ObjectSet<uiFlatViewAuxDataEditor>& auxdataedtors)
    : Vw2DEMDataObject(oid,win,auxdataedtors)
    , geomid_(Survey::GeometryManager::cUndefGeomID())
    , deselted_( this )
    , vdselspec_( 0 )
    , wvaselspec_( 0 )
{
    horeds_.allowNull();
    if ( oid >= 0 )
	setEditors();
}


void Vw2DHorizon2D::setEditors()
{
    deepErase( horeds_ );

    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	const uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	ConstDataPackRef<RegularFlatDataPack> regfdp =
				vwr.obtainPack( true, true );
	if ( !regfdp || !regfdp->is2D() )
	{
	    horeds_ += 0;
	    continue;
	}
	else
	    geomid_ = regfdp->getTrcKey(0).geomID();

	MPE::HorizonFlatViewEditor2D* hored =
	    new MPE::HorizonFlatViewEditor2D(
	     const_cast<uiFlatViewAuxDataEditor*>(auxdataeditors_[ivwr]),emid_);
	hored->setLine2DInterSectionSet( line2dintersectionset_ );
	horeds_ += hored;
    }
}


Vw2DHorizon2D::~Vw2DHorizon2D()
{
    deepErase(horeds_);
}


void Vw2DHorizon2D::setTrcKeyZSampling( const TrcKeyZSampling& cs, bool upd )
{
    if ( upd )
	draw();
}


void Vw2DHorizon2D::setSelSpec( const Attrib::SelSpec* as, bool wva )
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


void Vw2DHorizon2D::setGeomID( Pos::GeomID geomid )
{
    geomid_ = geomid;
}


void Vw2DHorizon2D::draw()
{
    bool trackerenbed = false;
    if (  MPE::engine().getTrackerByObject(emid_) != -1 )
	trackerenbed = true;

    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	ConstDataPackRef<RegularFlatDataPack> regfdp =
				    vwr.obtainPack( true, true );
	if ( !regfdp ) continue;

	horeds_[ivwr]->setTrcKeyZSampling( regfdp->sampling() );
	horeds_[ivwr]->setSelSpec( wvaselspec_, true );
	horeds_[ivwr]->setSelSpec( vdselspec_, false );
	horeds_[ivwr]->setGeomID( regfdp->getTrcKey(0).geomID() );

	TypeSet<int>& trcnrs = horeds_[ivwr]->getPaintingCanvTrcNos();
	TypeSet<float>& dists = horeds_[ivwr]->getPaintingCanDistances();
	trcnrs.erase(); dists.erase();
	for ( int idx=0; idx<regfdp->nrTrcs(); idx++ )
	{
	    trcnrs += regfdp->getTrcKey(idx).trcNr();
	    dists += mCast(float,regfdp->posData().position(true,idx));
	}

	horeds_[ivwr]->setLine2DInterSectionSet( line2dintersectionset_ );
	horeds_[ivwr]->paint();
	horeds_[ivwr]->enableSeed( trackerenbed );
	horeds_[ivwr]->enableIntersectionMarker( true );
    }
}


void Vw2DHorizon2D::enablePainting( bool yn )
{
    for ( int idx=0; idx<horeds_.size(); idx++ )
    {
	if ( horeds_[idx] )
	{
	    horeds_[idx]->enableLine( yn );
	    horeds_[idx]->enableSeed( yn
		    && (MPE::engine().getTrackerByObject(emid_) != -1) );
	    horeds_[idx]->enableIntersectionMarker( yn );
	}
    }
}


void Vw2DHorizon2D::selected( bool enabled )
{
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
	    horeds_[ivwr]->enableSeed( trackerenbed && enabled );
	}
    }

    const int trackerid = MPE::engine().getTrackerByObject(emid_);
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );

    if ( !tracker ) return;
}


void Vw2DHorizon2D::setSeedPicking( bool ison )
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( horeds_[ivwr] )
	    horeds_[ivwr]->setSeedPicking( ison );
    }
}


void Vw2DHorizon2D::setTrackerSetupActive( bool ison )
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( horeds_[ivwr] )
	    horeds_[ivwr]->setTrackerSetupActive( ison );
    }
}


void Vw2DHorizon2D::triggerDeSel()
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


void Vw2DHorizon2D::getHorEditors(
		    ObjectSet<const MPE::HorizonFlatViewEditor2D>& eds ) const
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( horeds_[ivwr] )
	    eds += horeds_[ivwr];
    }
}
