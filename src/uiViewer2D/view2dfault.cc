/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "view2dfault.h"

#include "emfault3d.h"
#include "emmanager.h"
#include "flatauxdataeditor.h"
#include "mpeengine.h"
#include "mpef3dflatvieweditor.h"
#include "seisdatapack.h"
#include "view2ddataman.h"

#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatauxdataeditor.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"

namespace View2D
{

mImplStd( Fault )

Fault::Fault( uiFlatViewWin* fvw,
		    const ObjectSet<uiFlatViewAuxDataEditor>& auxdataeds )
    : EMDataObject(fvw,auxdataeds)
    , deselected_(this)
{
    faulteds_.setNullAllowed();
}


Fault::~Fault()
{
    deepErase( faulteds_ );
}


void Fault::setEditors()
{
    deepErase( faulteds_ );
    if ( MPE::engine().hasEditor(emid_) )
    {
	RefMan<MPE::ObjectEditor> objeditor =
					MPE::engine().getEditorByID( emid_ );
	f3deditor_ = dynamic_cast<MPE::FaultEditor*>( objeditor.ptr() );
    }

    if ( !f3deditor_ )
    {
	RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid_ );
	mDynamicCastGet(EM::Fault3D*,fault,emobject.ptr());
	f3deditor_ = fault ? MPE::FaultEditor::create( *fault ) : nullptr;
	if ( f3deditor_ )
	    MPE::engine().addEditor( *f3deditor_.ptr() );
    }

    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	ConstRefMan<FlatDataPack> fdp = vwr.getPack( true, true ).get();
	mDynamicCastGet(const RegularFlatDataPack*,regfdp,fdp.ptr());
	mDynamicCastGet(const RandomFlatDataPack*,randfdp,fdp.ptr());
	if ( !regfdp && !randfdp )
	{
	    faulteds_ += nullptr;
	    continue;
	}

	auto* faulted = new MPE::Fault3DFlatViewEditor(
	     const_cast<uiFlatViewAuxDataEditor*>(auxdataeditors_[ivwr]),emid_);
	faulted->setMouseEventHandler(
			    &vwr.rgbCanvas().scene().getMouseEventHandler() );
	faulted->enableKnots( true );
	faulteds_ += faulted;
    }
}


void Fault::setTrcKeyZSampling( const TrcKeyZSampling& cs, bool upd )
{
    if ( upd )
	draw();
}


void Fault::draw()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	const uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	ConstRefMan<FlatDataPack> fdp = vwr.getPack( true, true ).get();
	mDynamicCastGet(const RegularFlatDataPack*,regfdp,fdp.ptr());
	mDynamicCastGet(const RandomFlatDataPack*,randfdp,fdp.ptr());
	if ( !regfdp && !randfdp )
	    continue;

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


void Fault::enablePainting( bool yn )
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


void Fault::selected()
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
		faulteds_[ivwr]->setMouseEventHandler( nullptr );

	    faulteds_[ivwr]->enableKnots( iseditable );
	    knotenabled_ = iseditable;
	}
    }
}


void Fault::triggerDeSel()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( faulteds_[ivwr] )
	{
	    faulteds_[ivwr]->setMouseEventHandler( nullptr );
	    faulteds_[ivwr]->enableKnots( false );
	}
    }

    knotenabled_ = false;
    deselected_.trigger();
}

} // namespace View2D
