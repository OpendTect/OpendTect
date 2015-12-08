/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Ranojay Sen
 Date:		Mar 2011
 RCS:		$Id$
________________________________________________________________________

-*/

#include "view2dpickset.h"

#include "angles.h"
#include "zaxistransform.h"
#include "flatposdata.h"
#include "ioobj.h"
#include "ioman.h"
#include "pickset.h"
#include "picksettr.h"
#include "seisdatapack.h"
#include "separstr.h"
#include "survinfo.h"
#include "uiflatviewer.h"
#include "uiflatauxdataeditor.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"


mCreateVw2DFactoryEntry( VW2DPickSet );

VW2DPickSet::VW2DPickSet( const EM::ObjectID& picksetidx, uiFlatViewWin* win,
			  const ObjectSet<uiFlatViewAuxDataEditor>& editors )
    : Vw2DDataObject()
    , pickset_(0)
    , deselected_(this)
    , isownremove_(false)
{
    if ( picksetidx >= 0 && Pick::Mgr().size() > picksetidx )
	pickset_ = &Pick::Mgr().get( picksetidx );

    for ( int idx=0; idx<editors.size(); idx++ )
    {
	editors_ += const_cast<uiFlatViewAuxDataEditor*>(editors[idx]);
	viewers_ += &editors[idx]->getFlatViewer();
	picks_ += viewers_[idx]->createAuxData( "Picks" );

	viewers_[idx]->addAuxData( picks_[idx] );
	viewers_[idx]->appearance().annot_.editable_ = false;
	mAttachCB( viewers_[idx]->dataChanged, VW2DPickSet::dataChangedCB );

	auxids_ += editors_[idx]->addAuxData( picks_[idx], true );
	editors_[idx]->enableEdit( auxids_[idx], true, true, true );
	editors_[idx]->enablePolySel( auxids_[idx], true );
	mAttachCB( editors_[idx]->removeSelected, VW2DPickSet::pickRemoveCB );
	mAttachCB( editors_[idx]->movementFinished, VW2DPickSet::pickAddChgCB );
    }

    mAttachCB( Pick::Mgr().setChanged, VW2DPickSet::dataChangedCB );
    mAttachCB( Pick::Mgr().locationChanged, VW2DPickSet::dataChangedCB );
}


VW2DPickSet::~VW2DPickSet()
{
    detachAllNotifiers();
    for ( int ivwr=0; ivwr<viewers_.size(); ivwr++ )
    {
	viewers_[ivwr]->removeAuxData( picks_[ivwr] );
	editors_[ivwr]->removeAuxData( auxids_[ivwr] );
    }
    deepErase( picks_ );
}


void VW2DPickSet::pickAddChgCB( CallBacker* cb )
{
    mDynamicCastGet(uiFlatViewAuxDataEditor*,editor,cb);
    if ( !editor || editor->getSelPtIdx().size() || editor->isSelActive() ||
	    !isselected_ || !pickset_ )
	return;

    const uiFlatViewer& vwr = editor->getFlatViewer();
    Coord3 crd = vwr.getCoord( editor->getSelPtPos() );
    if ( !crd.isDefined() ) return;

    if ( vwr.hasZAxisTransform() )
	crd.z = vwr.getZAxisTransform()->transformBack( crd );

    (*pickset_) += Pick::Location( crd );
    const int locidx = pickset_->size()-1;
    Pick::SetMgr::ChangeData cd( Pick::SetMgr::ChangeData::Added,
				 pickset_, locidx );
    Pick::Mgr().reportChange( 0, cd );
}


