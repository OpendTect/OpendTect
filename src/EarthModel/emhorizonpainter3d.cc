/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "emhorizonpainter3d.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "flatposdata.h"
#include "zaxistransform.h"
#include "hiddenparam.h"

namespace EM
{

    HiddenParam<HorizonPainter3D,TrcKeySampling*> updatesamplings_( 0 );


HorizonPainter3D::HorizonPainter3D( FlatView::Viewer& fv,
				    const EM::ObjectID& oid )
    : viewer_(fv)
    , id_(oid)
    , markerlinestyle_(LineStyle::Solid,2,Color(0,255,0))
    , markerstyle_(MarkerStyle2D::Square, 4, Color::White())
    , linenabled_(true)
    , seedenabled_(true)
    , markerseeds_(0)
    , nrseeds_(0)
    , path_(0)
    , flatposdata_(0)
    , abouttorepaint_(this)
    , repaintdone_(this)
    , intersection_( true )
{
    EM::EMObject* emobj = EM::EMM().getObject( id_ );
    if ( emobj )
    {
	emobj->ref();
	emobj->change.notify( mCB(this,HorizonPainter3D,horChangeCB) );
    }
    updatesamplings_.setParam( this, new TrcKeySampling() );
    updatesamplings_.getParam(this)->init( false );
}


HorizonPainter3D::~HorizonPainter3D()
{
    EM::EMObject* emobj = EM::EMM().getObject( id_ );
    if ( emobj )
    {
	emobj->removePosAttribList(
	    EM::EMObject::sIntersectionNode(), false );
	emobj->change.remove( mCB(this,HorizonPainter3D,horChangeCB) );
	emobj->unRef();
    }

    removePolyLine();
    viewer_.handleChange( FlatView::Viewer::Auxdata );
    delete updatesamplings_.getParam(this);
    updatesamplings_.removeParam( this );
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
    updatesamplings_.getParam(this)->init( false );
}


HorizonPainter3D::Marker3D* HorizonPainter3D::create3DMarker( 
    const EM::SectionID& sid )
{
    FlatView::AuxData* seedauxdata = viewer_.createAuxData(0);
    seedauxdata->enabled_ = seedenabled_;
    seedauxdata->poly_.erase();
    EM::EMObject* emobj = EM::EMM().getObject( id_ );
    markerstyle_.color_ = 
	emobj->getPosAttrMarkerStyle(EM::EMObject::sSeedNode()).color_;
    seedauxdata->markerstyles_ += markerstyle_;
    viewer_.addAuxData(seedauxdata);

    Marker3D* markerseed = new Marker3D;
    markerseed->marker_ = seedauxdata;
    markerseed->sectionid_ = sid;
    return markerseed;
}


bool HorizonPainter3D::addPolyLine()
{
    EM::EMObject* emobj = EM::EMM().getObject( id_ );
    if ( !emobj ) return false;

    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj);
    if ( !hor3d ) return false;

