/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vispicksetdisplay.h"

#include "callback.h"
#include "color.h"
#include "commondefs.h"
#include "draw.h"
#include "mousecursor.h"
#include "pickset.h"
#include "zaxistransform.h"

#include "visemobjdisplay.h"
#include "visevent.h"
#include "vismaterial.h"
#include "visplanedatadisplay.h"
#include "vispolygonselection.h"
#include "visrandomtrackdisplay.h"
#include "visseis2ddisplay.h"
#include "vistransform.h"

static float cDipFactor() { return SI().zIsTime() ? 1e-6f : 1e-3f; }

namespace visSurvey {

const char* PickSetDisplay::sKeyNrPicks()       { return "No Picks"; }
const char* PickSetDisplay::sKeyPickPrefix()    { return "Pick "; }
const char* PickSetDisplay::sKeyDisplayBody()	{ return "Show Body"; }

#define mCheckReadyOnly(set) { if (set->isReadOnly()) return; }

PickSetDisplay::PickSetDisplay()
    : LocationDisplay()
    , color_(OD::Color::White())
{
    ref();
    markerset_ = visBase::MarkerSet::create();
    markerset_->applyRotationToAllMarkers( false );
    addChild( markerset_->osgNode() );
    pickselstatus_.setEmpty();
    unRefNoDelete();
}


PickSetDisplay::~PickSetDisplay()
{
    dragger_ = nullptr;
    bodydisplay_ = nullptr;
    removeChild( markerset_->osgNode() );
    markerset_ = nullptr;
    if ( polylines_ )
    {
	removeChild( polylines_->osgNode() );
	polylines_ = nullptr;
    }

    Pick::SetMgr& mgr = Pick::Mgr();
    mgr.undo().removeAll();
}


void PickSetDisplay::setSet( Pick::Set* newset )
{
    if ( !newset )
	return;

    LocationDisplay::setSet( newset );
    if ( set_->isSizeLargerThanThreshold() )
    {
	set_->disp3d().markerstyle_.type_ = MarkerStyle3D::Point;
	set_->disp3d().markerstyle_.size_ = 2;
    }

    MarkerStyle3D markerstyle;
    markerstyle.size_ = set_->disp3d().size();
    markerstyle.type_ = (MarkerStyle3D::Type) set_->disp3d().type();
    markerset_->setMaterial( nullptr );
    markerset_->setMarkerStyle( markerstyle );
    markerset_->setMarkersSingleColor( set_->disp3d().color() );

    if ( newset->isPolygon() && set_->isPolygon() && set_->disp3d().polyDisp() )
    {
	OD::LineStyle ls = set_->disp3d().polyDisp()->linestyle_;
	if ( polylines_ )
	    polylines_->setLineStyle( ls );
    }

    if ( !showall_ && scene_ )
	scene_->objectMoved( nullptr );

    RefMan<visBase::MarkerSet> marker = createOneMarker();
    dragger_= visBase::Dragger::create();
    dragger_->setDisplayTransformation( transformation_.ptr() );
    dragger_->setDraggerType( visBase::Dragger::Translate2D );
    dragger_->setSize( (float)markerstyle.size_ );
    dragger_->setOwnShape( marker.ptr(), false );
    addChild( dragger_->osgNode() );
    dragger_->turnOn( false );

    const bool dodispbody = set_->isPolygon() &&
			    set_->disp3d().polyDisp() &&
			    set_->disp3d().polyDisp()->dofill_;
    displayBody( dodispbody );
}


void PickSetDisplay::updateDragger()
{
    mCheckReadyOnly( set_ )

    if ( dragger_ )
	dragger_->updateDragger( false );
}


bool PickSetDisplay::draggerNormal() const
{ return dragger_ ? !dragger_->defaultRotation() : false; }


void PickSetDisplay::setDraggerNormal( const Coord3& normal )
{
    if ( normal.isUdf() )
	return;

    const Coord3 defnormal( 0, 0, 1 );
    const float dotproduct = (float)defnormal.dot( normal );
    Coord3 rotationaxis(0,0,1);
    float angle = 0;
    if ( !mIsEqual(dotproduct,1,1e-3) )
    {
	const float zaxisangle =
                sCast( float, Math::Atan2(normal.y_,normal.x_) );
	Quaternion rotation( defnormal, zaxisangle );
	rotation *= Quaternion( Coord3(0,1,0), -Math::ACos(dotproduct) );
	rotation.getRotation( rotationaxis, angle );
    }

    dragger_->setRotation( rotationaxis, angle );
}


bool PickSetDisplay::isPolygon() const
{ return set_ ? set_->isPolygon() : false; }


void PickSetDisplay::locChg( CallBacker* cb )
{
    LocationDisplay::locChg( cb );
    setBodyDisplay();
    if ( markerset_ )
	markerset_->forceRedraw( true );
}


void PickSetDisplay::setChg( CallBacker* cb )
{
    LocationDisplay::setChg( cb );
    setBodyDisplay();

    // For polygons, ensure smooth curve is drawn after set changes
    if ( set_ && set_->isPolygon() && set_->nrSets() <= 1 )
    redrawLine();
}


void PickSetDisplay::dispChg( CallBacker* cb )
{
    if ( !markerset_ )
	return;

    const int oldpixsz = (int)(markerset_->getScreenSize() + .5);
    if ( oldpixsz != set_->disp3d().size() )
    {
	updateLineStyle();
	markerset_->setScreenSize( sCast(float,set_->disp3d().size()) );
    }

    if ( markerset_->getType()!=(MarkerStyle3D::Type)set_->disp3d().type() )
    {
	markerset_->setType( (MarkerStyle3D::Type)set_->disp3d().type() );
	if ( set_->disp3d().type() == MarkerStyle3D::Arrow ||
	     set_->disp3d().type() == MarkerStyle3D::Plane )
	    fullRedraw(nullptr);
    }

    if ( bodydisplay_ && set_->isPolygon() && set_->disp3d().polyDisp() &&
	 bodydisplay_->getMaterial()->getColor()
				    != set_->disp3d().polyDisp()->fillcolor_ )
    {
	bodydisplay_->getMaterial()->setColor(
				    set_->disp3d().polyDisp()->fillcolor_ );
    }

    LocationDisplay::dispChg( cb );
    markerset_->setMarkersSingleColor( set_->disp3d().color() );
    markerset_->forceRedraw( true );

    showLine( needLine() );

    const bool dodispbody = set_->isPolygon() &&
			    set_->disp3d().polyDisp() &&
			    set_->disp3d().polyDisp()->dofill_;
    displayBody( dodispbody );
}


void PickSetDisplay::updateLineStyle()
{
    if ( !polylines_ || !set_ )
	return;

    polylines_->setLineStyle( set_->disp3d().polyDisp()->linestyle_ );
}


void PickSetDisplay::setPosition( int idx, const Pick::Location& loc )
{
    setPosition( idx, loc, false );
}


void PickSetDisplay::setPosition( int idx, const Pick::Location& loc, bool add )
{

    if ( add )
	markerset_->insertPos( idx, loc.pos(), true );
    else
	markerset_->setPos( idx, loc.pos(), true );
    if ( set_->disp3d().type() == MarkerStyle3D::Arrow ||
	 set_->disp3d().type() == MarkerStyle3D::Plane )
	markerset_->setSingleMarkerRotation( getDirection(loc), idx );

    if ( needLine() )
	setPolylinePos( idx, loc.pos() );
}


Coord3 PickSetDisplay::getPosition( int loc ) const
{
    const visBase::Coordinates* markercoords = markerset_->getCoordinates();
    if( markercoords->size() )
	 return markercoords->getPos( loc );

    return Coord3::udf();
}


void PickSetDisplay::setPolylinePos( int idx, const Coord3& pos )
{
	if ( !polylines_ )
	createLine();

	if ( set_->nrSets() > 1 )
	return;

	const bool doredraw = set_->isPolygon() && set_->disp3d().polyDisp() &&
		set_->disp3d().polyDisp()->connect_!=Pick::Set::Connection::None;
	if ( doredraw )
	{
	redrawLine();
	return;
	}

	polylines_->setPoint( idx, pos );
	polylines_->dirtyCoordinates();
}


void PickSetDisplay::removePosition( int idx )
{
	mCheckReadyOnly( set_ )

	const bool doredraw = set_->isPolygon() && set_->disp3d().polyDisp() &&
		set_->disp3d().polyDisp()->connect_!=Pick::Set::Connection::None;
	if ( doredraw )
	{
	redrawAll( idx );
	return;
	}

	if ( !markerset_ || idx>=markerset_->size() )
	return;

	markerset_->removeMarker( idx );
	removePolylinePos( idx );

	dragger_->turnOn( false );
}


void PickSetDisplay::removePolylinePos( int idx )
{
    mCheckReadyOnly( set_ )
    if ( !polylines_ || idx>polylines_->size() )
	return;

    bool doredraw = set_->isPolygon() && set_->disp3d().polyDisp() &&
	    set_->disp3d().polyDisp()->connect_==Pick::Set::Connection::Close;
    if ( doredraw )
    {
	redrawLine();
	return;
    }

    polylines_->removePoint( idx );

    doredraw = set_->isPolygon() && set_->disp3d().polyDisp() &&
	   set_->disp3d().polyDisp()->connect_==Pick::Set::Connection::Close;
    if ( doredraw )
	redrawLine();
}


void PickSetDisplay::redrawMultiSets()
{
    if ( !markerset_ )
	return;

    if ( !polylines_ && needLine() )
	createLine();

    markerset_->clearMarkers();
    if ( polylines_ )
    {
	polylines_->removeAllPrimitiveSets();
	polylines_->getCoordinates()->setEmpty();
    }

    if ( set_->nrSets()==0 && set_->isPolygon() && set_->disp3d().polyDisp() &&
	 set_->disp3d().polyDisp()->connect_!=Pick::Set::Connection::None )
    {
	// this is for a default new polygon, we should have a default start
	// index. it should be changed in trunk version.
	set_->addStartIdx( 0 );
    }

    for ( int idx=0; idx<set_->nrSets(); idx++ )
    {
	int start=0, stop=0;
	set_->getStartStopIdx( idx, start, stop );
	TypeSet<int> ps;
	bool first = true;
	int firstidx = 0;
	for ( int idy=start; idy<=stop; idy++ )
	{
	    Coord3 pos = set_->getPos( idy );
	    if ( datatransform_ )
                pos.z_ = datatransform_->transform( pos );
            if ( mIsUdf(pos.z_) )
		continue;

	    markerset_->addPos( pos );
	    if ( !polylines_ )
		continue;

	    ps += polylines_->getCoordinates()->addPos( pos );
	    if ( first )
	    {
		firstidx = ps[ps.size()-1];
		first = false;
	    }
	}

	const bool ispoly = set_->isPolygon() && set_->disp3d().polyDisp() &&
	    set_->disp3d().polyDisp()->connect_==Pick::Set::Connection::Close;
	if ( ispoly )
	    ps += firstidx;

	if ( ps.isEmpty() )
	    continue;

	RefMan<Geometry::IndexedPrimitiveSet> lineprimitiveset =
			    Geometry::IndexedPrimitiveSet::create( true );
	lineprimitiveset->set( ps.arr(), ps.size() );
	polylines_->addPrimitiveSet( lineprimitiveset.ptr() );
    }
}


void PickSetDisplay::redrawAll( int drageridx )
{
	if ( !markerset_ )
	return;

	if ( !polylines_ && needLine() )
	createLine();

	markerset_->clearMarkers();
	for ( int idx=0; idx<set_->size(); idx++ )
	{
	Coord3 pos = set_->getPos( idx );
	if ( datatransform_ )
			pos.z_ = datatransform_->transform( pos );
		if ( !mIsUdf(pos.z_) )
		markerset_->addPos( pos );
	}

	markerset_->forceRedraw( true );
	redrawLine();
}


void PickSetDisplay::removeAll()
{
    if ( markerset_ )
    {
	markerset_->clearMarkers();
	markerset_->forceRedraw( true );
    }

    if ( polylines_ )
    {
	polylines_->removeAllPrimitiveSets();
	polylines_->getCoordinates()->setEmpty();
	removeChild( polylines_->osgNode() );
	polylines_ = nullptr;
    }
}


void PickSetDisplay::createLine()
{
     if ( polylines_ || !set_ )
	return;

    polylines_ = visBase::PolyLine::create();

    addChild( polylines_->osgNode() );
    polylines_->setDisplayTransformation( transformation_.ptr() );
    RefMan<visBase::Material> newmat = visBase::Material::create();
    polylines_->setMaterial( newmat.ptr() );

    updateLineStyle();
}


void PickSetDisplay::redrawLine()
{
	if ( !polylines_ || !set_ )
	return;

	const bool ispoly = set_->isPolygon() && set_->disp3d().polyDisp();
	polylines_->setLineStyle( ispoly ? set_->disp3d().polyDisp()->linestyle_
					 : OD::LineStyle() );

	if ( set_->nrSets() > 1 )
	return;

	polylines_->removeAllPoints();

	// Collect all valid knot positions
	TypeSet<Coord3> rawknots;
	for ( int idx=0; idx<set_->size(); idx++ )
	{
	Coord3 pos = set_->getPos( idx );
	if ( datatransform_ )
		pos.z_ = datatransform_->transform( pos );
	if ( !mIsUdf(pos.z_) )
		rawknots += pos;
	}

	const int nrknots = rawknots.size();
	if ( nrknots == 0 )
	return;

	// For polygons with >= 2 points, apply Catmull-Rom spline interpolation
	if ( ispoly && nrknots >= 2 )
	{
	const int interpsteps = 10; // Points per segment for smooth curve

	// Add interpolated points between each pair of knots
	for ( int kidx=0; kidx<nrknots-1; kidx++ )
	{
		const Coord3& p0 = rawknots[kidx];
		const Coord3& p1 = rawknots[kidx+1];

		// Get neighboring points for polynomial interpolation
		const Coord3 pm1 = kidx > 0 ? rawknots[kidx-1] : p0;
		const Coord3 p2 = kidx < nrknots-2 ? rawknots[kidx+2] : p1;

		// Interpolate between p0 and p1 using 4-point Catmull-Rom spline
		for ( int step=0; step<interpsteps; step++ )
		{
		const float t = float(step) / float(interpsteps);
		const float t2 = t * t;
		const float t3 = t2 * t;

		Coord3 interp;
		interp.x_ = 0.5f * ((2.0f*p0.x_) +
			(-pm1.x_ + p1.x_) * t +
			(2.0f*pm1.x_ - 5.0f*p0.x_ + 4.0f*p1.x_ - p2.x_) * t2 +
			(-pm1.x_ + 3.0f*p0.x_ - 3.0f*p1.x_ + p2.x_) * t3);
		interp.y_ = 0.5f * ((2.0f*p0.y_) +
			(-pm1.y_ + p1.y_) * t +
			(2.0f*pm1.y_ - 5.0f*p0.y_ + 4.0f*p1.y_ - p2.y_) * t2 +
			(-pm1.y_ + 3.0f*p0.y_ - 3.0f*p1.y_ + p2.y_) * t3);
		interp.z_ = 0.5f * ((2.0f*p0.z_) +
			(-pm1.z_ + p1.z_) * t +
			(2.0f*pm1.z_ - 5.0f*p0.z_ + 4.0f*p1.z_ - p2.z_) * t2 +
			(-pm1.z_ + 3.0f*p0.z_ - 3.0f*p1.z_ + p2.z_) * t3);

		polylines_->addPoint( interp );
		}
	}

	// Add the final knot
	polylines_->addPoint( rawknots[nrknots-1] );

	// If closed polygon, interpolate from last knot back to first
	const bool isclosed = set_->disp3d().polyDisp()->connect_ ==
				  Pick::Set::Connection::Close;
	if ( isclosed && nrknots > 2 )
	{
		const Coord3& p0 = rawknots[nrknots-1];
		const Coord3& p1 = rawknots[0];
		const Coord3& pm1 = rawknots[nrknots-2];
		const Coord3& p2 = rawknots[1];

		for ( int step=1; step<=interpsteps; step++ )
		{
		const float t = float(step) / float(interpsteps);
		const float t2 = t * t;
		const float t3 = t2 * t;

		Coord3 interp;
		interp.x_ = 0.5f * ((2.0f*p0.x_) +
			(-pm1.x_ + p1.x_) * t +
			(2.0f*pm1.x_ - 5.0f*p0.x_ + 4.0f*p1.x_ - p2.x_) * t2 +
			(-pm1.x_ + 3.0f*p0.x_ - 3.0f*p1.x_ + p2.x_) * t3);
		interp.y_ = 0.5f * ((2.0f*p0.y_) +
			(-pm1.y_ + p1.y_) * t +
			(2.0f*pm1.y_ - 5.0f*p0.y_ + 4.0f*p1.y_ - p2.y_) * t2 +
			(-pm1.y_ + 3.0f*p0.y_ - 3.0f*p1.y_ + p2.y_) * t3);
		interp.z_ = 0.5f * ((2.0f*p0.z_) +
			(-pm1.z_ + p1.z_) * t +
			(2.0f*pm1.z_ - 5.0f*p0.z_ + 4.0f*p1.z_ - p2.z_) * t2 +
			(-pm1.z_ + 3.0f*p0.z_ - 3.0f*p1.z_ + p2.z_) * t3);

		polylines_->addPoint( interp );
		}
	}
	}
	else
	{
	// For non-polygons or single point, use linear interpolation
	for ( int idx=0; idx<nrknots; idx++ )
		polylines_->addPoint( rawknots[idx] );

	// Close the polygon if needed (for polygons with < 2 points)
	if ( ispoly && nrknots > 0 &&
		 set_->disp3d().polyDisp()->connect_==Pick::Set::Connection::Close )
		polylines_->addPoint( rawknots[0] );
	}

	polylines_->dirtyCoordinates();
}


bool PickSetDisplay::needLine()
{
    needline_ = set_->isPolygon() &&  set_->disp3d().polyDisp() &&
	    set_->disp3d().polyDisp()->connect_!=Pick::Set::Connection::None;
    return needline_;
}


void PickSetDisplay::showLine( bool yn )
{
    if ( !needLine() && !lineShown() )
	return;

    if ( !polylines_ )
	createLine();

    redrawLine();
    polylines_->turnOn( yn );
}


bool PickSetDisplay::lineShown() const
{
    return polylines_ ? polylines_->isOn() : false;
}


void PickSetDisplay::showDragger( bool yn )
{ showdragger_ = yn; }

bool PickSetDisplay::draggerShown() const
{ return showdragger_; }


static float getSurveyRotation()
{
    const Pos::IdxPair2Coord& b2c = SI().binID2Coord();
    const float xcrd = (float) b2c.getTransform(true).c;
    const float ycrd = (float) b2c.getTransform(false).c;
    const float angle = Math::Atan2( ycrd, xcrd );
    return angle;
}


::Quaternion PickSetDisplay::getDirection( const Pick::Location& loc ) const
{
    const float survngle = getSurveyRotation();
    if ( set_->disp3d().type() == MarkerStyle3D::Arrow )
    {
	const float phi = SI().isRightHandSystem() ? survngle + loc.dir().phi
					     : survngle - loc.dir().phi;
	const Quaternion azimuth( Coord3(0,0,1), phi );
	const Quaternion dip( Coord3(0,1,0), loc.dir().theta );
	const Quaternion rot = azimuth * dip;
	return rot;
    }

    const float zscale = !scene_ ? 0 :
		    scene_->getApparentVelocity(scene_->getFixedZStretch());

    const float inldepth = (loc.inlDip()*cDipFactor()) * zscale;
    const float crldepth = (loc.crlDip()*cDipFactor()) * zscale;

    const float inlangle = atan( (SI().isRightHandSystem()
			? -inldepth
			: inldepth) );
    const float crlangle = atan( crldepth );

    const Quaternion inlrot( Coord3(1,0,0), inlangle );
    const Quaternion crlrot( Coord3(0,1,0), crlangle );
    const Quaternion survrot( Coord3(0,0,1),survngle );

    const Quaternion finalrot = survrot * crlrot * inlrot;
    return finalrot;
}


bool PickSetDisplay::isBodyDisplayed() const
{
    return bodydisplay_ && shoulddisplaybody_;
}


void PickSetDisplay::displayBody( bool yn )
{
    shoulddisplaybody_ = yn;
    if ( yn && !bodydisplay_ )
	setBodyDisplay();

    if ( bodydisplay_ )
	bodydisplay_->turnOn( yn );
}


bool PickSetDisplay::setBodyDisplay()
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );

    if ( !shoulddisplaybody_ || !set_ || !set_->size() )
	return false;

    if ( !bodydisplay_ )
    {
	bodydisplay_ = visBase::RandomPos2Body::create();
	addChild( bodydisplay_->osgNode() );
	bodydisplay_->setPixelDensity( getPixelDensity() );
    }

    if ( !bodydisplay_->getMaterial() )
    {
	RefMan<visBase::Material> newmat = visBase::Material::create();
	bodydisplay_->setMaterial( newmat.ptr() );
    }

    const bool ispoly = set_->isPolygon() && set_->disp3d().polyDisp();
    bodydisplay_->getMaterial()->setColor( ispoly
				    ? set_->disp3d().polyDisp()->fillcolor_
				    : OD::Color::NoColor() );
    bodydisplay_->setDisplayTransformation( transformation_.ptr() );

    TypeSet<Coord3> picks;
    for ( int idx=0; idx<set_->size(); idx++ )
    {
	picks += set_->getPos( idx );
	if ( datatransform_ )
            picks[idx].z_ = datatransform_->transformBack( picks[idx] );
    }

    setColor( ispoly ? set_->disp3d().polyDisp()->fillcolor_
		     : OD::Color::NoColor() );

    return bodydisplay_->setPoints( picks, set_->isPolygon() );
}


