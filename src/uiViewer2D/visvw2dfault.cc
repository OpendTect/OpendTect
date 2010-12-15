/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2008
 RCS:		$Id: visvw2dfault.cc,v 1.3 2010-12-15 12:03:13 cvsumesh Exp $
________________________________________________________________________

-*/

#include "visvw2dfault.h"

#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "faulteditor.h"
#include "flatauxdataeditor.h"
#include "mpeengine.h"
#include "mpef3dflatvieweditor.h"

#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatauxdataeditor.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"


VW2DFaut::VW2DFaut( const EM::ObjectID& oid, uiFlatViewWin* mainwin,
		    const ObjectSet<uiFlatViewAuxDataEditor>& auxdataeds )
    : Vw2DDataObject()
    , emid_(oid)
    , viewerwin_(mainwin)
    , auxdataeditors_(auxdataeds)
    , deselted_( this )
    , f3deditor_(0)
{
    faulteds_.allowNull();

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid_, true );
    mDynamicCastGet( MPE::FaultEditor*, f3deditor, editor.ptr() );
    f3deditor_ = f3deditor;
    if ( f3deditor_ )
	f3deditor_->ref();

    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	const FlatDataPack* fdp = viewerwin_->viewer( ivwr ).pack( true );
	if ( !fdp )
	    fdp = viewerwin_->viewer( ivwr ).pack( false );
	if ( !fdp )
	{
	    faulteds_ += 0;
	    continue;
	}

	mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,fdp);
	if ( !dp3d )
	{
	    faulteds_ += 0;
	    continue;
	}

	MPE::Fault3DFlatViewEditor* faulted =
	    new MPE::Fault3DFlatViewEditor(
	     const_cast<uiFlatViewAuxDataEditor*>(auxdataeditors_[ivwr]), oid );
	faulteds_ += faulted;
    }
}


VW2DFaut::~VW2DFaut()
{
    deepErase(faulteds_);
    if ( f3deditor_ )
    {
	f3deditor_->unRef();
	MPE::engine().removeEditor( emid_ );
    }
}


void VW2DFaut::setCubeSampling( const CubeSampling& cs, bool upd )
{
    if ( upd )
	draw();
}


void VW2DFaut::draw()
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

	if ( faulteds_[ivwr] )
	{
	    faulteds_[ivwr]->setCubeSampling( dp3d->cube().cubeSampling() );
	    faulteds_[ivwr]->drawFault();
	}
    }
}


void VW2DFaut::enablePainting( bool yn )
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( faulteds_[ivwr] )
	    faulteds_[ivwr]->enablePainting( yn );
    }
}


void VW2DFaut::selected()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( faulteds_[ivwr] )
	{
	    uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	    faulteds_[ivwr]->setMouseEventHandler(
		    &vwr.rgbCanvas().scene().getMouseEventHandler() );
	    faulteds_[ivwr]->enableKnots( true );
	}
    }
}


void VW2DFaut::triggerDeSel()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( faulteds_[ivwr] )
	{
	    faulteds_[ivwr]->setMouseEventHandler( 0 );
	    faulteds_[ivwr]->enableKnots( false );
	}
    }

    deselted_.trigger();
}
