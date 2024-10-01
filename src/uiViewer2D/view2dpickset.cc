/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "view2dpickset.h"

#include "flatposdata.h"
#include "ioman.h"
#include "ioobj.h"
#include "picksettr.h"
#include "posinfo2d.h"
#include "seisdatapack.h"
#include "separstr.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "zaxistransform.h"

#include "uiflatauxdataeditor.h"
#include "uiflatviewer.h"
#include "uigraphicsscene.h"


namespace View2D
{

mImplStd( PickSet )

PickSet::PickSet( uiFlatViewWin* fvw,
		  const ObjectSet<uiFlatViewAuxDataEditor>& editors )
    : DataObject()
    , deselected_(this)
{
    for ( int idx=0; idx<editors.size(); idx++ )
    {
	editors_ += const_cast<uiFlatViewAuxDataEditor*>(editors[idx]);
	viewers_ += &editors[idx]->getFlatViewer();
	picks_ += viewers_[idx]->createAuxData( "Picks" );

	viewers_[idx]->addAuxData( picks_[idx] );
	mAttachCB( viewers_[idx]->dataChanged, PickSet::dataChangedCB );

	auxids_ += editors_[idx]->addAuxData( picks_[idx], true );
	editors_[idx]->enableEdit( auxids_[idx], true, true, true );
	editors_[idx]->enablePolySel( auxids_[idx], true );
	mAttachCB( editors_[idx]->removeSelected, PickSet::pickRemoveCB );
	mAttachCB( editors_[idx]->movementFinished, PickSet::pickAddChgCB );
    }

    mAttachCB( Pick::Mgr().setChanged, PickSet::dataChangedCB );
    mAttachCB( Pick::Mgr().locationChanged, PickSet::dataChangedCB );
}


PickSet::~PickSet()
{
    detachAllNotifiers();
    for ( int ivwr=0; ivwr<viewers_.size(); ivwr++ )
    {
	viewers_[ivwr]->removeAuxData( picks_[ivwr] );
	editors_[ivwr]->removeAuxData( auxids_[ivwr] );
    }
    deepErase( picks_ );
}


void PickSet::setPickSet( Pick::Set& ps )
{
    pickset_ = &ps;
}


void PickSet::pickAddChgCB( CallBacker* cb )
{
    mDynamicCastGet(uiFlatViewAuxDataEditor*,editor,cb);
    if ( !editor || editor->getSelPtIdx().size() || editor->isSelActive() ||
	    !isselected_ || !pickset_ )
	return;

    const uiFlatViewer& vwr = editor->getFlatViewer();
    const FlatView::Point& selpt = editor->getSelPtPos();
    Coord3 crd = vwr.getCoord( selpt );
    if ( !crd.isDefined() )
	return;

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


void PickSet::pickRemoveCB( CallBacker* cb )
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
                    const Pos::GeomID geomid(vwr2dtkzs.hsamp_.inlRange().start_);
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
                const float vwr2dzpos = vwr2dtkzs.zsamp_.start_;
                const float eps = vwr2dtkzs.zsamp_.step_/2.f;
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


MarkerStyle2D PickSet::get2DMarkers( const Pick::Set& ps ) const
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


void PickSet::drawAll()
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
		pl.getKeyedText( "Dip" , dipval );
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
                    const Pos::GeomID geomid(vwr2dtkzs.hsamp_.inlRange().start_);
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
                const float vwr2dzpos = regfdp->sampling().zsamp_.start_;
                const float eps = regfdp->sampling().zsamp_.step_/2.f;
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


void PickSet::clearPicks()
{
    if ( !pickset_ )
	return;

    pickset_->setEmpty();
    drawAll();
}


void PickSet::enablePainting( bool yn )
{
    for ( int ivwr=0; ivwr<viewers_.size(); ivwr++ )
    {
	picks_[ivwr]->enabled_ = yn;
	viewers_[ivwr]->handleChange( FlatView::Viewer::Auxdata );
    }
}


void PickSet::dataChangedCB( CallBacker* )
{
    mEnsureExecutedInMainThread( PickSet::dataChangedCB );

    drawAll();
}


void PickSet::selected()
{
   isselected_ = true;
}


void PickSet::triggerDeSel()
{
    isselected_ = false;
    deselected_.trigger();
}


bool PickSet::fillPar( IOPar& iop ) const
{
    DataObject::fillPar( iop );
    iop.set( sKeyMID(), pickSetID() );
    return true;
}


bool PickSet::usePar( const IOPar& iop )
{
    DataObject::usePar( iop );
    MultiID mid;
    iop.get( sKeyMID(), mid );

    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( Pick::Mgr().indexOf(ioobj->key()) >= 0 )
	return false;
    uiString errmsg;
    RefMan<Pick::Set> newps = new Pick::Set;
    if ( PickSetTranslator::retrieve(*newps,ioobj,true,errmsg) )
    {
	Pick::Mgr().set( ioobj->key(), newps );
	pickset_ = newps;
	return true;
    }

    return false;
}


MultiID PickSet::pickSetID() const
{
    return pickset_ ? Pick::Mgr().get( *pickset_ ) : MultiID::udf();
}

} // namespace View2D