RefMan<visBase::MarkerSet> PickSetDisplay::createOneMarker() const
{
    RefMan<visBase::MarkerSet> marker = visBase::MarkerSet::create();
    MarkerStyle3D markerstyle;
    markerstyle.size_ = set_->disp3d().size();
    markerstyle.type_ = sCast(MarkerStyle3D::Type,set_->disp3d().type());
    marker->setMaterial( nullptr );
    marker->setMarkerStyle( markerstyle );
    marker->setMarkersSingleColor( OD::Color::NoColor() );
    marker->addPos( Coord3(0,0,0) );
    return marker;
}


int PickSetDisplay::clickedMarkerIndex( const visBase::EventInfo& evi ) const
{
    if ( !isMarkerClick( evi ) )
	return -1;

    return markerset_->findClosestMarker( evi.displaypickedpos, true );
}


bool PickSetDisplay::isMarkerClick( const visBase::EventInfo& evi ) const
{
    return evi.pickedobjids.isPresent( markerset_->id() );
}


void PickSetDisplay::otherObjectsMoved(
			const ObjectSet<const SurveyObject>& objs,
			const VisID& movedid )
{
	mCheckReadyOnly(set_)

	// If not in "display only at sections" mode and everything is shown, nothing to do
	if ( !displayonlyatsections_ && areallshown_ )
	return;

	const bool singleobjectmoved = movedid.isValid() && movedid != id();
    ObjectSet<const SurveyObject> objstouse;
    const SurveyObject* validmovedobj = nullptr;
    for ( int idx=0; idx<objs.size(); idx++ )
    {
	mDynamicCastGet(const visBase::VisualObject*,vo,objs[idx])
	if ( vo && singleobjectmoved && vo->id() == movedid )
	    validmovedobj = objs[idx];

	mDynamicCastGet(const PlaneDataDisplay*,planedisplay,objs[idx])
	if ( planedisplay )
	{
	    objstouse += objs[idx];
	    continue;
	}

	mDynamicCastGet(const EMObjectDisplay*,emobjdisplay,objs[idx])
	if ( emobjdisplay && !emobjdisplay->displayedOnlyAtSections() )
	{
	    objstouse += objs[idx];
	    continue;
	}

	mDynamicCastGet(const RandomTrackDisplay*,rtdisplay,objs[idx])
	if ( rtdisplay )
	{
	    objstouse += objs[idx];
	    continue;
	}

	mDynamicCastGet(const Seis2DDisplay*,seis2ddisplay,objs[idx])
	if ( seis2ddisplay )
	{
	    objstouse += objs[idx];
	    continue;
	}

	if ( validmovedobj == objs[idx] )
		validmovedobj = nullptr;
	}

	// If a single object moved but we didn't find it in the active objects list,
	// it might have been turned off. In "display only at sections" mode, we still
	// need to update to hide markers that were on that section.
	if ( singleobjectmoved && !validmovedobj && !displayonlyatsections_ )
	return;

	// In "display only at sections" mode, we must process even if objstouse is empty
	// to hide all markers when no sections are present
	if ( objstouse.isEmpty() && !displayonlyatsections_ )
	return;

	for ( int idx=0; idx<markerset_->getCoordinates()->size(); idx++ )
	{
	bool showmarker = false;
	if ( !displayonlyatsections_ )
		showmarker = true;
	else
	{
	    for ( int idy=0; idy<objstouse.size(); idy++ )
	    {
		if ( updateMarkerAtSection(objstouse[idy],idx) )
		{
		    showmarker = true;
		    break;
		}
	    }

	    if ( !showmarker && areallshown_ )
		areallshown_ = false;
	}

	markerset_->turnMarkerOn( idx, showmarker );
	markerset_->requestSingleRedraw();
	}

	updateLineAtSection();

	// Force visual update
	if ( markerset_ )
	markerset_->forceRedraw( true );

	if ( !displayonlyatsections_ )
	areallshown_ = true;
	else if ( objstouse.isEmpty() )
	areallshown_ = false;  // No sections visible, so markers should be hidden
}


