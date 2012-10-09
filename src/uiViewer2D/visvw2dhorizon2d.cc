/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "visvw2dhorizon2d.h"

#include "attribdataholder.h"
#include "attribdatapack.h"
#include "emseedpicker.h"
#include "flatauxdataeditor.h"
#include "horflatvieweditor2d.h"
#include "mpeengine.h"

#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatauxdataeditor.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"

mCreateVw2DFactoryEntry( Vw2DHorizon2D );

Vw2DHorizon2D::Vw2DHorizon2D( const EM::ObjectID& oid, uiFlatViewWin* win,
			const ObjectSet<uiFlatViewAuxDataEditor>& auxdataedtors)
    : Vw2DEMDataObject(oid,win,auxdataedtors)
    , linenm_(0)
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
	const FlatDataPack* fdp = viewerwin_->viewer( ivwr ).pack( true );
	if ( !fdp )
	    fdp = viewerwin_->viewer( ivwr ).pack( false );
	if ( !fdp ) 
	{ 
	    horeds_ += 0;
	    continue;
	}

	mDynamicCastGet(const Attrib::Flat2DDHDataPack*,dp2ddh,fdp);
	if ( !dp2ddh ) 
	{ 
	    horeds_ += 0;
	    continue; 
	}

	MPE::HorizonFlatViewEditor2D* hored = 
	    new MPE::HorizonFlatViewEditor2D( 
	     const_cast<uiFlatViewAuxDataEditor*>(auxdataeditors_[ivwr]),emid_);
	horeds_ += hored;
    }	
}


Vw2DHorizon2D::~Vw2DHorizon2D()
{ 
    deepErase(horeds_);
}


void Vw2DHorizon2D::setCubeSampling( const CubeSampling& cs, bool upd )
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


void Vw2DHorizon2D::setLineName( const char* ln )
{
    linenm_ = ln;
}


void Vw2DHorizon2D::draw()
{
    bool trackerenbed = false;
    if (  MPE::engine().getTrackerByObject(emid_) != -1 )
	trackerenbed = true;

    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	const FlatDataPack* fdp = vwr.pack( true );
	if ( !fdp )
	    fdp = vwr.pack( false );
	if ( !fdp ) continue;

	 mDynamicCastGet(const Attrib::Flat2DDHDataPack*,dp2ddh,fdp);
	 if ( !dp2ddh ) continue;

	 if ( horeds_[ivwr] )
	     horeds_[ivwr]->setMouseEventHandler(
	     		&vwr.rgbCanvas().scene().getMouseEventHandler() );
	 horeds_[ivwr]->setCubeSampling(dp2ddh->dataarray()->cubesampling_);
	 horeds_[ivwr]->setSelSpec( wvaselspec_, true );
	 horeds_[ivwr]->setSelSpec( vdselspec_, false );
	 horeds_[ivwr]->setLineSetID( lsetid_ );
	 BufferString* bs = new BufferString(); //TODO remove new
	 dp2ddh->getLineName( *bs );
	 horeds_[ivwr]->setLineName( *bs );
	 dp2ddh->getPosDataTable( horeds_[ivwr]->getPaintingCanvTrcNos(),
		 		  horeds_[ivwr]->getPaintingCanDistances() );
	 horeds_[ivwr]->paint();
	 horeds_[ivwr]->enableSeed( trackerenbed );
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
	    horeds_[ivwr]->enableSeed( false );
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
