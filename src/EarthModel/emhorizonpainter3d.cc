/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2010
________________________________________________________________________

-*/

#include "emhorizonpainter3d.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "flatposdata.h"
#include "zaxistransform.h"

namespace EM
{

HorizonPainter3D::HorizonPainter3D( FlatView::Viewer& fv,
				    const DBKey& oid )
    : viewer_(fv)
    , id_(oid)
    , markerlinestyle_(OD::LineStyle::Solid,2,Color(0,255,0))
    , markerstyle_(OD::MarkerStyle2D::Square, 4, Color::White())
    , linenabled_(true)
    , seedenabled_(true)
    , markerseeds_(0)
    , selectionpoints_(0)
    , nrseeds_(0)
    , path_(0)
    , flatposdata_(0)
    , abouttorepaint_(this)
    , repaintdone_(this)
{
    EM::Object* emobj = EM::Hor3DMan().getObject( id_ );
    if ( emobj )
    {
	emobj->ref();
	emobj->objectChanged().notify( mCB(this,HorizonPainter3D,horChangeCB) );
    }
    updatesamplings_.init( false );
}


HorizonPainter3D::~HorizonPainter3D()
{
    detachAllNotifiers();
    EM::Object* emobj = EM::Hor3DMan().getObject( id_ );
    if ( emobj )
    {
	emobj->removePosAttribList(
	    EM::Object::sIntersectionNode(), false );
	emobj->objectChanged().remove( mCB(this,HorizonPainter3D,horChangeCB) );
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
    const EM::Object* emobj = EM::Hor3DMan().getObject( id_ );
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
    EM::Object* emobj = EM::Hor3DMan().getObject( id_ );
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
	    bid = (*path_)[idx].binID();
	    const EM::PosID posid = PosID::getFromRowCol( bid );
	    const Coord3 crd = hor3d->getPos( posid );
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

	    addDataToMarker( bid, crd, posid, *hor3d, *marker, newmarker, idx );
	    newmarker = false;
	}
    }
    else
    {
	TrcKeySamplingIterator iter( tkzs_.hsamp_ );
	do
	{
	    const TrcKey trk( iter.curTrcKey() );
	    const EM::PosID posid = PosID::getFromRowCol( trk.position() );
	    const Coord3 crd = hor3d->getPos( posid );
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

	    if ( addDataToMarker(trk.position(),crd,posid,*hor3d,
				 *marker,newmarker) )
		nrseeds_++;

	    newmarker = false;

	} while ( iter.next() );
    }

    viewer_.handleChange( FlatView::Viewer::Auxdata );
    return true;
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
    Color prefcol = hor3d.preferredColor();
    prefcol.setTransparency( 0 );
    auxdata->linestyle_.color_ = prefcol;
    auxdata->fillcolor_ = prefcol;
    auxdata->enabled_ = linenabled_;
    auxdata->name_ = toUiString(hor3d.name());
    marker = new Marker3D;
    secmarkerln += marker;
    marker->marker_ = auxdata;
}


bool HorizonPainter3D::addDataToMarker( const BinID& bid, const Coord3& crd,
					const EM::PosID& posid,
					const EM::Horizon3D& hor3d,
					Marker3D& marker,
					bool newmarker, int idx )
{
    ConstRefMan<ZAxisTransform> zat = viewer_.getZAxisTransform();
    const double z = zat ? zat->transform(crd) : crd.z_;

    double x = 0.0;
    if ( path_ )
	x = flatposdata_->position( true, idx );
    else if ( tkzs_.nrInl() == 1 )
	x = bid.crl();
    else if ( tkzs_.nrCrl() == 1 )
	x = bid.inl();

    const TypeSet<OD::PlotAnnotation>& intsecpositions =
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
    const bool isseed = hor3d.isPosAttrib( posid, EM::Object::sSeedNode() );
    if ( newmarker || isseed || isintersec )
    {
	const int postype = isseed ? EM::Object::sSeedNode()
				   : EM::Object::sIntersectionNode();
	EM::Object* emobj = EM::Hor3DMan().getObject( id_ );
	OD::MarkerStyle3D ms3d = emobj->getPosAttrMarkerStyle( postype );
	markerstyle_.color_ = ms3d.color_;
	if ( newmarker )
	    ms3d.type_ = OD::MarkerStyle3D::Sphere;

	if ( isintersec || newmarker )
	    markerstyle_.color_ = emobj->preferredColor();
	markerstyle_.size_ = ms3d.size_*2;
	markerstyle_.type_ = OD::MarkerStyle3D::getMS2DType( ms3d.type_ );
	markerseeds_->marker_->markerstyles_ += markerstyle_;
	markerseeds_->marker_->poly_ += FlatView::Point( x, z );
	return true;
    }

    return false;
}


