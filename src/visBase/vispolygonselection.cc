/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          June 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: vispolygonselection.cc,v 1.19 2012-08-10 03:50:09 cvsaneesh Exp $";

#include "vispolygonselection.h"

#include "polygon.h"
#include "vistransform.h"
#include "visdrawstyle.h"

#include "SoPolygonSelect.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>

#include <math.h>

mCreateFactoryEntry( visBase::PolygonSelection );

namespace visBase
{



Notifier<PolygonSelection>* PolygonSelection::polygonFinished()
{
    static Notifier<PolygonSelection> polygonfinished(0);
    return &polygonfinished;
}


PolygonSelection::PolygonSelection()
    : VisualObjectImpl( false )
    , transformation_( 0 )
    , selector_( new SoPolygonSelect )
    , drawstyle_( DrawStyle::create() )
    , polygon_( 0 )
{
    removeSwitch();

    drawstyle_->ref();
    addChild( drawstyle_->getInventorNode() );
    addChild( selector_ );
    selector_->polygonChange.addCallback(
	    (SoCallbackListCB*) polygonChangeCB, this );
    selector_->paintStop.addCallback(
	    (SoCallbackListCB*) paintStopCB, this );
}


PolygonSelection::~PolygonSelection()
{
    selector_->polygonChange.removeCallback(
	    (SoCallbackListCB*) polygonChangeCB, this );
    selector_->paintStop.removeCallback(
	    (SoCallbackListCB*) paintStopCB, this );
    if ( transformation_ ) transformation_->unRef();
    drawstyle_->unRef();
    delete polygon_;
}


void PolygonSelection::setSelectionType( PolygonSelection::SelectionType st )
{
    if ( st==Off )
    {
	selector_->mode = SoPolygonSelect::OFF;
	selector_->clear();
    }
    else if ( st==Rectangle )
	selector_->mode = SoPolygonSelect::RECTANGLE;
    else if ( st==Polygon )
	selector_->mode = SoPolygonSelect::POLYGON;
}


PolygonSelection::SelectionType PolygonSelection::getSelectionType() const
{
    if ( selector_->mode.getValue() == SoPolygonSelect::OFF )
	return Off;

    if ( selector_->mode.getValue() == SoPolygonSelect::RECTANGLE )
	return Rectangle;

    return Polygon;
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
    return selector_->getPolygon().getLength()>2;
}


bool PolygonSelection::isSelfIntersecting() const
{
    if ( !hasPolygon() )
	return false;

    if ( !polygon_ )
    {
	polygon_ = new ODPolygon<double>;
	const SbList<SbVec2f>& sopolygon = selector_->getPolygon();
	for ( int idx=0; idx<sopolygon.getLength(); idx++ )
	    polygon_->add( Coord( sopolygon[idx][0], sopolygon[idx][1] ) );
    }

    return polygon_->isSelfIntersecting();
}

/*
void PolygonSelection::getSelectionRays( TypeSet<Line3D>& rays ) const
{
    rays.empty();

    const SbList<SbVec2f>& sopolygon = selector_->getPolygon();
    for ( int idx=0; idx<sopolygon.getLength(); idx++ )
    {
	SbLine sodisplayspaceline;
	selector_->projectPointFromScreen( sopolygon[idx], sodisplayspaceline );
	SbVec3f pos = sodisplayspaceline.getPosition();
	SbVec3f dir = sodisplayspaceline.getDirection();

	Line3D displayspaceline( pos[0], pos[1], pos[2],
				 dir[0], dir[1], dir[2] );

	if ( transformation_ )
	    transformation_.transformBack( pos );

	rays += displayspaceline;
    }
}
*/


bool PolygonSelection::isInside( const Coord3& crd, bool displayspace ) const
{
    if ( selector_->mode.getValue() == SoPolygonSelect::OFF )
	return false;

    if ( !hasPolygon() )
	return false;

    Coord3 checkcoord3d = crd;
    if ( !displayspace && transformation_ )
	checkcoord3d = transformation_->transform( checkcoord3d );

    const SbVec2f coord2d = selector_->projectPointToScreen(
		     SbVec3f((float) checkcoord3d.x,
		    (float) checkcoord3d.y,(float) checkcoord3d.z ) );

    const Coord checkcoord2d( coord2d[0], coord2d[1] );
    if ( !checkcoord2d.isDefined() )
	return false;

    polygonlock_.readLock();
    if ( !polygon_ )
    {
	polygonlock_.convReadToWriteLock();
	if ( !polygon_ )
	{
	    polygon_ = new ODPolygon<double>;
	    const SbList<SbVec2f> sopolygon = selector_->getPolygon();
	    for ( int idx=0; idx<sopolygon.getLength(); idx++ )
		polygon_->add( Coord( sopolygon[idx][0], sopolygon[idx][1] ) );
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
    if ( selector_->mode.getValue() == SoPolygonSelect::OFF )
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

    if ( !displayspace && transformation_ )
    {
	for ( int idx=0; idx<8; idx++ )
	    coords[idx] = transformation_->transform( coords[idx] );
    }

    ODPolygon<double> screenpts;

    int nrundefs = 0;
    for ( int idx=0; idx<8; idx++ )
    {
	const SbVec2f pt = selector_->projectPointToScreen(
		      SbVec3f((float) coords[idx].x,
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
	    const SbList<SbVec2f> sopolygon = selector_->getPolygon();
	    for ( int idx=0; idx<sopolygon.getLength(); idx++ )
		polygon_->add( Coord( sopolygon[idx][0], sopolygon[idx][1] ) );
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
    const Coord3 pos = !transformation_ ? worldpos :
		       transformation_->transform( worldpos );

    const SbVec3f displaypos( (float) pos.x, (float) pos.y, (float) pos.z );
    const SoPath* path = selector_->rayPickThrough( displaypos, depthidx );
    if ( !path )
	return false;

    DM().getIds( path, pickedobjids );
    return true;
}


void PolygonSelection::polygonChangeCB( void* data, SoPolygonSelect* )
{
    PolygonSelection* myptr = (PolygonSelection*) data;
    myptr->polygonlock_.writeLock();
    delete myptr->polygon_;
    myptr->polygon_ = 0;

    myptr->polygonlock_.writeUnLock();
}


void PolygonSelection::paintStopCB( void*, SoPolygonSelect* )
{ polygonFinished()->trigger(); }


void PolygonSelection::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation_ ) transformation_->unRef();
    transformation_ = nt;
    if ( transformation_ ) transformation_->ref();
}


const mVisTrans* PolygonSelection::getDisplayTransformation() const
{ return transformation_; }


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
