/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
________________________________________________________________________

-*/

#include "emhorizonpainter2d.h"

#include "emhorizon2d.h"
#include "emmanager.h"
#include "emobject.h"
#include "zaxistransform.h"

namespace EM
{

HorizonPainter2D::HorizonPainter2D( FlatView::Viewer& fv,
				    const DBKey& oid )
    : viewer_(fv)
    , id_(oid)
    , markerlinestyle_(OD::LineStyle::Solid,2,Color(0,255,0))
    , markerstyle_(OD::MarkerStyle2D::Square, 4, Color::White())
    , linenabled_(true)
    , seedenabled_(true)
    , markerseeds_(0)
    , selectionpoints_(0)
    , abouttorepaint_(this)
    , repaintdone_(this)
{
    EM::Object* emobj = EM::Hor2DMan().getObject( id_ );
    if ( emobj )
    {
	emobj->ref();
	emobj->objectChanged().notify( mCB(this,HorizonPainter2D,horChangeCB) );
    }
}


HorizonPainter2D::~HorizonPainter2D()
{
    detachAllNotifiers();
    EM::Object* emobj = EM::Hor2DMan().getObject( id_ );
    if ( emobj )
    {
	emobj->objectChanged().remove( mCB(this,HorizonPainter2D,horChangeCB) );
	emobj->unRef();
    }

    removePolyLine();
    removeSelections();
    viewer_.handleChange( FlatView::Viewer::Auxdata );
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


void HorizonPainter2D::paint( CallBacker* )
{
    mEnsureExecutedInMainThread( HorizonPainter2D::paint );
    abouttorepaint_.trigger();
    removePolyLine();
    addPolyLine();
    changePolyLineColor();
    viewer_.handleChange( FlatView::Viewer::Auxdata );
    repaintdone_.trigger();
}


bool HorizonPainter2D::addPolyLine()
{
    EM::Object* emobj = EM::Hor2DMan().getObject( id_ );
    if ( !emobj ) return false;

    const OD::MarkerStyle3D ms3d =
	emobj->getPosAttrMarkerStyle( EM::Object::sSeedNode() );
    markerstyle_.color_ = ms3d.color_;
    markerstyle_.size_ = ms3d.size_*2;
    markerstyle_.type_ = OD::MarkerStyle3D::getMS2DType(ms3d.type_);

    mDynamicCastGet(EM::Horizon2D*,hor2d,emobj)
    if ( !hor2d ) return false;

    updateIntersectionMarkers();
    SectionMarker2DLine* secmarkerln = new SectionMarker2DLine;
    markerline_ += secmarkerln;
    FlatView::AuxData* seedauxdata =
	viewer_.createAuxData( "Horizon2D Marker" );
    seedauxdata->cursor_ = seedenabled_ ? MouseCursor::Cross
					: MouseCursor::Arrow;
    seedauxdata->enabled_ = seedenabled_;
    seedauxdata->poly_.erase();
    seedauxdata->markerstyles_ += markerstyle_;
    viewer_.addAuxData( seedauxdata );

    markerseeds_ = new Marker2D;
    markerseeds_->marker_ = seedauxdata;

    bool newmarker = true;
    bool coorddefined = true;

    Marker2D* marker = 0;
    TrcKeySamplingIterator iter( tkzs_.hsamp_ );
    do
    {
	TrcKey trk( iter.curTrcKey() );
	if ( hor2d->geometry().lineIndex( geomid_ ) < 0 )
	    continue;

	const Coord3 crd = hor2d->getCoord( trk );
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
	    FlatView::AuxData* auxdata =
		viewer_.createAuxData( "Horizon2D marker" );
	    viewer_.addAuxData( auxdata );
	    auxdata->cursor_ = seedenabled_ ? MouseCursor::Cross
					    : MouseCursor::Arrow;
	    auxdata->poly_.erase();
	    auxdata->linestyle_ = markerlinestyle_;
	    Color prefcol = hor2d->preferredColor();
	    prefcol.setTransparency( 0 );
	    auxdata->linestyle_.color_ = prefcol;
	    auxdata->fillcolor_ = prefcol;
	    auxdata->enabled_ = linenabled_;
	    auxdata->name_ = toUiString(hor2d->name());
	    marker = new Marker2D;
	    (*secmarkerln) += marker;
	    marker->marker_ = auxdata;
	    newmarker = false;
	}

	const int idx = trcnos_.indexOf( trk.trcNr() );
	if ( idx == -1 )
	    continue;

	ConstRefMan<ZAxisTransform> zat = viewer_.getZAxisTransform();
	const double z = zat ? (double)zat->transformTrc(trk,(float)crd.z_)
			     : crd.z_;
	marker->marker_->poly_ += FlatView::Point( distances_[idx], z );

	if ( hor2d->isAttrib(trk,EM::Object::sSeedNode()) )
	    markerseeds_->marker_->poly_ +=
		FlatView::Point( distances_[idx], z );

    } while ( iter.next() );

    return true;
}


void HorizonPainter2D::horChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( EM::ObjectCallbackData,
				cbdata, caller, cb );
    mDynamicCastGet(EM::Object*,emobject,caller);
    if ( !emobject ) return;

