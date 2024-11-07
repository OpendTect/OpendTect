/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "view2dfaultss3d.h"

#include "emfaultstickset.h"
#include "emmanager.h"
#include "flatauxdataeditor.h"
#include "faultstickseteditor.h"
#include "mpeengine.h"
#include "mpefssflatvieweditor.h"
#include "seisdatapack.h"
#include "view2ddataman.h"

#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatauxdataeditor.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"

namespace View2D
{

mImplStd( FaultSS3D )

FaultSS3D::FaultSS3D( uiFlatViewWin* fvw,
			const ObjectSet<uiFlatViewAuxDataEditor>& auxdataeds )
    : EMDataObject(fvw,auxdataeds)
    , deselected_(this)
{
    fsseds_.setNullAllowed();
}


FaultSS3D::~FaultSS3D()
{
    deepErase( fsseds_ );
}


void FaultSS3D::setEditors()
{
    if ( MPE::engine().hasEditor(emid_) )
    {
	RefMan<MPE::ObjectEditor> objeditor =
					MPE::engine().getEditorByID( emid_ );
	fsseditor_ = dynamic_cast<MPE::FaultStickSetEditor*>( objeditor.ptr() );
    }

    if ( !fsseditor_ )
    {
	RefMan<EM::EMObject> emobject = EM::EMM().getObject( emid_ );
	mDynamicCastGet(EM::FaultStickSet*,fss,emobject.ptr());
	fsseditor_ = fss ? MPE::FaultStickSetEditor::create( *fss ) : nullptr;
	if ( fsseditor_ )
	    MPE::engine().addEditor( *fsseditor_.ptr() );
    }

    deepErase( fsseds_ );
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	const uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	ConstRefMan<FlatDataPack> fdp = vwr.getPack( true, true ).get();
	mDynamicCastGet(const RegularFlatDataPack*,regfdp,fdp.ptr());
	mDynamicCastGet(const RandomFlatDataPack*,randfdp,fdp.ptr());
	if ( !regfdp && !randfdp )
	{
	    fsseds_ += nullptr;
	    continue;
	}

	auto* fssed = new MPE::FaultStickSetFlatViewEditor(
	     const_cast<uiFlatViewAuxDataEditor*>(auxdataeditors_[ivwr]),emid_);
	fsseds_ += fssed;
    }
}


void FaultSS3D::setTrcKeyZSampling( const TrcKeyZSampling& cs, bool upd )
{
    if ( upd )
	draw();
}


void FaultSS3D::draw()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	const uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	ConstRefMan<FlatDataPack> fdp = vwr.getPack( true, true ).get();
	mDynamicCastGet(const RegularFlatDataPack*,regfdp,fdp.ptr());
	mDynamicCastGet(const RandomFlatDataPack*,randfdp,fdp.ptr());
	if ( !regfdp && !randfdp )
	    continue;

	if ( fsseds_[ivwr] )
	{
	    if ( regfdp )
		fsseds_[ivwr]->setTrcKeyZSampling( regfdp->sampling() );

	    if ( randfdp )
	    {
		fsseds_[ivwr]->setPath( randfdp->getPath() );
		fsseds_[ivwr]->setRandomLineID( randfdp->getRandomLineID() );
		fsseds_[ivwr]->setFlatPosData( &randfdp->posData() );
	    }

	    fsseds_[ivwr]->drawFault();
	}
    }
}


void FaultSS3D::enablePainting( bool yn )
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( fsseds_[ivwr] )
	{
	    fsseds_[ivwr]->enableLine( yn );
	    fsseds_[ivwr]->enableKnots( knotenabled_ );
	}
    }
}


void FaultSS3D::selected()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( fsseds_[ivwr] )
	{
	    uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	    const bool iseditable = vwr.appearance().annot_.editable_;
	    if ( iseditable )
		fsseds_[ivwr]->setMouseEventHandler(
			&vwr.rgbCanvas().scene().getMouseEventHandler() );
	    else
		fsseds_[ivwr]->setMouseEventHandler( nullptr );

	    fsseds_[ivwr]->enableKnots( iseditable );
	    knotenabled_ = iseditable;
	}
    }
}


void FaultSS3D::triggerDeSel()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( fsseds_[ivwr] )
	{
	    fsseds_[ivwr]->setMouseEventHandler( nullptr );
	    fsseds_[ivwr]->enableKnots( false );
	}
    }

    knotenabled_ = false;
    deselected_.trigger();
}

} // namespace View2D
