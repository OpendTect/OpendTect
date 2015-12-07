/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "emhorizonpainter2d.h"

#include "emhorizon2d.h"
#include "emmanager.h"
#include "emobject.h"
#include "zaxistransform.h"
#include "hiddenparam.h"

namespace EM
{
HiddenParam< HorizonPainter2D, ObjectSet<HorizonPainter2D::Marker2D>* > 
	    intsectmarkers_( 0 );
HiddenParam< HorizonPainter2D, Line2DInterSectionSet* > lnintsectset_( 0 );

HorizonPainter2D::HorizonPainter2D( FlatView::Viewer& fv,
				    const EM::ObjectID& oid )
    : viewer_(fv)
    , id_(oid)
    , markerlinestyle_(LineStyle::Solid,2,Color(0,255,0))
    , markerstyle_(MarkerStyle2D::Square, 4, Color::White())
    , linenabled_(true)
    , seedenabled_(true)
    , markerseeds_(0)
    , abouttorepaint_(this)
    , repaintdone_(this)
{
    EM::EMObject* emobj = EM::EMM().getObject( id_ );
    if ( emobj )
    {
	emobj->ref();
	emobj->change.notify( mCB(this,HorizonPainter2D,horChangeCB) );
    }
    
    intsectmarkers_.setParam( this, new ObjectSet<HorizonPainter2D::Marker2D> );
    lnintsectset_.setParam( this, new Line2DInterSectionSet );

}


HorizonPainter2D::~HorizonPainter2D()
{
    EM::EMObject* emobj = EM::EMM().getObject( id_ );
    if ( emobj )
    {
	emobj->change.remove( mCB(this,HorizonPainter2D,horChangeCB) );
	emobj->unRef();
    }

    removePolyLine();
    viewer_.handleChange( FlatView::Viewer::Auxdata );
    delete intsectmarkers_.getParam(this);
    delete lnintsectset_.getParam(this);
    intsectmarkers_.removeParam( this );
    lnintsectset_.removeParam( this );
}


void HorizonPainter2D::setTrcKeyZSampling( const TrcKeyZSampling& cs, 
    bool update )
{
    tkzs_ = cs;
}


void HorizonPainter2D::setGeomID( Pos::GeomID geomid )
{
    geomid_ = geomid;
}


void HorizonPainter2D::paint()
{
    abouttorepaint_.trigger();
    removePolyLine();
    addPolyLine();
    changePolyLineColor();
    viewer_.handleChange( FlatView::Viewer::Auxdata );
    repaintdone_.trigger();
}


bool HorizonPainter2D::addPolyLine()
{
    EM::EMObject* emobj = EM::EMM().getObject( id_ );
    if ( !emobj ) return false;
    
    mDynamicCastGet(EM::Horizon2D*,hor2d,emobj)
    if ( !hor2d ) return false;

    for ( int ids=0; ids<hor2d->nrSections(); ids++ )
    {
	EM::SectionID sid = hor2d->sectionID( ids );
	updateIntersectionMarkers( sid );

	SectionMarker2DLine* secmarkerln = new SectionMarker2DLine;
	markerline_ += secmarkerln;
	FlatView::AuxData* seedauxdata =
	    viewer_.createAuxData( "Horizon2D Marker" );
	seedauxdata->enabled_ = seedenabled_;
	seedauxdata->poly_.erase();
	seedauxdata->markerstyles_ += markerstyle_;
	viewer_.addAuxData( seedauxdata );
	
	markerseeds_ = new Marker2D;
	markerseeds_->marker_ = seedauxdata;
	markerseeds_->sectionid_ = sid; 

	bool newmarker = true;
	bool coorddefined = true;

	Marker2D* marker = 0;
	TrcKeySamplingIterator iter( tkzs_.hsamp_ );
	BinID bid;

	while ( iter.next(bid) )
	{
	    int inlfromcs = bid.inl();
	    if ( hor2d->geometry().lineIndex( geomid_ ) < 0 )
		continue;
	    else
		bid.inl() = hor2d->geometry().lineIndex( geomid_ );
	    
	    const Coord3 crd = hor2d->getPos( sid, bid.toInt64() );
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
	    }
	    
	    if ( newmarker )
	    {
		FlatView::AuxData* auxdata =
		    viewer_.createAuxData( "Horizon2D marker" );
		viewer_.addAuxData( auxdata );
		auxdata->poly_.erase();
		auxdata->linestyle_ = markerlinestyle_;
		Color prefcol = hor2d->preferredColor();
		prefcol.setTransparency( 0 );
		auxdata->linestyle_.color_ = prefcol;
		auxdata->fillcolor_ = prefcol;
		auxdata->enabled_ = linenabled_;
		auxdata->name_ = hor2d->name();
		marker = new Marker2D;
		(*secmarkerln) += marker;
		marker->marker_ = auxdata;
		marker->sectionid_ = sid;
		newmarker = false;
	    }
	    
	    int idx = trcnos_.indexOf(bid.crl());
	    if ( idx == -1 )
		continue;

	    ConstRefMan<ZAxisTransform> zat = viewer_.getZAxisTransform();
	    const double z = zat ? zat->transform(crd) : crd.z;
	    marker->marker_->poly_ += FlatView::Point( distances_[idx], z );

	    if ( hor2d->isPosAttrib(posid,EM::EMObject::sSeedNode()) )
		markerseeds_->marker_->poly_ +=
		    FlatView::Point( distances_[idx], z );
	    
	    bid.inl() = inlfromcs;
	}
    }