void PickSetDisplay::setOnlyAtSectionsDisplay( bool yn )
{
	if ( displayonlyatsections_ == yn )
	return;

	displayonlyatsections_ = yn;

	// Update showall_ to match (inverse relationship)
	showall_ = !yn;

	// Force state to be inconsistent so otherObjectsMoved will definitely process
	areallshown_ = false;

	// If switching to "show all", immediately turn on all markers and rebuild lines
	if ( !displayonlyatsections_ && markerset_ )
	{
	for ( int idx=0; idx<markerset_->getCoordinates()->size(); idx++ )
		markerset_->turnMarkerOn( idx, true );
	markerset_->forceRedraw( true );
	areallshown_ = true;

	// Also update the polylines to show all connections
	updateLineAtSection();
	}

	// Update display via scene notification
	if ( scene_ )
	scene_->objectMoved( nullptr );
}


bool PickSetDisplay::displayedOnlyAtSections() const
{
	return displayonlyatsections_;
}


bool PickSetDisplay::updateMarkerAtSection( const SurveyObject* obj, int idx )
{
    if ( !obj )
	return false;

    Coord3 pos = set_->validIdx(idx) ? set_->getPos(idx) : Coord3::udf();
    if ( !pos.isDefined() )
	return false;

    if ( datatransform_ )
        pos.z_ = datatransform_->transform( pos );

    if ( scene_ )
	scene_->getUTM2DisplayTransform()->transform( pos );

	bool onsection = false;
	if ( !pos.isDefined() )
	return false;
	else if ( !displayonlyatsections_ )
	onsection = true;
	else
    {
	mDynamicCastGet( const EMObjectDisplay*,emobj,obj )
	if ( emobj && emobj->displayedOnlyAtSections() )
	    return false;

	const float dist = obj->calcDist( pos );
	if ( dist<obj->maxDist() )
	    onsection = true;
    }

    return onsection;
}


