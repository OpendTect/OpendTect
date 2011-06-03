/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: visvw2dfaultss3d.cc,v 1.6 2011-06-03 14:10:26 cvsbruno Exp $
________________________________________________________________________

-*/

#include "visvw2dfaultss3d.h"

#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "emeditor.h"
#include "flatauxdataeditor.h"
#include "faultstickseteditor.h"
#include "mpeengine.h"
#include "mpefssflatvieweditor.h"

#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatauxdataeditor.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"

mCreateVw2DFactoryEntry( VW2DFaultSS3D );

VW2DFaultSS3D::VW2DFaultSS3D( const EM::ObjectID& oid, uiFlatViewWin* win,
			const ObjectSet<uiFlatViewAuxDataEditor>& auxdataeds )
    : Vw2DEMDataObject(oid,win,auxdataeds)
    , deselted_( this )
    , fsseditor_(0)
{
    fsseds_.allowNull();

    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid_, true );
    mDynamicCastGet( MPE::FaultStickSetEditor*, fsseditor, editor.ptr() );
    fsseditor_ = fsseditor;
    if ( fsseditor_ )
	fsseditor_->ref();

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


VW2DFaultSS3D::~VW2DFaultSS3D()
{
    deepErase(fsseds_);
    if ( fsseditor_ )
    {
	fsseditor_->unRef();
	MPE::engine().removeEditor( emid_ );
    }
}


void VW2DFaultSS3D::setCubeSampling( const CubeSampling& cs, bool upd )
{
    if ( upd )
	draw();
}


void VW2DFaultSS3D::draw()
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


void VW2DFaultSS3D::enablePainting( bool yn )
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( fsseds_[ivwr] )
	    fsseds_[ivwr]->enablePainting( yn );
    }
}


void VW2DFaultSS3D::selected( bool enabled )
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( fsseds_[ivwr] )
	{
	    uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	    if ( enabled )
		fsseds_[ivwr]->setMouseEventHandler(
			&vwr.rgbCanvas().scene().getMouseEventHandler() );
	    else
		fsseds_[ivwr]->setMouseEventHandler( 0 );
	    fsseds_[ivwr]->enableKnots( true && enabled );
	}
    }
}


void VW2DFaultSS3D::triggerDeSel()
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