    return true;
}


void HorizonPainter2D::horChangeCB( CallBacker* cb )
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
	case EM::EMObjectCallbackData::BurstAlert:
	    {
		if ( emobject->hasBurstAlert() )
		    return;
		paint();
		break;
	    }
	default:
	    break;
    }
}


void HorizonPainter2D::updateIntersectionMarkers( int sid )
{
    if ( lnintsectset_.getParam(this)->size()<=0 )
    {
	if ( !calcLine2DIntersections() )
	    return;
    }
    removeIntersectionMarkers();
    EM::EMObject* emobj = EM::EMM().getObject( id_ );
    mDynamicCastGet( EM::Horizon2D*, hor2d, emobj )
    if ( !hor2d ) return;

    TypeSet<Pos::GeomID> geomids;
    const int nrlns = hor2d->geometry().nrLines();
    for ( int idx=0; idx<nrlns; idx++ )
	geomids += hor2d->geometry().geomID(idx);

    for ( int idx=0; idx<lnintsectset_.getParam(this)->size(); idx++ )
    {
	const Line2DInterSection* intsect=(*lnintsectset_.getParam(this))[idx];
	if ( !intsect )  continue;
	if ( intsect->geomID() != geomid_ )
		continue;

	for ( int idy=0; idy<geomids.size(); idy++ )
	{
	    for ( int idz=0; idz<intsect->size(); idz++ )
	    {
		const Line2DInterSection::Point& intpoint = 
		    intsect->getPoint(idz);
		int trcnr = geomids[idy] != 
		    geomid_ ? intpoint.linetrcnr : intpoint.mytrcnr;
		if ( geomids[idy] != geomid_ )
		{
		    if ( intpoint.line != geomids[idy] )
			continue;
		}
		float x = .0f;
		Coord3 crd = hor2d->getPos( 
		    EM::SectionID(sid), geomids[idy], trcnr );
		ConstRefMan<ZAxisTransform> zat = viewer_.getZAxisTransform();
		const float z = zat ? zat->transform(crd) : (float)crd.z;
		const int didx = trcnos_.indexOf( intpoint.mytrcnr );
		if ( didx>0 && didx<distances_.size() )
		    x = distances_[didx];
		if ( !mIsUdf(z) && x!=.0f )
		    *intsectmarkers_.getParam(this) += 
		    create2DMarker( EM::SectionID(sid), x, z );
	    }
	}
    }
}