void PickSetDisplay::updateLineAtSection()
{
   if ( polylines_ )
   {
       TypeSet<Coord3> polycoords;
       for ( int idx = 0; idx<markerset_->getCoordinates()->size(); idx++ )
	   polycoords += markerset_->getCoordinates()->getPos(idx);

	polylines_->removeAllPoints();
	int pidx = 0;
	for ( pidx=0; pidx<polycoords.size(); pidx++ )
	{
	    if ( markerset_->markerOn(pidx) )
		polylines_->addPoint( polycoords[pidx] );
	}

	if ( pidx && set_->isPolygon() && set_->disp3d().polyDisp() &&
	     set_->disp3d().polyDisp()->connect_==Pick::Set::Connection::Close )
	    polylines_->setPoint( pidx, polylines_->getPoint(0) );
    }
}


//
//void PickSetDisplay::setScene( Scene* scn )
//{
//    SurveyObject::setScene( scn );
//    if ( scene_ )
//	scene_->zstretchchange.notify(
//		mCB(this,PickSetDisplay,sceneZChangeCB) );
//}

//
//void PickSetDisplay::sceneZChangeCB( CallBacker* )
//{
//    for ( int idx=0; idx<group_->size(); idx++ )
//    {
//	mDynamicCastGet(visBase::MarkerSet*,markerset,group_->getObject(idx));
//	/*if ( markerset && scene_ )
// markerset->setZStretch( scene_->getZStretch()*scene_->getZScale()/2 );*/
//    }
//}