    if ( cbdata.changeType() == EM::Object::cUndefChange() )
	return;
    else if ( cbdata.changeType() == EM::Object::cPrefColorChange() )
	changePolyLineColor();
    else if ( cbdata.changeType() == EM::Object::cAttribChange() )
	paint();
    else if ( cbdata.changeType() == EM::Object::cBurstAlert() )
    {
	if ( emobject->hasBurstAlert() )
	    return;
	paint();
    }
}


void HorizonPainter2D::updateIntersectionMarkers()
{
    if ( intsectset_.size()<=0 )
    {
	if ( !calcLine2DIntersections() )
	    return;
    }
    removeIntersectionMarkers();
    EM::Object* emobj = EM::Hor2DMan().getObject( id_ );
    mDynamicCastGet( EM::Horizon2D*, hor2d, emobj )
    if ( !hor2d ) return;

    GeomIDSet geomids;
    const int nrlns = hor2d->geometry().nrLines();
    for ( int idx=0; idx<nrlns; idx++ )
	geomids += hor2d->geometry().geomID(idx);

    for ( int idx=0; idx<intsectset_.size(); idx++ )
    {
	const Line2DInterSection* intsect = intsectset_[idx];
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
		    geomid_ ? intpoint.othertrcnr_ : intpoint.mytrcnr_;
		if ( geomids[idy] != geomid_ )
		{
		    if ( intpoint.otherid_ != geomids[idy] )
			continue;
		}
		float x = .0f;
		Coord3 crd = hor2d->getPos( geomids[idy], trcnr );
		ConstRefMan<ZAxisTransform> zat = viewer_.getZAxisTransform();
		const TrcKey tk( geomid_, trcnr );
		const float z = zat ? zat->transformTrc( tk, (float)crd.z_ )
				    : (float)crd.z_;
		const int didx = trcnos_.indexOf( intpoint.mytrcnr_ );
		if ( didx>0 && didx<distances_.size() )
		    x = distances_[didx];
		if ( !mIsUdf(z) && x!=.0f )
		{
		    Marker2D* intsecmarker =
			create2DMarker( x, z );
		    intsecmarker->marker_->markerstyles_.first().color_ =
			emobj->preferredColor();
		    intsectmarks_ += intsecmarker;
		}
	    }
	}
    }
}


bool HorizonPainter2D::calcLine2DIntersections()
{
    const GeomIDSet geom2dids = viewer_.getAllSeisGeomids();
    if ( geom2dids.size()==0 )
	return false;

    BendPointFinder2DGeomSet bpfinder( geom2dids );
    bpfinder.execute();
    Line2DInterSectionFinder intfinder( bpfinder.bendPoints(), intsectset_ );
    intfinder.execute();

    return intsectset_.size()>0;
}


