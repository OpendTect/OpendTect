/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emhorizonpainter3d.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "flatposdata.h"
#include "zaxistransform.h"

namespace EM
{
HorizonPainter3D::Marker3D::Marker3D()
{}


HorizonPainter3D::Marker3D::~Marker3D()
{
    delete marker_;
}



HorizonPainter3D::HorizonPainter3D( FlatView::Viewer& fv,
				    const EM::ObjectID& oid )
    : viewer_(fv)
    , id_(oid)
    , markerlinestyle_(OD::LineStyle::Solid,2,OD::Color(0,255,0))
    , markerstyle_(MarkerStyle2D::Square,4,OD::Color::White())
    , linenabled_(true)
    , seedenabled_(true)
    , markerseeds_(0)
    , nrseeds_(0)
    , path_(0)
    , flatposdata_(0)
    , abouttorepaint_(this)
    , repaintdone_(this)
    , intersection_(true)
    , selectionpoints_(0)
{
    EM::EMObject* emobj = EM::EMM().getObject( id_ );
    if ( emobj )
    {
	emobj->ref();
	emobj->change.notify( mCB(this,HorizonPainter3D,horChangeCB) );
    }
    updatesamplings_.init( false );
}


HorizonPainter3D::~HorizonPainter3D()
{
    detachAllNotifiers();
    EM::EMObject* emobj = EM::EMM().getObject( id_ );
    if ( emobj )
    {
	emobj->removePosAttribList(
	    EM::EMObject::sIntersectionNode(), false );
	emobj->change.remove( mCB(this,HorizonPainter3D,horChangeCB) );
	emobj->unRef();
    }

    removePolyLine();
    removeSelections();
    viewer_.handleChange( FlatView::Viewer::Auxdata );

    CallBack::removeFromThreadCalls( this );
}


void HorizonPainter3D::setTrcKeyZSampling( const TrcKeyZSampling& cs, bool upd )
{
    tkzs_ = cs;
}


void HorizonPainter3D::setPath( const TrcKeyPath& path )
{
    path_ = &path;
}


void HorizonPainter3D::setFlatPosData( const FlatPosData* fps )
{
    if ( path_ )
	flatposdata_ = fps;
}


void HorizonPainter3D::paint()
{
    CallBack::addToMainThread( mCB(this,HorizonPainter3D,paintCB) );
}


void HorizonPainter3D::paintCB( CallBacker* )
{
    abouttorepaint_.trigger();
    removePolyLine();
    addPolyLine();
    changePolyLineColor();
    const EM::EMObject* emobj = EM::EMM().getObject( id_ );
    if ( emobj && markerseeds_ && nrseeds_==1 )
    {
	for ( int idx=0;idx<markerseeds_->marker_->markerstyles_.size(); idx++ )
	    markerseeds_->marker_->markerstyles_[idx].color_=
							emobj->preferredColor();
    }

    viewer_.handleChange( FlatView::Viewer::Auxdata );
    repaintdone_.trigger();
    updatesamplings_.init( false );
}


HorizonPainter3D::Marker3D* HorizonPainter3D::create3DMarker()
{
    FlatView::AuxData* seedauxdata = viewer_.createAuxData(0);
    seedauxdata->enabled_ = seedenabled_;
    seedauxdata->cursor_ = seedenabled_ ? MouseCursor::Cross
					: MouseCursor::Arrow;
    seedauxdata->poly_.erase();
    viewer_.addAuxData(seedauxdata);

    Marker3D* markerseed = new Marker3D;
    markerseed->marker_ = seedauxdata;
    return markerseed;
}


bool HorizonPainter3D::addPolyLine()
{
    EM::EMObject* emobj = EM::EMM().getObject( id_ );
    if ( !emobj ) return false;

    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj);
    if ( !hor3d ) return false;

    nrseeds_ = 0;
    SectionMarker3DLine* secmarkerln = new SectionMarker3DLine;
    markerline_ += secmarkerln;

    markerseeds_ = create3DMarker();
    bool newmarker = true;
    bool coorddefined = true;

    Marker3D* marker = 0;
    BinID bid;

