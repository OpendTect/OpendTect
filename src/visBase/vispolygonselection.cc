/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vispolygonselection.h"

#include "polygon.h"

#include <osgGeo/PolygonSelection>

#include <math.h>

mCreateFactoryEntry( visBase::PolygonSelection );

namespace visBase
{

Notifier<PolygonSelection>& PolygonSelection::polygonFinished()
{
    static PtrMan<Notifier<PolygonSelection> > thenotif_;
    if ( !thenotif_ )
	thenotif_ = new Notifier<PolygonSelection>( nullptr );

    return *thenotif_.ptr();
}

class SelectionCallBack : public osg::NodeCallback
{
public:
		    SelectionCallBack( PolygonSelection* sel )
			: polysel_(sel)
		    {}

		    void operator() ( osg::Node* node,
				      osg::NodeVisitor* nv ) override
		    {
			if ( !nv )
			    return;

			RefMan<PolygonSelection> polysel = polysel_.get();
			if ( polysel && polysel->hasPolygon() )
			    PolygonSelection::polygonFinished().trigger();
		    }
protected:
		    ~SelectionCallBack(){}

    WeakPtr<PolygonSelection> polysel_;
};


// PolygonSelection

PolygonSelection::PolygonSelection()
    : VisualObjectImpl(false)
    , selector_(new osgGeo::PolygonSelection)
{
    ref();
    drawstyle_ = DrawStyle::create();
    refOsgPtr( selector_ );
    addChild( selector_ );
    selectorcb_ = new SelectionCallBack( this );
    refOsgPtr( selectorcb_ );
    selector_->addCallBack( selectorcb_ );
    mAttachCB( PolygonSelection::polygonFinished(),
	       PolygonSelection::polygonChangeCB );
    unRefNoDelete();
}


PolygonSelection::PolygonSelection( const osgGeo::PolygonSelection* selector )
    : VisualObjectImpl(false)
    , selector_(new osgGeo::PolygonSelection(*selector))
{
    ref();
    drawstyle_ = DrawStyle::create();
    refOsgPtr( selector_ );
    addChild( selector_ );
    selectorcb_ = new SelectionCallBack( this );
    refOsgPtr( selectorcb_ );
    selector_->addCallBack( selectorcb_ );
    mAttachCB( PolygonSelection::polygonFinished(),
	       PolygonSelection::polygonChangeCB );
    unRefNoDelete();
}


PolygonSelection::~PolygonSelection()
{
    detachAllNotifiers();
    selector_->removeCallBack( selectorcb_ );
    unRefOsgPtr( selectorcb_ );
    unRefOsgPtr( selector_ );
    delete polygon_;
}


PolygonSelection* PolygonSelection::copy() const
{
    if ( !selector_ || !hasPolygon() )
	return nullptr;

    auto* polsel = new PolygonSelection( selector_ );
    polsel->setUTMCoordinateTransform( utm2disptransform_.ptr() );
    return polsel;
}


void PolygonSelection::setSelectionType( PolygonSelection::SelectionType st )
{
    if ( st==Off )
    {
	selector_->setShapeType( osgGeo::PolygonSelection::Off );
	selector_->clear();
    }
    else if ( st==Rectangle )
	selector_->setShapeType( osgGeo::PolygonSelection::Rectangle );
    else if ( st==Polygon )
	selector_->setShapeType( osgGeo::PolygonSelection::Polygon );
}


PolygonSelection::SelectionType PolygonSelection::getSelectionType() const
{
    if ( selector_->getShapeType() == osgGeo::PolygonSelection::Off )
	return Off;

    if ( selector_->getShapeType() == osgGeo::PolygonSelection::Rectangle )
	return Rectangle;

    return Polygon;
}


void PolygonSelection::setPrimaryCamera( Camera* maincam )
{
    primarycamera_ = maincam;
    if ( !selector_->setEventHandlerCamera(primarycamera_->osgCamera(),false) )
	{ pErrMsg( "Need access to camera view and scene data" ); }
}


void PolygonSelection::setMasterCamera( Camera* camera )
{ setPrimaryCamera( camera ); }


void PolygonSelection::setHUDCamera( Camera* hudcam )
{
    selector_->setHUDCamera( hudcam->osgCamera() );
}


void PolygonSelection::setLineStyle( const OD::LineStyle& lst )
{
    drawstyle_->setLineStyle( lst );
}


const OD::LineStyle& PolygonSelection::getLineStyle() const
{
    return drawstyle_->lineStyle();
}


void PolygonSelection::clear()
{ selector_->clear(); }


bool PolygonSelection::hasPolygon() const
{
    return selector_->getCoords()->size() > 2;
}


bool PolygonSelection::singleSelection() const
{
    return selector_->getCoords()->size()==1;
}


bool PolygonSelection::isSelfIntersecting() const
{
    if ( !hasPolygon() )
	return false;

    if ( !polygon_ )
    {
	polygon_ = new ODPolygon<double>;
	const osg::Vec3Array& coords = *selector_->getCoords();
	for ( int idx=0; idx<coords.size(); idx++ )
	{
	    const osg::Vec3& coord = coords[idx];
	    polygon_->add( Coord(coord.x(),coord.y()) );
	}
    }

    return polygon_->isSelfIntersecting();
}


bool PolygonSelection::isInside( const Coord3& crd, bool displayspace ) const
{
   if ( selector_->getShapeType() == osgGeo::PolygonSelection::Off  )
	return false;

    if ( !hasPolygon() )
	return false;

    Coord3 checkcoord3d = crd;
    if ( !displayspace && utm2disptransform_ )
	utm2disptransform_->transform( checkcoord3d );

    const osg::Vec2 coord2d = selector_->projectPointToScreen(
                                  osg::Vec3((float) checkcoord3d.x_,
                                            (float) checkcoord3d.y_,
                                            (float) checkcoord3d.z_) );

    const Coord checkcoord2d( coord2d.x(), coord2d.y() );
    if ( !checkcoord2d.isDefined() )
	return false;

    polygonlock_.readLock();
    if ( !polygon_ )
    {
	polygonlock_.convReadToWriteLock();
	if ( !polygon_ )
	{
	    polygon_ = new ODPolygon<double>;

	    const osg::Vec3Array& polycoords = *selector_->getCoords();
	    for ( int idx=0; idx<polycoords.size(); idx++ )
	    {
		Coord polycoord2d( polycoords[idx].x(), polycoords[idx].y() );
		polygon_->add( polycoord2d );
	    }
	}

	polygonlock_.convWriteToReadLock();
    }

    const bool res = polygon_->isInside( checkcoord2d, true, 1e-3 );
    polygonlock_.readUnLock();

    return res;
}


char PolygonSelection::includesRange( const Coord3& start, const Coord3& stop,
				      bool displayspace ) const
{
  if ( selector_->getShapeType() == osgGeo::PolygonSelection::Off  )
	return 0;

    if ( !hasPolygon() )
	return 0;

    Coord3 coords[8];
    coords[0] = Coord3( start.x_, start.y_, start.z_ );
    coords[1] = Coord3( start.x_, start.y_, stop.z_ );
    coords[2] = Coord3( start.x_, stop.y_, start.z_ );
    coords[3] = Coord3( start.x_, stop.y_, stop.z_ );
    coords[4] = Coord3( stop.x_, start.y_, start.z_ );
    coords[5] = Coord3( stop.x_, start.y_, stop.z_ );
    coords[6] = Coord3( stop.x_, stop.y_, start.z_ );
    coords[7] = Coord3( stop.x_, stop.y_, stop.z_ );

    if ( !displayspace && utm2disptransform_ )
    {
	for ( int idx=0; idx<8; idx++ )
	    utm2disptransform_->transform( coords[idx] );
    }

    ODPolygon<double> screenpts;

    int nrundefs = 0;
    for ( int idx=0; idx<8; idx++ )
    {
	const osg::Vec2 pt = selector_->projectPointToScreen(
                                 osg::Vec3((float) coords[idx].x_,
                                           (float) coords[idx].y_,(float) coords[idx].z_ ) );

	const Coord vertex( pt[0], pt[1] );
	if ( vertex.isDefined() )
	    screenpts.add( vertex );
	else
	    nrundefs++;
    }

    if ( nrundefs )
	return nrundefs==8 ? 3 : 4;

    polygonlock_.readLock();
    if ( !polygon_ )
    {
	if ( polygonlock_.convReadToWriteLock() || !polygon_ )
	{
	    polygon_ = new ODPolygon<double>;
	    const osg::Vec3Array& sopolygon = *selector_->getCoords();
	    for ( int idx=0; idx<sopolygon.size(); idx++ )
		polygon_->add( Coord(sopolygon[idx].x(),sopolygon[idx].y()) );
	}

	polygonlock_.convWriteToReadLock();
    }

    screenpts.convexHull();
    const char res = (char) polygon_->isInside( screenpts );

    polygonlock_.readUnLock();

    return res;
}


bool PolygonSelection::rayPickThrough( const Coord3& worldpos,
				       TypeSet<int>& pickedobjids,
				       int depthidx ) const
{
    pickedobjids.erase();
    pErrMsg( "Not implemented");
    return true;
}


void PolygonSelection::polygonChangeCB( CallBacker* )
{
    polygonlock_.writeLock();
    deleteAndNullPtr( polygon_ );
    polygonlock_.writeUnLock();
}


void PolygonSelection::setUTMCoordinateTransform( const mVisTrans* nt )
{
    utm2disptransform_ = nt;
}


// PolygonCoord3Selector

PolygonCoord3Selector::PolygonCoord3Selector( const PolygonSelection& vs )
    : vissel_(&vs)
{
}


PolygonCoord3Selector::~PolygonCoord3Selector()
{
}


Selector<Coord3>* PolygonCoord3Selector::clone() const
{
    return new PolygonCoord3Selector( *vissel_.ptr() );
}


const char* PolygonCoord3Selector::selectorType() const
{ return "PolygonCoord3Selector"; }


bool PolygonCoord3Selector::isOK() const
{ return hasPolygon(); }


bool PolygonCoord3Selector::hasPolygon() const
{ return vissel_->hasPolygon(); }


bool PolygonCoord3Selector::includes( const Coord3& c ) const
{ return vissel_->isInside( c, false ); }


char PolygonCoord3Selector::includesRange( const Coord3& start,
					  const Coord3& stop ) const
{
    const char res = vissel_->includesRange( start, stop, false );
    if ( res==3 )
	return 0;

    if ( res==4 )
	return 1;

    return res;
}


bool PolygonCoord3Selector::isEq( const Selector<Coord3>& comp ) const
{
    mDynamicCastGet( const PolygonCoord3Selector*, b, &comp );
    if ( !b ) return false;

    return b->vissel_.ptr() == vissel_.ptr();
}


} // namespace visBase
