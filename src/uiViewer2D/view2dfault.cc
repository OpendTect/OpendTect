/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2008
________________________________________________________________________

-*/

#include "view2dfault.h"

#include "faulteditor.h"
#include "flatauxdataeditor.h"
#include "mpeengine.h"
#include "mpef3dflatvieweditor.h"
#include "seisdatapack.h"

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
    , knotenabled_(false)
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
	uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	ConstRefMan<FlatDataPack> fdp = vwr.obtainPack( true, true );
	mDynamicCastGet(const RegularFlatDataPack*,regfdp,fdp.ptr());
	mDynamicCastGet(const RandomFlatDataPack*,randfdp,fdp.ptr());
	if ( !regfdp && !randfdp )
	{
	    faulteds_ += 0;
	    continue;
	}

	MPE::Fault3DFlatViewEditor* faulted =
	    new MPE::Fault3DFlatViewEditor(
	     const_cast<uiFlatViewAuxDataEditor*>(auxdataeditors_[ivwr]),emid_);
	faulted->setMouseEventHandler(
		&vwr.rgbCanvas().scene().getMouseEventHandler() );
	faulted->enableKnots( true );
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


void VW2DFault::setTrcKeyZSampling( const TrcKeyZSampling& cs, bool upd )
{
    if ( upd )
	draw();
}


void VW2DFault::draw()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	const uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	ConstRefMan<FlatDataPack> fdp = vwr.obtainPack( true, true );
	mDynamicCastGet(const RegularFlatDataPack*,regfdp,fdp.ptr());
	mDynamicCastGet(const RandomFlatDataPack*,randfdp,fdp.ptr());
	if ( !regfdp && !randfdp ) continue;

	if ( faulteds_[ivwr] )
	{
	    if ( regfdp )
		faulteds_[ivwr]->setTrcKeyZSampling( regfdp->sampling() );

	    if ( randfdp )
	    {
		faulteds_[ivwr]->setPath( randfdp->getPath() );
		faulteds_[ivwr]->setRandomLineID( randfdp->getRandomLineID() );
		faulteds_[ivwr]->setFlatPosData( &randfdp->posData() );
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
	{
	    faulteds_[ivwr]->enableLine( yn );
	    faulteds_[ivwr]->enableKnots( knotenabled_ );
	}
    }
}


void VW2DFault::selected()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( faulteds_[ivwr] )
	{
	    uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	    const bool iseditable = vwr.appearance().annot_.editable_;
	    if ( iseditable )
		faulteds_[ivwr]->setMouseEventHandler(
			&vwr.rgbCanvas().scene().getMouseEventHandler() );
	    else
		faulteds_[ivwr]->setMouseEventHandler( 0 );
	    faulteds_[ivwr]->enableKnots( iseditable );
	    knotenabled_ = iseditable;
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

    knotenabled_ = false;
    deselted_.trigger();
}