void PickSetDisplay::getPickingMessage( BufferString& str ) const
{
    float area = set_ ? set_->getXYArea() : mUdf(float);
    const bool hasarea = !mIsUdf(area) && area>0;
    BufferString areastring;

    if ( hasarea )
    {
	areastring.set( "Area: " )
		  .add( getAreaString(area,SI().xyInFeet(),
				      SI().nrXYDecimals(),false) );
    }

    str = isPainting() ? "Painting " : "Picking ";
    str += set_ ? set_->name() : "";
    str += " (Nr points="; str += set_ ? set_->size() : 0;
    if ( !areastring.isEmpty() ) { str +=", "; str += areastring; }
    str += ")";
}


void PickSetDisplay::setColor( OD::Color nc )
{
    if ( set_ )
	set_->disp3d().polyDisp()->fillcolor_ = nc;

    if ( !bodydisplay_ ) return;

    if ( !bodydisplay_->getMaterial() )
    {
	RefMan<visBase::Material> newmat = visBase::Material::create();
	bodydisplay_->setMaterial( newmat.ptr() );
    }

    bodydisplay_->getMaterial()->setColor( nc );
}


void PickSetDisplay::setPixelDensity( float dpi )
{
    LocationDisplay::setPixelDensity( dpi );

    if ( markerset_ )
       markerset_->setPixelDensity( dpi );
    if ( bodydisplay_ )
	bodydisplay_->setPixelDensity( dpi );
    if ( polylines_ )
	polylines_->setPixelDensity( dpi );
}