    if ( path_ )
    {
	for ( int idx = 0; idx<path_->size(); idx++ )
	{
	    bid = (*path_)[idx].position();
	    const Coord3 crd = hor3d->getPos( bid.toInt64() );
	    EM::PosID posid( id_, bid );
	    if ( !crd.isDefined() )
	    {
		coorddefined = false;
		continue;
	    }
	    else if ( !coorddefined )
	    {
		coorddefined = true;
		newmarker = true;
	    }

	    if ( newmarker )
		generateNewMarker( *hor3d, *secmarkerln, marker );

	    addDataToMarker( bid, crd, posid, *hor3d, *marker,
		newmarker, idx );
	    newmarker = false;

	}
    }
    else
    {
	TrcKeySamplingIterator iter( tkzs_.hsamp_ );
	while ( iter.next(bid) )
	{
	    int inlfromcs = bid.inl();
	    const Coord3 crd = hor3d->getPos( bid.toInt64() );
	    EM::PosID posid( id_, bid );

	    if ( !crd.isDefined() )
	    {
		coorddefined = false;
		bid.inl() = inlfromcs;
		continue;
	    }
	    else if ( !coorddefined )
	    {
		coorddefined = true;
		newmarker = true;
	    }

	    if ( newmarker )
		generateNewMarker( *hor3d, *secmarkerln, marker );

	    if ( addDataToMarker(bid,crd,posid,*hor3d,*marker,newmarker) )
		nrseeds_++;

	    newmarker = false;
	}
    }

    viewer_.handleChange( FlatView::Viewer::Auxdata );
    return true;
}


bool HorizonPainter3D::addDataToMarker( const BinID& bid,const Coord3& crd,
    const PosID& posid, const Horizon3D& hor3d, Marker3D& marker,
    bool newmarker, int idx )
{
    ConstRefMan<ZAxisTransform> zat = viewer_.getZAxisTransform();
    const double z = zat ? zat->transform(crd) : crd.z;

    double x = 0.0;
    if ( path_ )
	x = flatposdata_->position( true, idx );
    else if ( tkzs_.nrInl() == 1 )
	x = bid.crl();
    else if ( tkzs_.nrCrl() == 1 )
	x = bid.inl();

    const TypeSet<PlotAnnotation>& intsecpositions =
	viewer_.appearance().annot_.x1_.auxannot_;
    bool isintersec = false;
    for ( int ipos=0; ipos<intsecpositions.size(); ipos++ )
    {
	if ( path_ )
	    break;

	const bool isvwrinl = tkzs_.nrInl() == 1;
	BinID intsecbid( isvwrinl ? tkzs_.hsamp_.inlRange().start
				  : mNINT32(intsecpositions[ipos].pos_),
			 isvwrinl ? mNINT32(intsecpositions[ipos].pos_)
				  : tkzs_.hsamp_.crlRange().start );
	if ( intsecbid == bid )
	{
	    isintersec = true;
	    break;
	}
    }

    marker.marker_->poly_ += FlatView::Point( x, z );
    const bool isseed = hor3d.isPosAttrib( posid, EM::EMObject::sSeedNode() );
    if ( isseed || isintersec )
    {
	const int postype = isseed ? EM::EMObject::sSeedNode()
				   : EM::EMObject::sIntersectionNode();
	EM::EMObject* emobj = EM::EMM().getObject( id_ );
	MarkerStyle3D ms3d = emobj->getPosAttrMarkerStyle( postype );
	markerstyle_.color_ = ms3d.color_;

	if ( isintersec )
	    markerstyle_.color_ = emobj->preferredColor();
	markerstyle_.size_ = ms3d.size_;
	markerstyle_.type_ = MarkerStyle3D::getMS2DType( ms3d.type_ );
	markerseeds_->marker_->markerstyles_ += markerstyle_;
	markerseeds_->marker_->poly_ += FlatView::Point( x, z );
	return true;
    }

    return false;

}


void HorizonPainter3D::generateNewMarker( const EM::Horizon3D& hor3d,
					  SectionMarker3DLine& secmarkerln,
					  Marker3D*& marker )
{
    FlatView::AuxData* auxdata = viewer_.createAuxData( 0 );
    viewer_.addAuxData( auxdata );
    auxdata->poly_.erase();
    auxdata->cursor_ = seedenabled_ ? MouseCursor::Cross : MouseCursor::Arrow;
    auxdata->linestyle_ = markerlinestyle_;
    OD::Color prefcol = hor3d.preferredColor();
    prefcol.setTransparency( 0 );
    auxdata->linestyle_.color_ = prefcol;
    auxdata->fillcolor_ = prefcol;
    auxdata->enabled_ = linenabled_;
    auxdata->name_ = hor3d.name();
    marker = new Marker3D;
    secmarkerln += marker;
    marker->marker_ = auxdata;
}


bool HorizonPainter3D::addDataToMarker( const BinID& bid, const Coord3& crd,
					const EM::PosID& posid,
					const EM::Horizon3D& hor3d,
					Marker3D& marker, int idx )
{
    return addDataToMarker( bid, crd, posid, hor3d, marker, false, idx );
}