bool HorizonPainter2D::calcLine2DIntersections()
{
    const TypeSet<Pos::GeomID> geom2dids = viewer_.getAllSeisGeomids();
    if ( geom2dids.size()==0 )
	return false;

    BendPointFinder2DGeomSet bpfinder( geom2dids );
    bpfinder.execute();
    Line2DInterSectionFinder intfinder( bpfinder.bendPoints(), 
	*lnintsectset_.getParam(this) );
    intfinder.execute();

    return lnintsectset_.getParam(this)->size()>0;
}


HorizonPainter2D::Marker2D* HorizonPainter2D::create2DMarker(
    const EM::SectionID& sid, float x, float z )
{
    FlatView::AuxData* seedauxdata = viewer_.createAuxData( 0 );
    seedauxdata->enabled_ = seedenabled_;
    seedauxdata->poly_.erase();
    EM::EMObject* emobj = EM::EMM().getObject(id_);
    markerstyle_.color_ =
	emobj->getPosAttrMarkerStyle(EM::EMObject::sSeedNode()).color_;
    seedauxdata->markerstyles_ += markerstyle_;
    viewer_.addAuxData( seedauxdata );
    Marker2D* marker = new Marker2D;
    marker->marker_ = seedauxdata;
    marker->sectionid_ = sid;
    marker->marker_->poly_ += FlatView::Point( x, z );
    return marker;
}


void HorizonPainter2D::getDisplayedHor( ObjectSet<Marker2D>& disphor )
{
    for ( int secidx=0; secidx<markerline_.size(); secidx++ )
	disphor.append( *markerline_[secidx] );

    if ( seedenabled_ )
	disphor += markerseeds_;
}


void HorizonPainter2D::changePolyLineColor()
{
    EM::EMObject* emobj = EM::EMM().getObject( id_ );
    if ( !emobj ) return;

    for ( int idx=0; idx<markerline_.size(); idx++ )
    {
	SectionMarker2DLine* secmarkerlines = markerline_[idx];

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


void HorizonPainter2D::removePolyLine()
{
    for ( int markidx=markerline_.size()-1;  markidx>=0; markidx-- )
    {
	SectionMarker2DLine* markerlines = markerline_[markidx];
	for ( int idy=markerlines->size()-1; idy>=0; idy-- )
	    if ( !viewer_.removeAuxData( (*markerlines)[idy]->marker_ ) )
		(*markerlines)[idy]->marker_ = 0;
	deepErase( *markerlines );
    }
    deepErase( markerline_ );

    if ( markerseeds_ )
    {
	viewer_.removeAuxData(  markerseeds_->marker_ );
	delete markerseeds_;
	markerseeds_ = 0;
    }
    removeIntersectionMarkers();
}


void HorizonPainter2D::removeIntersectionMarkers()
{
    for ( int idx=intsectmarkers_.getParam(this)->size()-1; idx>=0; idx-- )
	viewer_.removeAuxData((*intsectmarkers_.getParam(this))[idx]->marker_);
    deepErase( *intsectmarkers_.getParam(this) );
}


void HorizonPainter2D::enableLine( bool yn )
{
    if ( linenabled_ == yn )
	return;

    for ( int markidx=markerline_.size()-1;  markidx>=0; markidx-- )
    {
	SectionMarker2DLine* markerlines = markerline_[markidx];
	
	for ( int idy=markerlines->size()-1; idy>=0; idy-- )
	{
	    (*markerlines)[idy]->marker_->enabled_ = yn;
	}
    }

    linenabled_ = yn;
    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


void HorizonPainter2D::enableSeed( bool yn )
{
    if ( seedenabled_ == yn )
	return;

    if ( !markerseeds_ ) return;
    markerseeds_->marker_->enabled_ = yn;
    seedenabled_ = yn;
    viewer_.handleChange( FlatView::Viewer::Auxdata );
}



void HorizonPainter2D::displayIntersection( bool yn )
{
    for ( int idx=0; idx<intsectmarkers_.getParam(this)->size(); idx++ )
	(*intsectmarkers_.getParam(this))[idx]->marker_->enabled_ =  yn;
    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


}; //namespace EM