void HorizonPainter3D::horChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( EM::ObjectCallbackData, cbdata, caller, cb );
    mDynamicCastGet(EM::Object*,emobject,caller);
    if ( !emobject ) return;

    if ( cbdata.changeType() == EM::Object::cUndefChange() )
	return;
    else if ( cbdata.changeType() == EM::Object::cPrefColorChange() )
	changePolyLineColor();
    else if ( cbdata.changeType() == EM::Object::cAttribChange() )
	paint();
    else if ( cbdata.changeType() == EM::Object::cPositionChange() )
    {
	if ( emobject->hasBurstAlert() )
	    return;

	RefMan<EM::ChangeAuxData> cbauxdata =
			cbdata.auxDataAs<EM::ChangeAuxData>();
	if ( !cbauxdata )
	    return;

	const BinID bid = cbauxdata->pid0.getBinID();
	const TrcKey tk( bid );
	if ( tkzs_.hsamp_.includes(bid) ||
	    (path_&&path_->isPresent(tk)) )
	{
	    changePolyLinePosition( cbauxdata->pid0 );
	    viewer_.handleChange( FlatView::Viewer::Auxdata );
	}
    }
    else if ( cbdata.changeType() == EM::Object::cBurstAlert() )
    {
	if ( emobject->hasBurstAlert() )
	    return;
	paint();
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
    EM::Object* emobj = EM::Hor3DMan().getObject( id_ );
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
    mDynamicCastGet(EM::Horizon3D*,hor3d,EM::Hor3DMan().getObject( id_ ));
    if ( !hor3d ) return;

    const BinID binid = pid.getBinID();
    const TrcKey trckey( binid );

    SectionMarker3DLine* secmarkerlines = markerline_[0];
    for ( int markidx=0; markidx<secmarkerlines->size(); markidx++ )
    {
	Coord3 crd = hor3d->getPos( pid );
	FlatView::AuxData* auxdata = (*secmarkerlines)[markidx]->marker_;
	for ( int posidx = 0; posidx < auxdata->poly_.size(); posidx ++ )
	{
	    if ( path_ )
	    {
		if ( mIsEqual(
		    flatposdata_->position(true,path_->indexOf(trckey)),
		    auxdata->poly_[posidx].x_,.001) )
		{
		    auxdata->poly_[posidx].y_ = crd.z_;
		    return;
		}
		continue;
	    }

	    if ( tkzs_.nrInl() == 1 )
	    {
		if ( binid.crl() == auxdata->poly_[posidx].x_ )
		{
		    auxdata->poly_[posidx].y_ = crd.z_;
		    return;
		}
	    }
	    else if ( tkzs_.nrCrl() == 1 )
	    {
		if ( binid.inl() == auxdata->poly_[posidx].x_ )
		{
		    auxdata->poly_[posidx].y_ = crd.z_;
		    return;
		}
	    }
	}

	if ( crd.isDefined() )
	{
	    if ( path_ )
	    {
		auxdata->poly_ += FlatView::Point( flatposdata_->position(
			    true,path_->indexOf(trckey)), crd.z_ );
		continue;
	    }
	    if ( tkzs_.nrInl() == 1 )
		auxdata->poly_ += FlatView::Point( binid.crl(), crd.z_ );
	    else if ( tkzs_.nrCrl() == 1 )
		auxdata->poly_ += FlatView::Point( binid.inl(), crd.z_ );
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
    updatesamplings_ = TrcKeySampling( samplings );
}



void HorizonPainter3D::displaySelections(
    const TypeSet<EM::PosID>& pointselections )
{
    EM::Object* emobj = EM::Hor3DMan().getObject( id_ );
    if ( !emobj )
	return;

    mDynamicCastGet( const EM::Horizon3D*, hor3d, emobj );
    if ( !hor3d ) return;

    removeSelections();

    selectionpoints_ = create3DMarker();

    for ( int idx=0; idx<pointselections.size(); idx++ )
    {
	const Coord3 pos = emobj->getPos( pointselections[idx] );
	const TrcKey tk = tkzs_.hsamp_.toTrcKey(pos.getXY());
	double x = 0.0;
	if ( tkzs_.nrLines()==1 )
	    x = tk.crl();
	else if ( tkzs_.nrTrcs()==1 )
	    x = tk.inl();
	const bool isseed =
	    hor3d->isPosAttrib(pointselections[idx],EM::Object::sSeedNode());
	const int postype = isseed ? EM::Object::sSeedNode()
	    : EM::Object::sIntersectionNode();
	const OD::MarkerStyle3D ms3d = emobj->getPosAttrMarkerStyle( postype );
	markerstyle_.color_ = ms3d.color_;
	markerstyle_.color_ = hor3d->selectionColor();
	markerstyle_.size_ = ms3d.size_*2;
	markerstyle_.type_ = OD::MarkerStyle3D::getMS2DType( ms3d.type_ );
	selectionpoints_->marker_->markerstyles_ += markerstyle_;
	selectionpoints_->marker_->poly_ += FlatView::Point( x, pos.z_ );
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


void HorizonPainter3D::updateSelectionColor()
{
    EM::Object* emobj = EM::Hor3DMan().getObject( id_ );
    mDynamicCastGet( const EM::Horizon3D*, hor3d, emobj );
    if ( !hor3d ) return;

    if ( !selectionpoints_ )
	return;
    TypeSet<OD::MarkerStyle2D>& markerstyles =
	selectionpoints_->marker_->markerstyles_;

    for ( int idx=0; idx<markerstyles.size(); idx++ )
	markerstyles[idx].color_ = hor3d->selectionColor();

    viewer_.handleChange( FlatView::Viewer::Auxdata );
}
} // namespace EM