    nrseeds_ = 0;
    for ( int ids=0; ids<hor3d->nrSections(); ids++ )
    {
	EM::SectionID sid = hor3d->sectionID( ids );
	SectionMarker3DLine* secmarkerln = new SectionMarker3DLine;
	markerline_ += secmarkerln;

	markerseeds_ = create3DMarker( sid );
	bool newmarker = true;
	bool coorddefined = true;

	Marker3D* marker = 0;
	BinID bid;

	if ( path_ )
	{
	    for ( int idx = 0; idx<path_->size(); idx++ )
	    {
		bid = (*path_)[idx].pos();
		const Coord3 crd = hor3d->getPos( sid, bid.toInt64() );
		EM::PosID posid( id_, sid, bid.toInt64() );

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
		{
		    generateNewMarker( *hor3d, sid, *secmarkerln, marker );
		    newmarker = false;
		}

		addDataToMarker( bid, crd, posid, *hor3d, *marker, idx );
	    }
	    continue;
	}

	TrcKeySamplingIterator iter( tkzs_.hsamp_ );
	while ( iter.next(bid) )
	{
	    int inlfromcs = bid.inl();
	    const Coord3 crd = hor3d->getPos( sid, bid.toInt64() );
	    EM::PosID posid( id_, sid, bid.toInt64() );

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
		const EM::PosID pid = EM::PosID(emobj->id(),sid,bid.toInt64());
		if ( intersection_ )
		    emobj->setPosAttrib(
		    pid, EM::EMObject::sIntersectionNode(), true, false );
	    }

	    if ( newmarker )
	    {
		generateNewMarker( *hor3d, sid, *secmarkerln, marker );
		newmarker = false;
	    }

	    const bool intsectnode = emobj->isPosAttrib(
		posid,EM::EMObject::sIntersectionNode() );
	    const TrcKey tk ( TrcKey(hor3d->getSurveyID(),bid) );

	    if ( updatesamplings_.getParam(this)->includes(tk) && intsectnode )
		emobj->setPosAttrib( posid ,EM::EMObject::sIntersectionNode(), 
		false );

	    if ( addDataToMarker( bid, crd, posid, *hor3d, *marker ) )
		nrseeds_++;
	}
    }

    viewer_.handleChange( FlatView::Viewer::Auxdata );
    return true;
}


void HorizonPainter3D::generateNewMarker( const EM::Horizon3D& hor3d,
					  const EM::SectionID& sid,
					  SectionMarker3DLine& secmarkerln,
					  Marker3D*& marker )
{
    FlatView::AuxData* auxdata = viewer_.createAuxData( 0 );
    viewer_.addAuxData( auxdata );
    auxdata->poly_.erase();
    auxdata->linestyle_ = markerlinestyle_;
    Color prefcol = hor3d.preferredColor();
    prefcol.setTransparency( 0 );
    auxdata->linestyle_.color_ = prefcol;
    auxdata->fillcolor_ = prefcol;
    auxdata->enabled_ = linenabled_;
    auxdata->name_ = hor3d.name();
    marker = new Marker3D;
    secmarkerln += marker;
    marker->marker_ = auxdata;
    marker->sectionid_ = sid;
}


bool HorizonPainter3D::addDataToMarker( const BinID& bid, const Coord3& crd,
					const EM::PosID& posid,
					const EM::Horizon3D& hor3d,
					Marker3D& marker, int idx )
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

    marker.marker_->poly_ += FlatView::Point( x, z ); 
    if ( hor3d.isPosAttrib(posid,EM::EMObject::sSeedNode()) ||
	hor3d.isPosAttrib(posid,EM::EMObject::sIntersectionNode()) ) 
    {
	 markerseeds_->marker_->poly_ += FlatView::Point( x, z ); 
	 return true;
    }

    return false;
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
	case EM::EMObjectCallbackData::PositionChange:
	    {
		if ( emobject->hasBurstAlert() )
		    return;

		const BinID bid = BinID::fromInt64( cbdata.pid0.subID() );
		const TrcKey tk = Survey::GM().traceKey(
			Survey::GM().default3DSurvID(), bid.inl(), bid.crl() );
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
	Color prefcol = emobj->preferredColor();
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
    mDynamicCastGet(EM::Horizon3D*,hor3d,EM::EMM().getObject( id_ ));
    if ( !hor3d ) return;

    if ( id_ != pid.objectID() ) return;

    const BinID binid = BinID::fromInt64( pid.subID() );
    const TrcKey trckey = Survey::GM().traceKey(
		Survey::GM().default3DSurvID(), binid.inl(), binid.crl() );

    for ( int idx=0; idx<hor3d->nrSections(); idx++ )
    {
	if ( hor3d->sectionID(idx) != pid.sectionID() )
	    continue;

	SectionMarker3DLine* secmarkerlines = markerline_[idx];
	for ( int markidx=0; markidx<secmarkerlines->size(); markidx++ )
	{
	    Coord3 crd = hor3d->getPos( hor3d->sectionID(idx), pid.subID() );
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
    delete updatesamplings_.getParam(this);
    updatesamplings_.removeParam( this );
    updatesamplings_.setParam(this, new TrcKeySampling(samplings) );
}

} // namespace EM