HorizonPainter2D::Marker2D* HorizonPainter2D::create2DMarker( float x, float z )
{
    Marker2D* marker = 0;
    marker = create2DMarker();
    if ( marker )
	marker->marker_->poly_ += FlatView::Point( x, z );
    return marker;
}


HorizonPainter2D::Marker2D* HorizonPainter2D::create2DMarker()
{
    FlatView::AuxData* seedauxdata = viewer_.createAuxData(0);
    seedauxdata->cursor_ = seedenabled_ ? MouseCursor::Cross
	: MouseCursor::Arrow;
    seedauxdata->enabled_ = seedenabled_;
    seedauxdata->poly_.erase();
    seedauxdata->markerstyles_ += markerstyle_;
    viewer_.addAuxData(seedauxdata);
    Marker2D* marker = new Marker2D;
    marker->marker_ = seedauxdata;
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
    EM::Object* emobj = EM::Hor2DMan().getObject( id_ );
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
    for ( int idx=intsectmarks_.size()-1; idx>=0; idx-- )
	viewer_.removeAuxData( intsectmarks_[idx]->marker_ );
    deepErase( intsectmarks_ );
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
    for ( int idx=0; idx<intsectmarks_.size(); idx++ )
	intsectmarks_[idx]->marker_->enabled_ =  yn;
    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


void HorizonPainter2D::displaySelections(
    const TypeSet<EM::PosID>& pointselections )
{
    EM::Object* emobj = EM::Hor2DMan().getObject( id_ );
    if ( !emobj )
	return;

    mDynamicCastGet( const EM::Horizon2D*, hor2d, emobj );
    if ( !hor2d ) return;

    removeSelections();

    selectionpoints_ = create2DMarker();
    for ( int idx=0; idx<pointselections.size(); idx++ )
    {
	const Coord3 pos = emobj->getPos( pointselections[idx] );
	const TrcKey tk = tkzs_.hsamp_.toTrcKey( pos.getXY() );
	ConstRefMan<ZAxisTransform> zat = viewer_.getZAxisTransform();
	const float z = zat ? zat->transformTrc(tk,(float)pos.z_)
			    : (float)pos.z_;

	const int didx = trcnos_.indexOf( tk.trcNr() );

	const bool isseed =
	    hor2d->isPosAttrib(pointselections[idx],EM::Object::sSeedNode());
	const int postype = isseed ? EM::Object::sSeedNode()
	    : EM::Object::sIntersectionNode();
	const OD::MarkerStyle3D ms3d = emobj->getPosAttrMarkerStyle( postype );
	markerstyle_.color_ = hor2d->selectionColor();
	markerstyle_.size_ = ms3d.size_*2;
	markerstyle_.type_ = OD::MarkerStyle3D::getMS2DType( ms3d.type_ );
	selectionpoints_->marker_->markerstyles_ += markerstyle_;
	selectionpoints_->marker_->poly_ += FlatView::Point(distances_[didx],z);
    }

    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


void HorizonPainter2D::removeSelections()
{
    if ( selectionpoints_ )
    {
	viewer_.removeAuxData( selectionpoints_->marker_ );
	delete selectionpoints_;
	selectionpoints_ = 0;
    }
}


void HorizonPainter2D::updateSelectionColor()
{
    EM::Object* emobj = EM::Hor2DMan().getObject( id_ );
    mDynamicCastGet( const EM::Horizon2D*, hor2d, emobj );
    if ( !hor2d ) return;

    if ( !selectionpoints_ )
	return;
    TypeSet<OD::MarkerStyle2D>& markerstyles =
	selectionpoints_->marker_->markerstyles_;

    for ( int idx=0;idx<markerstyles.size();idx++ )
	markerstyles[idx].color_ = hor2d->selectionColor();

    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


} //namespace EM
