/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "view2dfaultss2d.h"

#include "emfaultstickset.h"
#include "emmanager.h"
#include "flatauxdataeditor.h"
#include "flatposdata.h"
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

mImplStd( FaultSS2D )

FaultSS2D::FaultSS2D( uiFlatViewWin* fvw,
		      const ObjectSet<uiFlatViewAuxDataEditor>& auxdataeds )
    : EMDataObject(fvw,auxdataeds)
    , deselected_(this)
{
    fsseds_.setNullAllowed();
}


FaultSS2D::~FaultSS2D()
{
    deepErase(fsseds_);
}


void FaultSS2D::setEditors()
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
	ConstRefMan<RegularSeisFlatDataPack> regfdp =
						vwr.getPack( true,true).get();
	if ( !regfdp )
	{
	    fsseds_ += nullptr;
	    continue;
	}

	auto* fssed = new MPE::FaultStickSetFlatViewEditor(
	     const_cast<uiFlatViewAuxDataEditor*>(auxdataeditors_[ivwr]),emid_);
	fsseds_ += fssed;
    }
}


void FaultSS2D::draw()
{
    ConstRefMan<Survey::Geometry> geometry = Survey::GM().getGeometry( geomid_);
    if ( !geometry )
	return;

    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	const uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	ConstRefMan<RegularSeisFlatDataPack> regfdp =
					 vwr.getPack( true, true ).get();
	if ( !regfdp ) continue;

	if ( fsseds_[ivwr] )
	{
	    TypeSet<int>& trcnrs = fsseds_[ivwr]->getTrcNos();
	    TypeSet<float>& dists = fsseds_[ivwr]->getDistances();
	    trcnrs.erase(); dists.erase();
	    for ( int idx=0; idx<regfdp->nrTrcs(); idx++ )
	    {
		const TrcKey tk = regfdp->getTrcKey(idx);
		trcnrs += tk.trcNr();
		dists += mCast(float,regfdp->posData().position(true,idx));
	    }

	    TypeSet<Coord>& coords = fsseds_[ivwr]->getCoords();
	    for ( int idx=0; idx<fsseds_[ivwr]->getTrcNos().size(); idx++ )
		coords += geometry->toCoord(
			geomid_.asInt(), fsseds_[ivwr]->getTrcNos()[idx] );

	    fsseds_[ivwr]->setGeomID( geomid_ );
	    fsseds_[ivwr]->set2D( true );
	    fsseds_[ivwr]->drawFault();
	}
    }
}


void FaultSS2D::enablePainting( bool yn )
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


void FaultSS2D::selected()
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


void FaultSS2D::triggerDeSel()
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
