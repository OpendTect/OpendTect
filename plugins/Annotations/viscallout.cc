/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          Jan 2005
 RCS:           $Id: viscallout.cc,v 1.1 2006-07-03 20:02:06 cvskris Exp $
________________________________________________________________________

-*/

#include "viscallout.h"

#include "pickset.h"
#include "viscoord.h"
#include "vismarker.h"
#include "visfaceset.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistext.h"
#include "vistransform.h"
//#include "visevent.h"
//#include "color.h"
//#include "iopar.h"
//#include "linear.h"
//#include "separstr.h"
//#include "survinfo.h"

//#include "SoForegroundTranslation.h"

//#include <Inventor/nodes/SoAsciiText.h>
//#include <Inventor/nodes/SoBaseColor.h>
//#include <Inventor/nodes/SoFont.h>
//#include <Inventor/nodes/SoRotation.h>
//#include <Inventor/nodes/SoRotationXYZ.h>
//#include <Inventor/nodes/SoScale.h>
//#include <Inventor/nodes/SoSeparator.h>
//#include <Inventor/nodes/SoSwitch.h>
//#include <Inventor/nodes/SoTranslation.h>
//#include <Inventor/actions/SoGetBoundingBoxAction.h>
//
//

namespace visSurvey
{

class Callout : public visBase::VisualObjectImpl
{
public:
    static Callout*		create()
				mCreateDataObj(Callout);

    void			setBoxScale(const Coord3&);
    void			setPick(const Pick::Location&);
    void			setMarkerMaterial(visBase::Material*);
    void			setBoxMaterial(visBase::Material*);
    void			setTextMaterial(visBase::Material*);

    void			displayMarker(bool);
    void			setDisplayTransformation(mVisTrans*);

    void			setText(const char*);
protected:
    				~Callout();

    void			updateCoords();
    void			updateArrow();

    visBase::Transformation*	displaytrans_;

    visBase::Marker*		marker_; //In normal space
    visBase::Transformation*	trans_;	 //Trans to object space