void HorizonPainter3D::horChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( const EM::EMObjectCallbackData&,
				cbdata, caller, cb );
    mDynamicCastGet(EM::EMObject*,emobject,caller);
    if ( !emobject ) return;

    switch ( cbdata.event )
    {
	case EM::EMObjectCallbackData::Undef:
	    break;
	case EM::EMObjectCallbackData::PrefColorChange:
	    {
		changePolyLineColor();
		break;
	    }
	case EM::EMObjectCallbackData::AttribChange:
	    {
		paint();
		break;
	    }
	case EM::EMObjectCallbackData::PositionChange:
	    {
		if ( emobject->hasBurstAlert() )
		    return;

		const BinID bid = BinID::fromInt64( cbdata.pid0.subID() );
		const TrcKey tk( bid );
		if ( tkzs_.hsamp_.includes(bid) ||
		    (path_&&path_->isPresent(tk)) )
		{
		    changePolyLinePosition( cbdata.pid0 );
		    viewer_.handleChange( FlatView::Viewer::Auxdata );
		}

		break;
	    }
	case EM::EMObjectCallbackData::BurstAlert:
	    {
		if ( emobject->hasBurstAlert() )
		    return;
		paint();
		break;
	    }
	default: break;
    }
}


void HorizonPainter3D::getDisplayedHor( ObjectSet<Marker3D>& disphor )
{
    for ( int secidx=0; secidx<markerline_.size(); secidx++ )
	disphor.append( *markerline_[secidx] );

    if ( seedenabled_ )
	disphor += markerseeds_;
}


