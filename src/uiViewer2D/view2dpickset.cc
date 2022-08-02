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
#include "ioman.h"
#include "picksettr.h"
#include "posinfo2dsurv.h"
#include "seisdatapack.h"
#include "separstr.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "uiflatviewer.h"
#include "uiflatauxdataeditor.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"


mImplStd( VW2DPickSet )

VW2DPickSet::VW2DPickSet( uiFlatViewWin* fvw,
			  const ObjectSet<uiFlatViewAuxDataEditor>& editors )
    : Vw2DDataObject()
    , deselected_(this)
{
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


void VW2DPickSet::setPickSet( Pick::Set& ps )
{
    pickset_ = &ps;
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
	crd.z = vwr.getZAxisTransform()->transformBack( crd );

    Pick::Location newloc( crd );
    ConstRefMan<FlatDataPack> dp = vwr.getPack( false, true ).get();
    if ( dp )
    {
	mDynamicCastGet(const SeisFlatDataPack*,seisdp,dp.ptr())
	if ( seisdp && seisdp->nrTrcs() && seisdp->is2D() )
	{
	    const FlatPosData& pd = seisdp->posData();
	    const IndexInfo ix = pd.indexInfo( true, selpt.x );
	    const TrcKey trckey = seisdp->getTrcKey( ix.nearest_ );
	    newloc.setTrcKey( trckey );
	}
    }

    pickset_->add( newloc );
    const int locidx = pickset_->size()-1;
    Pick::SetMgr::ChangeData cd( Pick::SetMgr::ChangeData::Added,
				 pickset_, locidx );
    Pick::Mgr().reportChange( 0, cd );
}


void VW2DPickSet::pickRemoveCB( CallBacker* cb )
{
    mCBCapsuleGet(bool,caps,cb);
    mDynamicCastGet(uiFlatViewAuxDataEditor*,editor,caps->caller);
    ConstRefMan<FlatDataPack> fdp = viewers_[0]->getPack( true, true ).get();
    if ( !fdp || !editor || !pickset_ )
	return;

    const int editoridx = editors_.indexOf( editor );
    if ( editoridx<0 ) return;

    mDynamicCastGet(const RegularFlatDataPack*,regfdp,fdp.ptr());
    mDynamicCastGet(const RandomFlatDataPack*,randfdp,fdp.ptr());
    if ( !regfdp && !randfdp ) return;

    TypeSet<int> vw2dpsidxs;
    for ( int idx=0; idx<pickset_->size(); idx++ )
    {
	const Pick::Location& pl = pickset_->get( idx );
	const BinID bid = SI().transform( pl.pos() );
	if ( regfdp )
	{
	    const TrcKeyZSampling& vwr2dtkzs = regfdp->sampling();
	    if ( regfdp->isVertical() )
	    {
		if ( regfdp->is2D() )
		{
		    const Pos::GeomID geomid(vwr2dtkzs.hsamp_.inlRange().start);
		    if ( pl.hasTrcKey() &&  pl.geomID()!=geomid )
			continue;
		    else
		    {
			mDynamicCastGet(const Survey::Geometry2D*,geom2d,
					Survey::GM().getGeometry(geomid) );
			if ( !geom2d || geom2d->data().nearestIdx(pl.pos())<0 )
			    continue;
		    }
		}
		else if ( !vwr2dtkzs.hsamp_.includes(bid) )
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
	else
	{
	    const TrcKey trckey( bid );
	    if ( randfdp->getPath().indexOf(trckey)<0 )
		continue;
	}

	vw2dpsidxs += idx;
    }

    const TypeSet<int>&	selpts = editor->getSelPtIdx();
    const int selsize = selpts.size();
    for ( int idx=0; idx<selsize; idx++ )
    {
	const int locidx = selpts[idx];
	if ( !picks_[editoridx]->poly_.validIdx(locidx) ||
	     !vw2dpsidxs.validIdx(locidx) )
	    continue;

	const int pickidx = vw2dpsidxs[locidx];
	Pick::SetMgr::ChangeData cd( Pick::SetMgr::ChangeData::ToBeRemoved,
				     pickset_, pickidx );
	pickset_->remove( pickidx );
	Pick::Mgr().reportChange( 0, cd );
    }
}


MarkerStyle2D VW2DPickSet::get2DMarkers( const Pick::Set& ps ) const
{
    MarkerStyle2D style( MarkerStyle2D::Square, ps.disp_.pixsize_,
			 ps.disp_.color_ );
    switch( ps.disp_.markertype_ )
    {
	case MarkerStyle3D::Plane:
	    style.type_ = MarkerStyle2D::Plane;
	    break;
	case MarkerStyle3D::Sphere:
	    style.type_ = MarkerStyle2D::Circle;
	    break;
	case MarkerStyle3D::Cube:
	    style.type_ = MarkerStyle2D::Square;
	    break;
	case MarkerStyle3D::Cone:
	    style.type_ = MarkerStyle2D::Triangle;
	    break;
	case MarkerStyle3D::Cross:
	    style.type_ = MarkerStyle2D::Plus;
	    break;
	case MarkerStyle3D::Arrow:
	    style.type_ = MarkerStyle2D::Arrow;
	    break;
	case MarkerStyle3D::Cylinder:
	    style.type_ = MarkerStyle2D::Square;
	    break;
	default:
	    style.type_ = MarkerStyle2D::Circle;
    }

    return style;
}


void VW2DPickSet::drawAll()
{
    ConstRefMan<FlatDataPack> fdp = viewers_[0]->getPack( true, true ).get();
    if ( !fdp || !pickset_ ) return;

    mDynamicCastGet(const RegularFlatDataPack*,regfdp,fdp.ptr());
    mDynamicCastGet(const RandomFlatDataPack*,randfdp,fdp.ptr());
    if ( !regfdp && !randfdp ) return;

    RefMan<Survey::Geometry3D> geom3d = SI().get3DGeometry( false );
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
	MarkerStyle2D markerstyle = get2DMarkers( *pickset_ );
	const int nrpicks = pickset_->size();
	ConstRefMan<ZAxisTransform> zat = vwr.getZAxisTransform();
	for ( int idx=0; idx<nrpicks; idx++ )
	{
	    const Pick::Location& pl = pickset_->get( idx );
	    const Coord3& pos = pl.pos();
	    const double z = zat ? zat->transform(pos) : pos.z;
	    const Coord bidf = bid2crd.transformBackNoSnap( pos.coord() );
	    if ( regfdp && regfdp->isVertical() )
	    {
		BufferString dipval;
		pl.getText( "Dip" , dipval );
		SeparString dipstr( dipval );
		double distance = 0.;
		const TrcKeyZSampling& vwr2dtkzs = regfdp->sampling();
		if ( !regfdp->is2D() )
		{
		    if ( !vwr2dtkzs.hsamp_.includes(SI().transform(pos)) )
			continue;
		}
		else
		{
		    const Pos::GeomID geomid(vwr2dtkzs.hsamp_.inlRange().start);
		    int trcidx = -1;
		    if ( pl.hasTrcKey() )
		    {
			if ( pl.geomID()!=geomid )
			    continue;

			trcidx = regfdp->getSourceGlobalIdx( pl.trcKey() );
		    }
		    else
		    {
			mDynamicCastGet(const Survey::Geometry2D*,geom2d,
					Survey::GM().getGeometry(geomid) );
			if ( !geom2d )
			    continue;
			trcidx = geom2d->data().nearestIdx( pos );
		    }


		    if ( trcidx<0 )
			continue;

		    distance = regfdp->getAltDim0Value( -1, trcidx );
		}

		const bool oninl = regfdp->is2D() ||
		    regfdp->sampling().defaultDir() == TrcKeyZSampling::Inl;
		const float dip = oninl ? dipstr.getFValue( 1 )
					: dipstr.getFValue( 0 );
		const float depth = (dip/1000000) * zfac;
		const float xdiff = (float) ( curvw.width() *
			(regfdp->is2D() ? 1
				       : (oninl ? SI().crlDistance()
						: SI().inlDistance()) ) );
		const float xfac = nrxpixels / xdiff;
		markerstyle.rotation_ = mIsUdf(dip) ? 0
			    : Math::toDegrees( Math::Atan2( 2*depth, xfac ) );
		FlatView::Point point( regfdp->is2D() ? distance
						      : oninl ? bidf.y
							      : bidf.x, z );
		picks->poly_ += point;
	    }
	    else if ( regfdp && !regfdp->isVertical() )
	    {
		const float vwr2dzpos = regfdp->sampling().zsamp_.start;
		const float eps = regfdp->sampling().zsamp_.step/2.f;
		if ( !mIsEqual(vwr2dzpos,pos.z,eps) )
		    continue;

		FlatView::Point point( bidf.x, bidf.y );
		picks->poly_ += point;
	    }
	    else if ( randfdp )
	    {
		const BinID bid = SI().transform(pos);
		const FlatPosData& flatposdata = randfdp->posData();
		const TrcKey trckey( bid );
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
    if ( !pickset_ )
	return;

    pickset_->setEmpty();
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
    BufferString bs;
    RefMan<Pick::Set> newps = new Pick::Set;
    if ( PickSetTranslator::retrieve(*newps,ioobj,true, bs) )
    {
	Pick::Mgr().set( ioobj->key(), newps );
	pickset_ = newps;
	return true;
    }

    return false;
}


MultiID VW2DPickSet::pickSetID() const
{
    return pickset_ ? Pick::Mgr().get( *pickset_ ) : MultiID::udf();
}