float PickSetDisplay::getPixelDensity() const
{ return markerset_->getPixelDensity(); }


void PickSetDisplay::setDisplayTransformation( const mVisTrans* newtr )
{
    LocationDisplay::setDisplayTransformation( newtr );
    if ( bodydisplay_ )
	bodydisplay_->setDisplayTransformation( newtr );
    if ( markerset_ )
	markerset_->setDisplayTransformation( newtr );
    if ( polylines_ )
	polylines_->setDisplayTransformation( newtr );
    if ( dragger_ )
	dragger_->setDisplayTransformation( newtr );
}


const mVisTrans* PickSetDisplay::getDisplayTransformation() const
{
    return transformation_.ptr();
}


void PickSetDisplay::fillPar( IOPar& par ) const
{
    LocationDisplay::fillPar( par );
    par.setYN( sKeyDisplayBody(), shoulddisplaybody_ );
}


bool PickSetDisplay::usePar( const IOPar& par )
{
    if ( !LocationDisplay::usePar(par) )
	return false;

    bool showbody = false;
    par.getYN( sKeyDisplayBody(), showbody );
    displayBody( showbody );

    return true;
}


void PickSetDisplay::turnOnSelectionMode( bool yn )
{
    ctrldown_ = false;

    if ( scene_ && scene_->getPolySelection() )
    {
	selectionmodel_ = yn;
	if ( yn )
	{
	    mAttachCBIfNotAttached(
				scene_->getPolySelection()->polygonFinished(),
				PickSetDisplay::polygonFinishedCB );
	}
	else
	{
	    mDetachCB( scene_->getPolySelection()->polygonFinished(),
		       PickSetDisplay::polygonFinishedCB );
	}
    }
}


void PickSetDisplay::polygonFinishedCB(CallBacker*)
{
    mCheckReadyOnly( set_ )

    if ( !scene_ || ! scene_->getPolySelection() )
	return;

    color_ = set_->disp3d().color();
    const int diff = markerset_->size()-pickselstatus_.size();
    if ( diff !=0 ) // added new pos or removed pos. reset
    {
	pickselstatus_.setSize( markerset_->size() );
	pickselstatus_.setAll( false );
    }

    RefMan<visBase::PolygonSelection> polysel =  scene_->getPolySelection();
    MouseCursorChanger mousecursorchanger( MouseCursor::Wait );

    if ( !polysel->hasPolygon() && !polysel->singleSelection() )
    {
	unSelectAll();
	return;
    }

    if ( !ctrldown_ )
	unSelectAll();

    updateSelections( polysel.ptr() );
    polysel->clear();

}


