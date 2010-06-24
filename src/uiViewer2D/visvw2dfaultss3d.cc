/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: visvw2dfaultss3d.cc,v 1.1 2010-06-24 08:41:01 cvsumesh Exp $
________________________________________________________________________

-*/

#include "visvw2dfaultss3d.h"

#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "flatauxdataeditor.h"
#include "mpefssflatvieweditor.h"

#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatauxdataeditor.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"


VW2DFautSS3D::VW2DFautSS3D( const EM::ObjectID& oid, uiFlatViewWin* mainwin,
			const ObjectSet<uiFlatViewAuxDataEditor>& auxdataeds )
    : Vw2DDataObject()
    , emid_(oid)
    , viewerwin_(mainwin)
    , auxdataeditors_(auxdataeds)
    , deselted_( this )
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	const FlatDataPack* fdp = viewerwin_->viewer( ivwr ).pack( true );
	if ( !fdp )
	    fdp = viewerwin_->viewer( ivwr ).pack( false );
	if ( !fdp )
	{
	    fsseds_ += 0;
	    continue;
	}

	mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,fdp);
	if ( !dp3d )
	{
	    fsseds_ += 0;
	    continue;
	}

	MPE::FaultStickSetFlatViewEditor* fssed = 
	    new MPE::FaultStickSetFlatViewEditor(
	     const_cast<uiFlatViewAuxDataEditor*>(auxdataeditors_[ivwr]), oid );
	fsseds_ += fssed;
    }
}


VW2DFautSS3D::~VW2DFautSS3D()
{
    deepErase(fsseds_);
}


void VW2DFautSS3D::setCubeSampling( const CubeSampling& cs, bool upd )
{
    if ( upd )
	draw();
}


void VW2DFautSS3D::draw()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	const FlatDataPack* fdp = vwr.pack( true );
	if ( !fdp )
	    fdp = vwr.pack( false );
	if ( !fdp ) continue;

	mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,fdp);
	if ( !dp3d ) continue;

	if ( fsseds_[ivwr] )
	{
	    fsseds_[ivwr]->setCubeSampling( dp3d->cube().cubeSampling() );
	    fsseds_[ivwr]->drawFault();
	}
    }
}


void VW2DFautSS3D::enablePainting( bool yn )
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( fsseds_[ivwr] )
	    fsseds_[ivwr]->enablePainting( yn );
    }
}


void VW2DFautSS3D::selected()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( fsseds_[ivwr] )
	{
	    uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	    fsseds_[ivwr]->setMouseEventHandler(
		    &vwr.rgbCanvas().scene().getMouseEventHandler() );
	    fsseds_[ivwr]->enableKnots( true );
	}
    }
}


void VW2DFautSS3D::triggerDeSel()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( fsseds_[ivwr] )
	{
	    fsseds_[ivwr]->setMouseEventHandler( 0 );
	    fsseds_[ivwr]->enableKnots( false );
	}
    }
    deselted_.trigger();
}