void VW2DPickSet::pickRemoveCB( CallBacker* cb )
{
    mDynamicCastGet(uiFlatViewAuxDataEditor*,editor,cb);
    if ( !editor || !pickset_ ) return;

    const int editoridx = editors_.indexOf( editor );
    if ( editoridx<0 ) return;

    const TypeSet<int>&	selpts = editor->getSelPtIdx();
    const int selsize = selpts.size();
    isownremove_ = selsize == 1;
    for ( int idx=0; idx<selsize; idx++ )
    {
	const int locidx = selpts[idx];
	if ( !picks_[editoridx]->poly_.validIdx(locidx) )
	    continue;

	const int pickidx = picksetidxs_[locidx];
	picksetidxs_.removeSingle( locidx );
	Pick::SetMgr::ChangeData cd( Pick::SetMgr::ChangeData::ToBeRemoved,
				     pickset_, pickidx );
	pickset_->removeSingle( pickidx );
	Pick::Mgr().reportChange( 0, cd );
    }

    isownremove_ = false;
}


OD::MarkerStyle2D VW2DPickSet::get2DMarkers( const Pick::Set& ps ) const
{
    OD::MarkerStyle2D style( OD::MarkerStyle2D::Square, ps.disp_.pixsize_,
			 ps.disp_.color_ );
    switch( ps.disp_.markertype_ )
    {
	case OD::MarkerStyle3D::Plane:
	    style.type_ = OD::MarkerStyle2D::Plane;
	    break;
	case OD::MarkerStyle3D::Sphere:
	    style.type_ = OD::MarkerStyle2D::Circle;
	    break;
	case OD::MarkerStyle3D::Cube:
	    style.type_ = OD::MarkerStyle2D::Square;
	    break;
	case OD::MarkerStyle3D::Cone:
	    style.type_ = OD::MarkerStyle2D::Triangle;
	    break;
	case OD::MarkerStyle3D::Cross:
	    style.type_ = OD::MarkerStyle2D::Plus;
	    break;
	case OD::MarkerStyle3D::Arrow:
	    style.type_ = OD::MarkerStyle2D::Arrow;
	    break;
	case OD::MarkerStyle3D::Cylinder:
	    style.type_ = OD::MarkerStyle2D::Square;
	    break;
	default:
	    style.type_ = OD::MarkerStyle2D::Circle;
    }

    return style;
}


void VW2DPickSet::updateSetIdx( const TrcKeyZSampling& cs )
{
    if ( !pickset_ ) return;
    picksetidxs_.erase();
    for ( int idx=0; idx<pickset_->size(); idx++ )
    {
	const Coord3& pos = (*pickset_)[idx].pos_;
	const BinID bid = SI().transform( pos );
	if ( cs.hsamp_.includes(bid) )
	    picksetidxs_ += idx;
    }
}


void VW2DPickSet::updateSetIdx( const TrcKeyPath& trckeys )
{
    if ( !pickset_ ) return;
    picksetidxs_.erase();
    for ( int idx=0; idx<pickset_->size(); idx++ )
    {
	const Coord3& pos = (*pickset_)[idx].pos_;
	const BinID bid = SI().transform( pos );
	const TrcKey trckey = Survey::GM().traceKey(
		Survey::GM().default3DSurvID(), bid.inl(), bid.crl() );
	if ( trckeys.isPresent(trckey) )
	    picksetidxs_ += idx;
    }
}