void PickSetDisplay::unSelectAll()
{
    markerset_->setMarkersSingleColor( color_ );
    pickselstatus_.setAll( false );
}


void PickSetDisplay::setPickSelect( int idx, bool yn )
{
    OD::Color clr = yn ? color_.complementaryColor() : color_;
    markerset_->getMaterial()->setColor( clr, idx, false );
    pickselstatus_[idx] = yn;
}


void PickSetDisplay::updateSelections( const visBase::PolygonSelection* polysel)
{
    if ( !markerset_ || !polysel || !polysel->hasPolygon() )
	return;

    const visBase::Coordinates* coords = markerset_->getCoordinates();
    if ( !coords||coords->size()==0 )
	return;

    for ( int idx=0; idx<markerset_->size(); idx++ )
    {
	const Coord3 pos = coords->getPos(idx);
	if ( !ctrldown_ )
	{
	    setPickSelect( idx, polysel->isInside(pos) );
	}
	else
	{
	    if ( polysel->isInside(pos) )
	    {
		if ( pickselstatus_[idx] )
		    setPickSelect( idx, false );
		else
		    setPickSelect( idx, true );
	    }
	}
    }

    markerset_->getMaterial()->change.trigger();
}


bool PickSetDisplay::removeSelections( TaskRunner* )
{
    MouseCursorChanger mousecursorchanger( MouseCursor::Wait );
    bool changed = false;
    for ( int idx=pickselstatus_.size()-1; idx>=0; idx-- )
    {
	if ( pickselstatus_[idx] )
	{
	    Pick::SetMgr::ChangeData cd(
		Pick::SetMgr::ChangeData::ToBeRemoved, set_.ptr(), idx );
	    set_->removeSingleWithUndo( idx );
	    Pick::Mgr().reportChange( nullptr, cd );
	    changed = true;
	}
    }

	Pick::Mgr().undo().setUserInteractionEnd(
				Pick::Mgr().undo().currentEventID() );

	unSelectAll();
	if ( changed )
	SurveyObject::removeSelections( nullptr );

	return changed;
}


