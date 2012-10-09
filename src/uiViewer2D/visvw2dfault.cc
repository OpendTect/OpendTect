/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2008
 RCS:		$Id$
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


mCreateVw2DFactoryEntry( VW2DFault );


VW2DFault::VW2DFault( const EM::ObjectID& oid, uiFlatViewWin* win,
		    const ObjectSet<uiFlatViewAuxDataEditor>& auxdataeds )
    : Vw2DEMDataObject(oid,win,auxdataeds)
    , deselted_( this )
    , f3deditor_(0)
{
    faulteds_.allowNull();
    if ( oid >= 0 )
	setEditors();
}


void VW2DFault::setEditors()
{
    deepErase( faulteds_ );
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
	mDynamicCastGet(const Attrib::FlatRdmTrcsDataPack*,dprdm,fdp)
	if ( !(dp3d||dprdm) )
	{
	    faulteds_ += 0;
	    continue;
	}

	MPE::Fault3DFlatViewEditor* faulted =
	    new MPE::Fault3DFlatViewEditor(
	     const_cast<uiFlatViewAuxDataEditor*>(auxdataeditors_[ivwr]),emid_);
	faulteds_ += faulted;
    }
}


VW2DFault::~VW2DFault()
{
    deepErase(faulteds_);
    if ( f3deditor_ )
    {
	f3deditor_->unRef();
	MPE::engine().removeEditor( emid_ );
    }
}


void VW2DFault::setCubeSampling( const CubeSampling& cs, bool upd )
{
    if ( upd )
	draw();
}


void VW2DFault::draw()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	const FlatDataPack* fdp = vwr.pack( true );
	if ( !fdp )
	    fdp = vwr.pack( false );
	if ( !fdp ) continue;

	mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,fdp);
	mDynamicCastGet(const Attrib::FlatRdmTrcsDataPack*,dprdm,fdp)
	if ( !(dp3d||dprdm) ) continue;

	if ( faulteds_[ivwr] )
	{
	    if ( dp3d )
		faulteds_[ivwr]->setCubeSampling( dp3d->cube().cubeSampling() );

	    if ( dprdm )
	    {
		faulteds_[ivwr]->setPath( dprdm->pathBIDs() );
		faulteds_[ivwr]->setFlatPosData( &dprdm->posData() );
	    }

	    faulteds_[ivwr]->drawFault();
	}
    }
}


void VW2DFault::enablePainting( bool yn )
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( faulteds_[ivwr] )
	    faulteds_[ivwr]->enablePainting( yn );
    }
}


void VW2DFault::selected()
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


void VW2DFault::triggerDeSel()
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