    visBase::TextBox*		text_;		//In object space
    visBase::FaceSet*		faceset_;	//In object space
    visBase::IndexedPolyLine*	border_;	//In object space
};


static const int txtsize = 1;

mCreateFactoryEntry( CalloutDisplay );
mCreateFactoryEntry( Callout );

#define mAddChild( var, rmswitch ) \
    var->ref(); \
    if ( rmswitch ) var->removeSwitch(); \
    addChild( var->getInventorNode() )

#define mBoxIdx	12

Callout::Callout()
    : visBase::VisualObjectImpl( false )
    , text_( visBase::TextBox::create() )
    , faceset_( visBase::FaceSet::create() )
    , border_( visBase::IndexedPolyLine::create() )
    , marker_( visBase::Marker::create() )
    , trans_( visBase::Transformation::create() )
    , displaytrans_( 0 )
{
    mAddChild( marker_, false );

    trans_->ref();
    addChild( trans_->getInventorNode() );

    mAddChild( text_, true );
    mAddChild( faceset_, true );
    mAddChild( border_, true );
    border_->setCoordinates( faceset_->getCoordinates() );
}


Callout::~Callout()
{
    text_->unRef();
    faceset_->unRef();
    border_->unRef();
    marker_->unRef();
    trans_->unRef();
    if ( displaytrans_ ) displaytrans_->unRef();
}


void Callout::setBoxScale( const Coord3& ns )
{
    trans_->setScale( ns );
}


void Callout::setPick( const Pick::Location& loc )
{
    marker_->setCenterPos( loc.pos );

    const Coord3 vector = spherical2Cartesian( loc.dir, true );
    Coord3 boxpos = loc.pos+vector;
    if ( displaytrans_ )
	boxpos = displaytrans_->transform( boxpos );

    trans_->setTranslation( boxpos );

    const Coord3 transpos = trans_->transform( loc.pos );
    faceset_->getCoordinates()->setPos( mBoxIdx, transpos );

    if ( loc.text && strcmp( loc.text->buf(), text_->getText() ) )
    {
	text_->setText( loc.text->buf() );

	//Update Coords
    }

}


void Callout::setMarkerMaterial( visBase::Material* mat )
{ marker_->setMaterial( mat ); }


void Callout::setBoxMaterial( visBase::Material* mat )
{ faceset_->setMaterial( mat ); }


void Callout::setTextMaterial( visBase::Material* mat )
{ text_->setMaterial( mat ); }


void Callout::displayMarker( bool yn )
{ marker_->turnOn( yn ); }


void Callout::setDisplayTransformation( visBase::Transformation* nt )
{
    if ( displaytrans_ )
    {
	pErrMsg( "Object not designed for this" );
	return;
    }

    marker_->setDisplayTransformation( nt );

    displaytrans_ = nt;
    displaytrans_->ref();
}


void Callout::setText( const char* txt )
{
    text_->setText( txt );
    updateCoords();
}


void Callout::updateCoords()
{
}


void Callout::updateArrow()
{
}


CalloutDisplay::CalloutDisplay()
    : markermaterial_( visBase::Material::create() )
    , boxmaterial_( visBase::Material::create() )
    , textmaterial_( visBase::Material::create() )
    //: Text()
    //, eventcatcher_(0)
    //, movemarker_(false)
    //, moveface_(false)
{
    markermaterial_->ref();
    boxmaterial_->ref();
    textmaterial_->ref();
    /*
    setMaterial(0);
    scale_ = new SoScale;
    addChild( scale_ );

    zrotation_ = new SoRotationXYZ;
    zrotation_->axis = SoRotationXYZ::Z;
    addChild( zrotation_ );

    SoRotation* xrotation_ = new SoRotation;
    const SbVec3f orgvec( 0, 1, 0 );
    xrotation_->rotation.setValue( SbRotation(orgvec,SbVec3f(0,0,1)) );
    addChild( xrotation_ );

#define mAddOffset(off,val) \
    SoForegroundTranslation* off = new SoForegroundTranslation; \
    off->lift.setValue( val ); \
    addChild( off );

    mAddOffset(faceoffset,0.8)
    createFaceNode();
    createMarkerNode();
    mAddOffset(linetxtoffset,0.5)
    createLineNode();
    createTextNode();
    */
}


CalloutDisplay::~CalloutDisplay()
{
    setSceneEventCatcher(0);
    markermaterial_->unRef();
    boxmaterial_->unRef();
    textmaterial_->unRef();
}


void CalloutDisplay::setScale( const Coord3& ns )
{
    scale_ = ns;
    for ( int idx=group_->size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( Callout*, call, group_->getObject(idx) );
	call->setBoxScale( ns );
    }
}


Coord3 CalloutDisplay::getScale() const
{
    return scale_;
}


visBase::VisualObject* CalloutDisplay::createLocation() const
{
    Callout* res = Callout::create();
    res->setMarkerMaterial( markermaterial_ );
    res->setBoxMaterial( boxmaterial_ );
    res->setTextMaterial( textmaterial_ );
    res->setBoxScale( scale_ );

    return res;
}


void CalloutDisplay::setPosition( int idx, const Pick::Location& pick )
{
    mDynamicCastGet( Callout*, call, group_->getObject(idx) );
    call->setPick( pick );
}

/*
void CalloutDisplay::createTextNode()
{
    SoSeparator* sep = new SoSeparator;
    SoBaseColor* color = new SoBaseColor;
    color->rgb.setValue( 0, 0, 0 );
    sep->addChild( color );
    text_ = new SoAsciiText;
    sep->addChild( text_ );
    addChild( sep );
    setSize( txtsize*10 );
}


void CalloutDisplay::createLineNode()
{
    Material* linemat = Material::create();
    linemat->setColor( Color(255,0,0) );
    border_ = IndexedPolyLine::create();
    border_->setMaterial( linemat );
    addChild( border_->getInventorNode() );
}


void CalloutDisplay::createFaceNode()
{
    faceset_ = FaceSet::create();
    Material* facemat = Material::create();
    facemat->setColor( Color(255,255,127) );
    faceset_->setMaterial( facemat );
    addChild( faceset_->getInventorNode() );
}


void CalloutDisplay::createMarkerNode()
{
    SoSeparator* markersep = new SoSeparator;
    addChild( markersep );
    SoBaseColor* markercolor = new SoBaseColor;
    markercolor->rgb.setValue( 1, 0, 0 );
    markersep->addChild( markercolor );
    marker_ = visBase::Cube::create();
    const float markersz = 2*txtsize;
    marker_->setWidth( Coord3(markersz,markersz,markersz) );
    marker_->setCenterPos( Coord3::udf() );
    markersep->addChild( marker_->getInventorNode() );
}


void CalloutDisplay::setOrientation( int orientation )
{
    const RCol2Coord& b2c = SI().binID2Coord();
    const RCol2Coord::RCTransform& xtr = b2c.getTransform(true);
    const RCol2Coord::RCTransform& ytr = b2c.getTransform(false);
    const float det = xtr.det( ytr );
    const SbVec3f orgvec( 0, 1, 0 );

    float zrot_angle = 0;
    if ( orientation != 1 )
    {
	SbVec3f inlvec( ytr.c/det, -xtr.c/det, 0 );
	inlvec.normalize();
	const float angle = acos( orgvec.dot(inlvec) );
	zrot_angle = det < 0 ? angle : -angle;
    }
    else
    {
	SbVec3f crlvec( -ytr.b/det, xtr.b/det, 0 );
	crlvec.normalize();
	const float angle = acos( orgvec.dot(crlvec) );
	zrot_angle = det < 0 ? angle : -angle;
    }

    zrotation_->angle = zrot_angle;
}


void CalloutDisplay::displayMarker( bool yn )
{
    marker_->turnOn( yn );
}


void CalloutDisplay::setText( const char* newtext )
{
    text_->string.deleteValues(0);
    SeparString sepstr( newtext, '\n' );
    for ( int idx=0; idx<sepstr.size(); idx++ )
	text_->string.set1Value( idx, sepstr[idx] );

    updateBackground();
}


const char* CalloutDisplay::getText() const
{
    static BufferString res;
    res = "";
    for ( int idx=0; idx<text_->string.getNum(); idx++ )
    {
	if ( idx ) res += "\n";
	res += text_->string[idx].getString();
    }

    return res;
}


void CalloutDisplay::setLocation( const Coord3& crd, const Coord3& arrowpos )
{
    setPosition( crd );
    marker_->setCenterPos( arrowpos );
    setArrowCoord();
}


Coord3 CalloutDisplay::getMarkerLocation() const
{
    return marker_->centerPos();
}


Coord3 CalloutDisplay::getLocation() const
{
    return position();
}


void CalloutDisplay::setJustification( Justification just )
{
    if ( just==Center )
	text_->justification.setValue( SoAsciiText::CENTER );
    else if ( just==Left )
	text_->justification.setValue( SoAsciiText::LEFT );
    else
	text_->justification.setValue( SoAsciiText::RIGHT );
}


Text::Justification CalloutDisplay::justification() const
{
    if ( text_->justification.getValue() == SoAsciiText::CENTER )
	return Center;
    if ( text_->justification.getValue() == SoAsciiText::LEFT )
	return Left;

    return Right;
}


void CalloutDisplay::updateBackground()
{
    SbViewportRegion vp;
    SoGetBoundingBoxAction action( vp );
    action.apply( text_ );
    float dx, dy, dz;
    action.getBoundingBox().getSize( dx, dy, dz );
    const SbVec3f& center = action.getCenter();
    const float xrg[2] = { 0, txtsize*dx };
    const float yrg[2] = { center[1] - dy/2, txtsize*(center[1] + dy/2) };
    setBackgroundCoords( xrg, yrg );
    setBackgroundIndices();
    if ( !marker_->centerPos().isDefined() )
	marker_->setCenterPos( Coord3(xrg[0],yrg[0]-txtsize*5,0) );
    setArrowCoord();
}


void CalloutDisplay::updateArrowIndices( int startidx )
{
    Coordinates* facecoords = faceset_->getCoordinates();
    faceset_->setCoordIndex( facecoords->size()+1, startidx );
    faceset_->setCoordIndex( facecoords->size()+2, 12 );
    faceset_->setCoordIndex( facecoords->size()+3, startidx+1 );
    faceset_->setCoordIndex( facecoords->size()+4, -1 );
}


void CalloutDisplay::setArrowCoord()
{
    Coordinates& coords = *faceset_->getCoordinates();
    const Coord3& markerpos = marker_->centerPos();
    coords.setPos( 12, markerpos );

    Coord crd0( coords.getPos(0) ); Coord crd1( coords.getPos(6) );
    float ax = (crd0.y-crd1.y) / (crd0.x-crd1.x);
    LinePars lp0( crd0.y-ax*crd0.x, ax );

    crd0 = coords.getPos(3); crd1 = coords.getPos(9);
    ax = (crd0.y-crd1.y) / (crd0.x-crd1.x);
    LinePars lp1( crd0.y-ax*crd0.x, ax );

#define mCompare(op1,op2) \
    markerpos.y op1 lp0.getValue(markerpos.x) && \
    markerpos.y op2 lp1.getValue(markerpos.x)

    int startidx;
    if ( mCompare(>,<) )
	startidx = 1;
    else if ( mCompare(>,>) )
	startidx = 4;
    else if ( mCompare(<,>) )
	startidx = 7;
    else
	startidx = 10;

    updateArrowIndices( startidx );
}


void CalloutDisplay::setBackgroundCoords( const float* xrg, const float* yrg )
{
    const float dx = xrg[1] - xrg[0];
    const float dy = yrg[1] - yrg[0];

#define mCrd(xy,p) xy##rg[0] + p*d##xy/8

    Coordinates* coords = faceset_->getCoordinates();
    coords->setPos( 0, Coord3(xrg[0],yrg[0],0) );
    coords->setPos( 1, Coord3(xrg[0],mCrd(y,3),0) );
    coords->setPos( 2, Coord3(xrg[0],mCrd(y,5),0) );

    coords->setPos( 3, Coord3(xrg[0],yrg[1],0) );
    coords->setPos( 4, Coord3(mCrd(x,3),yrg[1],0) );
    coords->setPos( 5, Coord3(mCrd(x,5),yrg[1],0) );

    coords->setPos( 6, Coord3(xrg[1],yrg[1],0) );
    coords->setPos( 7, Coord3(xrg[1],mCrd(y,5),0) );
    coords->setPos( 8, Coord3(xrg[1],mCrd(y,3),0) );

    coords->setPos( 9, Coord3(xrg[1],yrg[0],0) );
    coords->setPos( 10, Coord3(mCrd(x,5),yrg[0],0) );
    coords->setPos( 11, Coord3(mCrd(x,3),yrg[0],0) );

    Coordinates* linecoords = border_->getCoordinates();
    for ( int idx=0; idx<coords->size(); idx++ )
	linecoords->setPos( idx, coords->getPos(idx) );
}


void CalloutDisplay::setBackgroundIndices()
{
    Coordinates* facecoords = faceset_->getCoordinates();
    if ( faceset_->nrCoordIndex() >= facecoords->size() ) return;

    for ( int idx=0; idx<facecoords->size(); idx++ )
    {
	faceset_->setCoordIndex( idx, idx );
	border_->setCoordIndex( idx, idx );
    }

    faceset_->setCoordIndex( facecoords->size(), -1 );
    border_->setCoordIndex( facecoords->size(), 0 );
    border_->setCoordIndex( facecoords->size()+1, -1 );
}


void CalloutDisplay::setScale( const Coord3& sc_ )
{
    scale_->scaleFactor.setValue( sc_.x, sc_.y, sc_.z );
}


Coord3 CalloutDisplay::getScale() const
{
    SbVec3f size = scale_->scaleFactor.getValue();
    return Coord3( size[0], size[1], size[2] );
}


void CalloutDisplay::setDisplayTransformation( Transformation* nt )
{
    Text::setDisplayTransformation( nt );
}


void CalloutDisplay::setSceneEventCatcher( EventCatcher* ec )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove( mCB(this,CalloutDisplay,mouseCB) );
	eventcatcher_->unRef();
    }

    eventcatcher_ = ec;
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.notify( mCB(this,CalloutDisplay,mouseCB) );
	eventcatcher_->ref();
    }
}


void CalloutDisplay::mouseCB( CallBacker* cb )
{
    if ( eventcatcher_->isEventHandled() ) return;

    mCBCapsuleUnpack(const EventInfo&,eventinfo,cb);
    const bool onmarker = eventinfo.pickedobjids.indexOf( marker_->id() ) >= 0;
    const bool onface = eventinfo.pickedobjids.indexOf( id() ) >= 0;

    if ( !eventinfo.mousebutton && eventinfo.type==visBase::MouseClick )
    {
	movemarker_ = eventinfo.pressed && onmarker;
	moveface_ = eventinfo.pressed && onface;
    }
    else if ( eventinfo.type==visBase::MouseMovement && movemarker_ )
    {
	Coord3 pickpos = eventinfo.pickedpos;
	if ( !pickpos.x && !pickpos.y ) return;
	SbVec3f txtpos = textpos->translation.getValue();
	txtpos[2] *= getScale().x / getScale().z; // correct for Z-transform
	
	Coord3 newpos( pickpos.x-txtpos[0], pickpos.z-txtpos[2], 0 );
	newpos /= getScale().x;
	
	marker_->setCenterPos( newpos );
	setArrowCoord();
	bool tocheck = true;
	eventcatcher_->eventIsHandled();
    }
    else if ( eventinfo.type==visBase::MouseMovement && moveface_ )
    {
	Coord3 pickpos = eventinfo.pickedpos;
	if ( !pickpos.x && !pickpos.y ) return;

	pickpos.z *= getScale().z / getScale().x;
	pickpos = getDisplayTransformation()
	    ? getDisplayTransformation()->transformBack( pickpos ) : pickpos;

	setPosition( pickpos );
	updateBackground();
	eventcatcher_->eventIsHandled();
    }
}
*/


}; // namespace visBase