void HorizonPainter3D::changePolyLineColor()
{
    EM::EMObject* emobj = EM::EMM().getObject( id_ );
    if ( !emobj ) return;

    for ( int idx=0; idx<markerline_.size(); idx++ )
    {
	SectionMarker3DLine* secmarkerlines = markerline_[idx];
	OD::Color prefcol = emobj->preferredColor();
	prefcol.setTransparency( 0 );
	const int width = emobj->preferredLineStyle().width_;

	for ( int markidx=0; markidx<secmarkerlines->size(); markidx++ )
	{
	    (*secmarkerlines)[markidx]->marker_->linestyle_.color_ = prefcol;
	    (*secmarkerlines)[markidx]->marker_->linestyle_.width_ = width;
	}
    }

    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


void HorizonPainter3D::changePolyLinePosition( const EM::PosID& pid )
{
    mDynamicCastGet(EM::Horizon3D*,hor3d,EM::EMM().getObject(id_))
    if ( !hor3d ) return;

    if ( id_ != pid.objectID() )
	return;

    const BinID binid = BinID::fromInt64( pid.subID() );
    const TrcKey trckey( binid );
    for ( int idx=0; idx<hor3d->nrSections(); idx++ )
    {
	SectionMarker3DLine* secmarkerlines = markerline_[idx];
	for ( int markidx=0; markidx<secmarkerlines->size(); markidx++ )
	{
	    Coord3 crd = hor3d->getPos( pid.subID() );
	    FlatView::AuxData* auxdata = (*secmarkerlines)[markidx]->marker_;
	    for ( int posidx = 0; posidx < auxdata->poly_.size(); posidx ++ )
	    {
		if ( path_ )
		{
		    if ( mIsEqual(
			flatposdata_->position(true,path_->indexOf(trckey)),
			auxdata->poly_[posidx].x,.001) )
		    {
			auxdata->poly_[posidx].y = crd.z;
			return;
		    }
		    continue;
		}

		if ( tkzs_.nrInl() == 1 )
		{
		    if ( binid.crl() == auxdata->poly_[posidx].x )
		    {
			auxdata->poly_[posidx].y = crd.z;
			return;
		    }
		}
		else if ( tkzs_.nrCrl() == 1 )
		{
		    if ( binid.inl() == auxdata->poly_[posidx].x )
		    {
			auxdata->poly_[posidx].y = crd.z;
			return;
		    }
		}
	    }

	    if ( crd.isDefined() )
	    {
		if ( path_ )
		{
		    auxdata->poly_ += FlatView::Point( flatposdata_->position(
				true,path_->indexOf(trckey)), crd.z );
		    continue;
		}
		if ( tkzs_.nrInl() == 1 )
		    auxdata->poly_ += FlatView::Point( binid.crl(), crd.z );
		else if ( tkzs_.nrCrl() == 1 )
		    auxdata->poly_ += FlatView::Point( binid.inl(), crd.z );
	    }
	}
    }
}


void HorizonPainter3D::removePolyLine()
{
    for ( int markidx=markerline_.size()-1; markidx>=0; markidx-- )
    {
	SectionMarker3DLine* markerlines = markerline_[markidx];
	for ( int idy=markerlines->size()-1; idy>=0; idy-- )
	    if ( !viewer_.removeAuxData( (*markerlines)[idy]->marker_ ) )
		(*markerlines)[idy]->marker_ = 0;
	deepErase( *markerlines );
    }
    deepErase( markerline_ );

    if ( markerseeds_ )
    {
	viewer_.removeAuxData( markerseeds_->marker_ );
	delete markerseeds_;
	markerseeds_ = 0;
    }
}


void HorizonPainter3D::enableLine( bool yn )
{
    if ( linenabled_ == yn )
	return;

    for ( int markidx=markerline_.size()-1;  markidx>=0; markidx-- )
    {
	SectionMarker3DLine* markerlines = markerline_[markidx];
	for ( int idy=markerlines->size()-1; idy>=0; idy-- )
	{
	    (*markerlines)[idy]->marker_->enabled_ = yn;
	}
    }

    linenabled_ = yn;
    if ( nrseeds_ == 1 )
	markerseeds_->marker_->enabled_ = yn;

    viewer_.handleChange( FlatView::Viewer::Auxdata );

}


void HorizonPainter3D::enableSeed( bool yn )
{
    if ( seedenabled_ == yn )
	return;

    if ( !markerseeds_ ) return;
    if ( nrseeds_!=1 )
	markerseeds_->marker_->enabled_ = yn;
    seedenabled_ = yn;
    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


void HorizonPainter3D::setUpdateTrcKeySampling(
    const TrcKeySampling& samplings )
{
    updatesamplings_= TrcKeySampling( samplings );
}


void HorizonPainter3D::displaySelections(
    const TypeSet<EM::PosID>& pointselections )
{
    EM::EMObject* emobj = EM::EMM().getObject( id_ );
    if ( !emobj )
	return;

    mDynamicCastGet( const EM::Horizon3D*, hor3d, emobj );
    if ( !hor3d ) return;

    removeSelections();

    selectionpoints_ = create3DMarker();

    for ( int idx=0; idx<pointselections.size(); idx++ )
    {
	const Coord3 pos = emobj->getPos( pointselections[idx] );
	const TrcKey tk = tkzs_.hsamp_.toTrcKey(pos.coord());
	double x = 0.0;
	if ( tkzs_.nrLines()==1 )
	    x = tk.crl();
	else if ( tkzs_.nrTrcs()==1 )
	    x = tk.inl();
	const bool isseed =
	    hor3d->isPosAttrib(pointselections[idx],EM::EMObject::sSeedNode());
	const int postype = isseed ? EM::EMObject::sSeedNode()
	    : EM::EMObject::sIntersectionNode();
	const MarkerStyle3D ms3d = emobj->getPosAttrMarkerStyle( postype );
	markerstyle_.color_ = ms3d.color_;
	markerstyle_.color_ = hor3d->getSelectionColor();
	markerstyle_.size_ = ms3d.size_*2;
	markerstyle_.type_ = MarkerStyle3D::getMS2DType( ms3d.type_ );
	selectionpoints_->marker_->markerstyles_ += markerstyle_;
	selectionpoints_->marker_->poly_ += FlatView::Point( x, pos.z );
    }

    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


void HorizonPainter3D::removeSelections()
{
    if ( selectionpoints_ )
    {
	viewer_.removeAuxData( selectionpoints_->marker_ );
	delete selectionpoints_;
	selectionpoints_ = 0;
    }
}


void HorizonPainter3D::updatePreferColors()
{
    updateSelectionColor();
    // perhaps in the future, it will have more colors to be update, then
    // to do more color update
}


void HorizonPainter3D::updateSelectionColor()
{
    EM::EMObject* emobj = EM::EMM().getObject( id_ );
    mDynamicCastGet( const EM::Horizon3D*, hor3d, emobj );
    if ( !hor3d ) return;

    if ( !selectionpoints_ )
	return;

    TypeSet<MarkerStyle2D>& markerstyles =
		selectionpoints_->marker_->markerstyles_;
    for ( int idx=0; idx<markerstyles.size(); idx++ )
	markerstyles[idx].color_ = hor3d->getSelectionColor();

    viewer_.handleChange( FlatView::Viewer::Auxdata );
}

} // namespace EM