void VW2DPickSet::drawAll()
{
    ConstDataPackRef<FlatDataPack> fdp = viewers_[0]->obtainPack( true, true );
    if ( !fdp || !pickset_ ) return;

    mDynamicCastGet(const RegularFlatDataPack*,regfdp,fdp.ptr());
    mDynamicCastGet(const RandomFlatDataPack*,randfdp,fdp.ptr());
    if ( !regfdp && !randfdp ) return;

    if ( regfdp )
	updateSetIdx( regfdp->sampling() );
    else if ( randfdp )
	updateSetIdx( randfdp->getPath() );

    if ( isownremove_ ) return;

    for ( int ivwr=0; ivwr<viewers_.size(); ivwr++ )
    {
	uiFlatViewer& vwr = *viewers_[ivwr];
	const uiWorldRect& curvw = vwr.curView();
	const float zdiff = (float) curvw.height();
	const float nrzpixels = mCast(float,vwr.getViewRect().vNrPics());
	const float zfac = nrzpixels / zdiff;
	const float nrxpixels = mCast(float,vwr.getViewRect().hNrPics());

	FlatView::AuxData* picks = picks_[ivwr];
	picks->poly_.erase();
	picks->markerstyles_.erase();
	OD::MarkerStyle2D markerstyle = get2DMarkers( *pickset_ );
	const int nrpicks = picksetidxs_.size();
	ConstRefMan<ZAxisTransform> zat = vwr.getZAxisTransform();
	for ( int idx=0; idx<nrpicks; idx++ )
	{
	    const int pickidx = picksetidxs_[idx];
	    const Coord3& pos = (*pickset_)[pickidx].pos_;
	    const BinID bid = SI().transform(pos);
	    const double z = zat ? zat->transform(pos) : pos.z;
	    if ( regfdp )
	    {
		BufferString dipval;
		(*pickset_)[pickidx].getText( "Dip" , dipval );
		SeparString dipstr( dipval );
		const bool oninl =
		    regfdp->sampling().defaultDir() == TrcKeyZSampling::Inl;
		const float dip = oninl ? dipstr.getFValue( 1 )
					: dipstr.getFValue( 0 );
		const float depth = (dip/1000000) * zfac;
		const float xdiff = (float) ( curvw.width() *
			  ( oninl ? SI().crlDistance() : SI().inlDistance() ) );
		const float xfac = nrxpixels / xdiff;
		markerstyle.rotation_ = mIsUdf(dip) ? 0
			    : Math::toDegrees( Math::Atan2( 2*depth, xfac ) );
		FlatView::Point point( oninl ? bid.crl():bid.inl(), z );
		picks->poly_ += point;
	    }
	    else if ( randfdp )
	    {
		const FlatPosData& flatposdata = randfdp->posData();
		const TrcKey trckey = Survey::GM().traceKey(
			Survey::GM().default3DSurvID(), bid.inl(), bid.crl() );
		const int bidindex = randfdp->getPath().indexOf( trckey );
		const double bidpos = flatposdata.position( true, bidindex );
		FlatView::Point point( bidpos, z );
		picks->poly_ += point;
	    }
	    picks->markerstyles_ += markerstyle;
	}
	vwr.handleChange( FlatView::Viewer::Auxdata );
    }
}


void VW2DPickSet::clearPicks()
{
    if ( !pickset_ ) return;
    pickset_->erase();
    drawAll();
}


void VW2DPickSet::enablePainting( bool yn )
{
    for ( int ivwr=0; ivwr<viewers_.size(); ivwr++ )
    {
	picks_[ivwr]->enabled_ = yn;
	viewers_[ivwr]->handleChange( FlatView::Viewer::Auxdata );
    }
}


void VW2DPickSet::dataChangedCB( CallBacker* )
{
    drawAll();
}


void VW2DPickSet::selected()
{
   isselected_ = true;
}


void VW2DPickSet::triggerDeSel()
{
    isselected_ = false;
    deselected_.trigger();
}


bool VW2DPickSet::fillPar( IOPar& iop ) const
{
    Vw2DDataObject::fillPar( iop );
    iop.set( sKeyMID(), pickSetID() );
    return true;
}


bool VW2DPickSet::usePar( const IOPar& iop )
{
    Vw2DDataObject::usePar( iop );
    MultiID mid;
    iop.get( sKeyMID(), mid );

    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( Pick::Mgr().indexOf(ioobj->key()) >= 0 )
	return false;
    Pick::Set* newps = new Pick::Set; uiString errmsg;
    if ( PickSetTranslator::retrieve(*newps,ioobj,true,errmsg) )
    {
	Pick::Mgr().set( ioobj->key(), newps );
	pickset_ = newps;
	return true;
    }
    delete newps;
    return false;
}


const MultiID VW2DPickSet::pickSetID() const
{
    return pickset_ ? Pick::Mgr().get( *pickset_ ) : -1;
}