void PickSetDisplay::erasePointsBetweenDragPositions()
{
	if ( !set_ || !erasestartpos_.isDefined() || !eraseendpos_.isDefined() )
	return;

	const int nrpts = set_->size();
	if ( nrpts < 2 )
	return;

	// Get sampling intervals from survey info for normalized distance calculation
	const double inl_dist = SI().inlDistance();  // Inline spacing in meters
	const double crl_dist = SI().crlDistance();  // Crossline spacing in meters
	const double z_step = SI().zStep();          // Z sample rate (e.g., 0.004 for 4ms)
	const double xy_step = (inl_dist + crl_dist) / 2.0;

	// Find the point nearest to the START position using normalized distances
	int startknotidx = -1;
	double mindist_start = 1e30;

	for ( int idx=0; idx<nrpts; idx++ )
	{
	const Coord3 ptpos = set_->getPos( idx );
	if ( !ptpos.isDefined() )
		continue;

	// Normalize distances by sampling intervals (sample space)
	const double dx_norm = (ptpos.x_ - erasestartpos_.x_) / xy_step;
	const double dy_norm = (ptpos.y_ - erasestartpos_.y_) / xy_step;
	const double dz_norm = (ptpos.z_ - erasestartpos_.z_) / z_step;

	// Compute distance in "sample space"
	const double dist = sqrt(dx_norm*dx_norm + dy_norm*dy_norm + dz_norm*dz_norm);

	if ( dist < mindist_start )
	{
		mindist_start = dist;
		startknotidx = idx;
	}
	}

	if ( startknotidx == -1 )
	return;

	// Determine direction by checking which neighbor is closer to END (also normalized)
	const int neighbor_forward = (startknotidx + 1 < nrpts) ? startknotidx + 1 : -1;
	const int neighbor_backward = (startknotidx - 1 >= 0) ? startknotidx - 1 : -1;

	double dist_forward = 1e30;
	double dist_backward = 1e30;

	if ( neighbor_forward >= 0 )
	{
	const Coord3 knotpos = set_->getPos( neighbor_forward );
	if ( knotpos.isDefined() )
	{
		const double dx_norm = (knotpos.x_ - eraseendpos_.x_) / xy_step;
		const double dy_norm = (knotpos.y_ - eraseendpos_.y_) / xy_step;
		const double dz_norm = (knotpos.z_ - eraseendpos_.z_) / z_step;
		dist_forward = sqrt(dx_norm*dx_norm + dy_norm*dy_norm + dz_norm*dz_norm);
	}
	}

	if ( neighbor_backward >= 0 )
	{
	const Coord3 knotpos = set_->getPos( neighbor_backward );
	if ( knotpos.isDefined() )
	{
		const double dx_norm = (knotpos.x_ - eraseendpos_.x_) / xy_step;
		const double dy_norm = (knotpos.y_ - eraseendpos_.y_) / xy_step;
		const double dz_norm = (knotpos.z_ - eraseendpos_.z_) / z_step;
		dist_backward = sqrt(dx_norm*dx_norm + dy_norm*dy_norm + dz_norm*dz_norm);
	}
	}

	// Determine search direction
	bool search_forward = true;
	if ( neighbor_forward < 0 && neighbor_backward >= 0 )
	search_forward = false;
	else if ( neighbor_backward < 0 && neighbor_forward >= 0 )
	search_forward = true;
	else
	search_forward = (dist_forward <= dist_backward);

	// Collect indices to delete
	TypeSet<int> indicestodelete;

	if ( search_forward )
	{
	int currentidx = startknotidx;
	const Coord3 curpos = set_->getPos(currentidx);
	const double dx_norm = (curpos.x_ - eraseendpos_.x_) / xy_step;
	const double dy_norm = (curpos.y_ - eraseendpos_.y_) / xy_step;
	const double dz_norm = (curpos.z_ - eraseendpos_.z_) / z_step;
	double prev_dist = sqrt(dx_norm*dx_norm + dy_norm*dy_norm + dz_norm*dz_norm);
	indicestodelete += currentidx;
	currentidx++;

	while ( currentidx < nrpts )
	{
		const Coord3 knotpos = set_->getPos( currentidx );
		if ( knotpos.isDefined() )
		{
		const double dx_norm = (knotpos.x_ - eraseendpos_.x_) / xy_step;
		const double dy_norm = (knotpos.y_ - eraseendpos_.y_) / xy_step;
		const double dz_norm = (knotpos.z_ - eraseendpos_.z_) / z_step;
		const double current_dist = sqrt(dx_norm*dx_norm + dy_norm*dy_norm + dz_norm*dz_norm);
		indicestodelete += currentidx;

		if ( current_dist > prev_dist )
			break;

		prev_dist = current_dist;
		}
		currentidx++;
	}
	}
	else
	{
	int currentidx = startknotidx;
	const Coord3 curpos = set_->getPos(currentidx);
	const double dx_norm = (curpos.x_ - eraseendpos_.x_) / xy_step;
	const double dy_norm = (curpos.y_ - eraseendpos_.y_) / xy_step;
	const double dz_norm = (curpos.z_ - eraseendpos_.z_) / z_step;
	double prev_dist = sqrt(dx_norm*dx_norm + dy_norm*dy_norm + dz_norm*dz_norm);
	indicestodelete += currentidx;
	currentidx--;

	while ( currentidx >= 0 )
	{
		const Coord3 knotpos = set_->getPos( currentidx );
		if ( knotpos.isDefined() )
		{
		const double dx_norm = (knotpos.x_ - eraseendpos_.x_) / xy_step;
		const double dy_norm = (knotpos.y_ - eraseendpos_.y_) / xy_step;
		const double dz_norm = (knotpos.z_ - eraseendpos_.z_) / z_step;
		const double current_dist = sqrt(dx_norm*dx_norm + dy_norm*dy_norm + dz_norm*dz_norm);
		indicestodelete += currentidx;

		if ( current_dist > prev_dist )
			break;

		prev_dist = current_dist;
		}
		currentidx--;
	}
	}

	// Sort indices in descending order
	for ( int i=0; i<indicestodelete.size()-1; i++ )
	{
	for ( int j=i+1; j<indicestodelete.size(); j++ )
	{
		if ( indicestodelete[i] < indicestodelete[j] )
		{
		const int temp = indicestodelete[i];
		indicestodelete[i] = indicestodelete[j];
		indicestodelete[j] = temp;
		}
	}
	}

	// Delete in descending order
	for ( int i=0; i<indicestodelete.size(); i++ )
	{
	const int idx = indicestodelete[i];
	if ( set_->validIdx(idx) )
		set_->removeSingleWithUndo( idx );
	}

	if ( !indicestodelete.isEmpty() )
	{
	Pick::Mgr().undo().setUserInteractionEnd(
		Pick::Mgr().undo().currentEventID() );

	// Trigger proper redraw
	redrawAll();

	// Also notify scene so section visibility is updated
	if ( scene_ )
		scene_->objectMoved( nullptr );
	}
}


void PickSetDisplay::erasePointsNearPosition( const Coord3& pos )
{
	if ( !set_ || !pos.isDefined() )
	return;

	const int nrpts = set_->size();
	if ( nrpts == 0 )
	return;

	// Get sampling intervals from survey info for normalized distance calculation
	const double inl_dist = SI().inlDistance();
	const double crl_dist = SI().crlDistance();
	const double z_step = SI().zStep();
	const double xy_step = (inl_dist + crl_dist) / 2.0;

	// Find nearest point using normalized distances
	int nearestidx = -1;
	double mindist = 1e30;

	for ( int idx=0; idx<nrpts; idx++ )
	{
	const Coord3 ptpos = set_->getPos( idx );
	if ( !ptpos.isDefined() )
		continue;

	// Normalize distances by sampling intervals (sample space)
	const double dx_norm = (ptpos.x_ - pos.x_) / xy_step;
	const double dy_norm = (ptpos.y_ - pos.y_) / xy_step;
	const double dz_norm = (ptpos.z_ - pos.z_) / z_step;

	// Compute distance in "sample space"
	const double dist = sqrt(dx_norm*dx_norm + dy_norm*dy_norm + dz_norm*dz_norm);

	if ( dist < mindist )
	{
		mindist = dist;
		nearestidx = idx;
	}
	}

	if ( nearestidx >= 0 && set_->validIdx(nearestidx) )
	{
	set_->removeSingleWithUndo( nearestidx );
	Pick::Mgr().undo().setUserInteractionEnd(
		Pick::Mgr().undo().currentEventID() );

	// Trigger proper redraw
	redrawAll();

	// Also notify scene so section visibility is updated
	if ( scene_ )
		scene_->objectMoved( nullptr );
	}
}

} // namespace visSurvey
