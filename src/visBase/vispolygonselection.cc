/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          June 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vispolygonselection.h"

#include "polygon.h"
#include "viscamera.h"
#include "vistransform.h"
#include "visdrawstyle.h"

#include <osgGeo/PolygonSelection>

#include <math.h>

mCreateFactoryEntry( visBase::PolygonSelection );

namespace visBase
{

Notifier<PolygonSelection>* PolygonSelection::polygonFinished()
{
    mDefineStaticLocalObject( Notifier<PolygonSelection>, 
			      polygonfinished, (0) );
    return &polygonfinished;
}

class SelectionCallBack : public osg::NodeCallback
{
public:
		    SelectionCallBack(){}
		    void operator() ( osg::Node* node, osg::NodeVisitor* nv )
		    {
			if ( nv )
			    PolygonSelection::polygonFinished()->trigger();
		    }
protected:
    		    ~SelectionCallBack(){}
};

PolygonSelection::PolygonSelection()
    : VisualObjectImpl( false )
    , utm2disptransform_( 0 )
    , selector_( new osgGeo::PolygonSelection )
    , drawstyle_( new DrawStyle )
    , polygon_( 0 )
{
    drawstyle_->ref();
    addChild( selector_ );
    selectorcb_ = new SelectionCallBack;
    selectorcb_->ref();
    selector_->addCallBack( selectorcb_ );
    mAttachCB( *PolygonSelection::polygonFinished(),
		PolygonSelection::polygonChangeCB );
}


PolygonSelection::~PolygonSelection()
{
    detachAllNotifiers();
    unRefPtr( utm2disptransform_ );
    unRefPtr( drawstyle_ );
    unRefPtr( mastercamera_ );
    selector_->removeCallBack( selectorcb_ );
    selectorcb_->unref();
    delete polygon_;
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


void PolygonSelection::setMasterCamera( Camera* maincam )
{
    mastercamera_ = maincam;
    mastercamera_->ref();
    mDynamicCastGet(osg::Camera*,osgcamera,mastercamera_->osgNode());
    selector_->setEventHandlerCamera( osgcamera );
}


void PolygonSelection::setLineStyle( const LineStyle& lst )
{
    drawstyle_->setLineStyle( lst );
}


const LineStyle& PolygonSelection::getLineStyle() const
{ return drawstyle_->lineStyle(); }


void PolygonSelection::clear()
{ selector_->clear(); }


bool PolygonSelection::hasPolygon() const
{
    return selector_->getCoords()->size() > 2;
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
					    osg::Vec3((float) checkcoord3d.x,
						      (float) checkcoord3d.y,
						      (float) checkcoord3d.z) );

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
    coords[0] = Coord3( start.x, start.y, start.z );
    coords[1] = Coord3( start.x, start.y, stop.z );
    coords[2] = Coord3( start.x, stop.y, start.z );
    coords[3] = Coord3( start.x, stop.y, stop.z );
    coords[4] = Coord3( stop.x, start.y, start.z );
    coords[5] = Coord3( stop.x, start.y, stop.z );
    coords[6] = Coord3( stop.x, stop.y, start.z );
    coords[7] = Coord3( stop.x, stop.y, stop.z );

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
		      osg::Vec3((float) coords[idx].x,
		      (float) coords[idx].y,(float) coords[idx].z ) );

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
    const bool res = polygon_->isInside( screenpts );

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
    delete polygon_;
    polygon_ = 0;
    polygonlock_.writeUnLock();
}


void PolygonSelection::setUTMCoordinateTransform( const mVisTrans* nt )
{
    if ( utm2disptransform_ ) utm2disptransform_->unRef();
    utm2disptransform_ = nt;
    if ( utm2disptransform_ ) utm2disptransform_->ref();
}


PolygonCoord3Selector::PolygonCoord3Selector( const PolygonSelection& vs )
    : vissel_( vs )
{
    vissel_.ref();
}


PolygonCoord3Selector::~PolygonCoord3Selector()
{ vissel_.unRef(); }


Selector<Coord3>* PolygonCoord3Selector::clone() const
{
    mDeclareAndTryAlloc(Selector<Coord3>*,res,PolygonCoord3Selector(vissel_));
    return res;
}


const char* PolygonCoord3Selector::selectorType() const
{ return "PolygonCoord3Selector"; }


bool PolygonCoord3Selector::isOK() const
{ return hasPolygon(); }


bool PolygonCoord3Selector::hasPolygon() const
{ return vissel_.hasPolygon(); }


bool PolygonCoord3Selector::includes( const Coord3& c ) const
{ return vissel_.isInside( c, false ); }


char PolygonCoord3Selector::includesRange( const Coord3& start,
					  const Coord3& stop ) const
{ return vissel_.includesRange( start, stop, false ); }


bool PolygonCoord3Selector::isEq( const Selector<Coord3>& comp ) const
{
    mDynamicCastGet( const PolygonCoord3Selector*, b, &comp );
    if ( !b ) return false;

    return &b->vissel_ == &vissel_;
}


}; // namespace visBase
