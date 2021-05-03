/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
________________________________________________________________________

-*/

#include "view2dfaultss2d.h"

#include "flatauxdataeditor.h"
#include "faultstickseteditor.h"
#include "flatposdata.h"
#include "mpeengine.h"
#include "mpefssflatvieweditor.h"
#include "seisdatapack.h"

#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatauxdataeditor.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"

mCreateVw2DFactoryEntry( VW2DFaultSS2D );

VW2DFaultSS2D::VW2DFaultSS2D( const EM::ObjectID& oid, uiFlatViewWin* win,
			const ObjectSet<uiFlatViewAuxDataEditor>& auxdataeds )
    : Vw2DEMDataObject(oid,win,auxdataeds)
    , deselted_( this )
    , fsseditor_(0)
    , knotenabled_(false)
{
    fsseds_.allowNull();
    if ( oid >= 0 )
	setEditors();
}


void VW2DFaultSS2D::setEditors()
{
    deepErase( fsseds_ );
    RefMan<MPE::ObjectEditor> editor = MPE::engine().getEditor( emid_, true );
    mDynamicCastGet( MPE::FaultStickSetEditor*, fsseditor, editor.ptr() );
    fsseditor_ = fsseditor;
    if ( fsseditor_ )
	fsseditor_->ref();

    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	const uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	ConstDataPackRef<RegularFlatDataPack> regfdp = vwr.obtainPack( true );
	if ( !regfdp )
	{
	    fsseds_ += 0;
	    continue;
	}

	MPE::FaultStickSetFlatViewEditor* fssed =
	    new MPE::FaultStickSetFlatViewEditor(
	     const_cast<uiFlatViewAuxDataEditor*>(auxdataeditors_[ivwr]),emid_);
	fsseds_ += fssed;
    }
}


VW2DFaultSS2D::~VW2DFaultSS2D()
{
    deepErase(fsseds_);
    if ( fsseditor_ )
    {
	fsseditor_->unRef();
	MPE::engine().removeEditor( emid_ );
    }
}


void VW2DFaultSS2D::draw()
{
    const Survey::Geometry* geometry = Survey::GM().getGeometry( geomid_ );
    if ( !geometry ) return;

    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	const uiFlatViewer& vwr = viewerwin_->viewer( ivwr );
	ConstDataPackRef<RegularFlatDataPack> regfdp =
					vwr.obtainPack( true, true );
	if ( !regfdp ) continue;

	if ( fsseds_[ivwr] )
	{
	    TypeSet<int>& trcnrs = fsseds_[ivwr]->getTrcNos();
	    TypeSet<float>& dists = fsseds_[ivwr]->getDistances();
	    trcnrs.erase(); dists.erase();
	    for ( int idx=0; idx<regfdp->nrTrcs(); idx++ )
	    {
		trcnrs += regfdp->getTrcKey(idx).trcNr();
		dists += mCast(float,regfdp->posData().position(true,idx));
	    }

	    TypeSet<Coord>& coords = fsseds_[ivwr]->getCoords();
	    for ( int idx=0; idx<fsseds_[ivwr]->getTrcNos().size(); idx++ )
		coords += geometry->toCoord(
			geomid_, fsseds_[ivwr]->getTrcNos()[idx] );

	    fsseds_[ivwr]->setGeomID( geomid_ );
	    fsseds_[ivwr]->set2D( true );
	    fsseds_[ivwr]->drawFault();
	}
    }
}


void VW2DFaultSS2D::enablePainting( bool yn )
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


void VW2DFaultSS2D::selected()
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
		fsseds_[ivwr]->setMouseEventHandler( 0 );
	    fsseds_[ivwr]->enableKnots( iseditable );
	    knotenabled_ = iseditable;
	}
    }
}


void VW2DFaultSS2D::triggerDeSel()
{
    for ( int ivwr=0; ivwr<viewerwin_->nrViewers(); ivwr++ )
    {
	if ( fsseds_[ivwr] )
	{
	    fsseds_[ivwr]->setMouseEventHandler( 0 );
	    fsseds_[ivwr]->enableKnots( false );
	}
    }

    knotenabled_ = false;
    deselted_.trigger();
}
