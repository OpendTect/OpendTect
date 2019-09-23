/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Ranojay Sen
 Date:		Mar 2011
________________________________________________________________________

-*/

#include "view2dpickset.h"

#include "angles.h"
#include "zaxistransform.h"
#include "flatposdata.h"
#include "ioobj.h"
#include "ioobjctxt.h"
#include "picksetmanager.h"
#include "posinfo2d.h"
#include "seisdatapack.h"
#include "separstr.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "survgeom3d.h"
#include "trckeyzsampling.h"
#include "uiflatviewer.h"
#include "uiflatauxdataeditor.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"


mCreateVw2DFactoryEntry( VW2DPickSet );

VW2DPickSet::VW2DPickSet( const DBKey& psid, uiFlatViewWin* win,
			  const ObjectSet<uiFlatViewAuxDataEditor>& editors )
    : Vw2DDataObject()
    , pickset_(0)
    , deselected_(this)
{
    if ( !psid.isInvalid() )
    {
	RefMan<Pick::Set> ps = Pick::SetMGR().fetchForEdit( psid );
	if ( ps )
	    setPickSet( ps );
    }

    for ( int idx=0; idx<editors.size(); idx++ )
    {
	editors_ += const_cast<uiFlatViewAuxDataEditor*>(editors[idx]);
	viewers_ += &editors[idx]->getFlatViewer();
	picks_ += viewers_[idx]->createAuxData( "Picks" );

	viewers_[idx]->addAuxData( picks_[idx] );
	mAttachCB( viewers_[idx]->dataChanged, VW2DPickSet::dataChangedCB );

	auxids_ += editors_[idx]->addAuxData( picks_[idx], true );
	editors_[idx]->enableEdit( auxids_[idx], true, true, true );
	editors_[idx]->enablePolySel( auxids_[idx], true );
	mAttachCB( editors_[idx]->removeSelected, VW2DPickSet::pickRemoveCB );
	mAttachCB( editors_[idx]->movementFinished, VW2DPickSet::pickAddChgCB );
    }
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


void VW2DPickSet::setPickSet( Pick::Set* ps )
{
    if ( pickset_ == ps )
	return;
    else if ( pickset_ )
	mDetachCB( pickset_->objectChanged(), VW2DPickSet::dataChangedCB );

    pickset_ = ps;
    if ( pickset_ )
	mAttachCB( pickset_->objectChanged(), VW2DPickSet::dataChangedCB );
}


void VW2DPickSet::pickAddChgCB( CallBacker* cb )
{
    mDynamicCastGet(uiFlatViewAuxDataEditor*,editor,cb);
    if ( !editor || editor->getSelPtIdx().size() || editor->isSelActive() ||
	    !isselected_ || !pickset_ )
	return;

    const uiFlatViewer& vwr = editor->getFlatViewer();
    const FlatView::Point& selpt = editor->getSelPtPos();
    Coord3 crd = vwr.getCoord( selpt );
    if ( !crd.isDefined() ) return;

    if ( vwr.hasZAxisTransform() )
	crd.z_ = vwr.getZAxisTransform()->transformBack( crd );

    Pick::Location newloc( crd );
    ConstRefMan<FlatDataPack> dp = vwr.getPack( false, true );
    if ( dp )
    {
	mDynamicCastGet(const SeisFlatDataPack*,seisdp,dp.ptr())
	if ( seisdp && seisdp->nrPositions()>0 && seisdp->is2D() )
	{
	    const FlatPosData& pd = seisdp->posData();
	    const IndexInfo ix = pd.indexInfo( true, selpt.x_ );
	    newloc.setTrcKey( seisdp->path().get(ix.nearest_) );
	}
    }

    pickset_->add( newloc );
}


void VW2DPickSet::pickRemoveCB( CallBacker* cb )
{
    mCBCapsuleGet(bool,caps,cb);
    mDynamicCastGet(uiFlatViewAuxDataEditor*,editor,caps->caller);
    ConstRefMan<FlatDataPack> fdp = viewers_[0]->getPack( true, true );
    if ( !fdp || !editor || !pickset_ ) return;

    const int editoridx = editors_.indexOf( editor );
    if ( editoridx<0 ) return;

    mDynamicCastGet(const RegularSeisFlatDataPack*,regfdp,fdp.ptr());
    mDynamicCastGet(const RandomSeisFlatDataPack*,randfdp,fdp.ptr());
    if ( !regfdp && !randfdp ) return;

    TypeSet<Pick::Set::LocID> vw2dlocids;
    Pick::SetIter psiter( *pickset_ );
    while ( psiter.next() )
    {
	const Pick::Location& pl = psiter.get();
	if ( regfdp )
	{
	    const TrcKeyZSampling vwr2dtkzs( regfdp->subSel() );
	    if ( regfdp->isVertical() )
	    {
		if ( regfdp->is2D() )
		{
		    const Pos::GeomID geomid(
				vwr2dtkzs.hsamp_.inlRange().start );
		    if ( pl.hasTrcKey() && pl.geomID()!=geomid )
			continue;
		    else
		    {
			const auto& geom2d = SurvGeom::get2D( geomid );
			if ( geom2d.isEmpty()
			  || geom2d.data().nearestIdx(pl.pos().getXY())<0 )
			    continue;
		    }
		}
		else if ( !vwr2dtkzs.hsamp_.includes(pl.binID()) )
		    continue;
	    }
	    else
	    {
		const float vwr2dzpos = vwr2dtkzs.zsamp_.start;
		const float eps = vwr2dtkzs.zsamp_.step/2.f;
		if ( !mIsEqual(vwr2dzpos,pl.z(),eps) )
		    continue;
	    }
	}
	else if ( randfdp->path().indexOf(pl.trcKey())<0 )
	    continue;

	vw2dlocids += psiter.ID();
    }

    psiter.retire();


    ChangeNotifyBlocker notifyblocker( *pickset_ );
    const TypeSet<int>&	selpts = editor->getSelPtIdx();
    for ( int idx=0; idx<selpts.size(); idx++ )
    {
	const int locidx = selpts[idx];
	if ( !picks_[editoridx]->poly_.validIdx(locidx) ||
	     !vw2dlocids.validIdx(locidx) )
	    continue;

	pickset_->remove( vw2dlocids[locidx] );
    }
}


OD::MarkerStyle2D VW2DPickSet::get2DMarkers( const Pick::Set& ps ) const
{
    const Pick::Set::Disp psdisp = ps.getDisp();
    OD::MarkerStyle2D style( OD::MarkerStyle2D::Square, psdisp.mkstyle_.size_,
			     psdisp.mkstyle_.color_ );
    switch( psdisp.mkstyle_.type_ )
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


void VW2DPickSet::draw()
{
    ConstRefMan<FlatDataPack> fdp = viewers_[0]->getPack( true, true );
    if ( !fdp || !pickset_ )
	return;

    mDynamicCastGet(const RegularSeisFlatDataPack*,regfdp,fdp.ptr());
    mDynamicCastGet(const RandomSeisFlatDataPack*,randfdp,fdp.ptr());
    if ( !regfdp && !randfdp )
	return;

    ConstRefMan<SurvGeom3D> geom3d = SI().get3DGeometry();
    const Pos::IdxPair2Coord& bid2crd = geom3d->binID2Coord();
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
	ConstRefMan<ZAxisTransform> zat = vwr.getZAxisTransform();
	Pick::SetIter psiter( *pickset_ );
	while ( psiter.next() )
	{
	    const Pick::Location& pl = psiter.get();
	    const Coord3& pos = pl.pos();
	    const double z = zat ? zat->transform(pos) : pos.z_;
	    const Coord bidf = bid2crd.transformBackNoSnap( pos.getXY() );
	    if ( regfdp && regfdp->isVertical() )
	    {
		BufferString dipval;
		pl.getKeyedText( "Dip" , dipval );
		SeparString dipstr( dipval );
		double distance = mUdf(double);
		const TrcKeyZSampling vwr2dtkzs( regfdp->subSel() );
		if ( !regfdp->is2D() )
		{
		    if ( !vwr2dtkzs.hsamp_.includes(pl.binID()) )
			continue;
		}
		else
		{
		    Pos::GeomID geomid = vwr2dtkzs.hsamp_.getGeomID();
		    int trcidx = -1;
		    if ( pl.hasTrcKey() )
		    {
			if ( pl.geomID() != geomid )
			    continue;

			trcidx = regfdp->getSourceDataPack().getGlobalIdx(
								pl.trcKey() );
		    }
		    else
		    {
			const auto& geom2d = SurvGeom::get2D( geomid );
			if ( !geom2d.isEmpty() )
			    trcidx = geom2d.data().nearestIdx( pos.getXY() );
		    }

		    distance = regfdp->getAltDim0Value( -1, trcidx );
		}

		const bool oninl = regfdp->is2D() ||
		    vwr2dtkzs.defaultDir() == OD::InlineSlice;
		const float dip = oninl ? dipstr.getFValue( 1 )
					: dipstr.getFValue( 0 );
		const float depth = (dip/1000000) * zfac;
		const float xdiff = (float) ( curvw.width() *
			(regfdp->is2D() ? 1
					: (oninl ? SI().crlDistance()
						 : SI().inlDistance())) );
		const float xfac = nrxpixels / xdiff;
		markerstyle.rotation_ = mIsUdf(dip) ? 0
			    : Math::toDegrees( Math::Atan2( 2*depth, xfac ) );
		FlatView::Point point( regfdp->is2D()
			? distance
			: oninl ? bidf.y_ : bidf.x_, z );
		picks->poly_ += point;
	    }
	    else if ( regfdp && !regfdp->isVertical() )
	    {
		const auto zrg( regfdp->subSel().zRange() );
		const float vwr2dzpos = zrg.start;
		const float eps = zrg.step/2.f;
		if ( !mIsEqual(vwr2dzpos,pos.z_,eps) )
		    continue;

		FlatView::Point point( bidf.x_, bidf.y_ );
		picks->poly_ += point;
	    }
	    else if ( randfdp )
	    {
		const BinID bid = SI().transform( pos.getXY() );
		const FlatPosData& flatposdata = randfdp->posData();
		const TrcKey trckey( bid );
		const int bidindex = randfdp->path().indexOf( trckey );
		if ( bidindex<0 )
		    continue;

		const double bidpos = flatposdata.position( true, bidindex );
		FlatView::Point point( bidpos, z );
		picks->poly_ += point;
	    }
	    picks->markerstyles_ += markerstyle;
	}
	psiter.retire();
	vwr.handleChange( FlatView::Viewer::Auxdata );
    }
}


void VW2DPickSet::clearPicks()
{
    if ( pickset_ )
    {
	pickset_->setEmpty();
	draw();
    }
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
    draw();
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
    DBKey mid;
    iop.get( sKeyMID(), mid );

    RefMan<Pick::Set> newps = Pick::SetMGR().fetchForEdit( mid );
    if ( !newps )
	return false;

    setPickSet( newps );
    return true;
}


DBKey VW2DPickSet::pickSetID() const
{
    return pickset_ ? Pick::SetMGR().getID( *pickset_ ) : DBKey::getInvalid();
}
