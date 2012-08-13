/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          Jan 2005
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: viscallout.cc,v 1.36 2012-08-13 03:56:43 cvssalil Exp $";

#include "viscallout.h"

#include "fontdata.h"
#include "survinfo.h"
#include "visanchor.h"
#include "viscoord.h"
#include "visdragger.h"
#include "visfaceset.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vispolygonoffset.h"
#include "vispolyline.h"
#include "visrotationdragger.h"
#include "vistristripset.h"
#include "vistext.h"
#include "vistransform.h"

#include "pickset.h"
#include "strmprov.h"
#include "uidesktopservices.h"

#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSeparator.h>


#define mTextLift 1


namespace Annotations
{

static const int txtsize = 1;

mCreateFactoryEntry( CalloutDisplay );
mCreateFactoryEntry( Callout );

#define mAddChild( var, rmswitch ) \
    var->ref(); \
    if ( rmswitch ) var->removeSwitch(); \
    addChild( var->getInventorNode() ); \
    var->setSelectable( false )

#define mPickPosIdx	12
#define mLastBoxCI	4

Callout::Callout()
    : visBase::VisualObjectImpl( false )
    , fronttext_( visBase::TextBox::create() )
    , backtext_( visBase::TextBox::create() )
    , backtextrotation_( visBase::Rotation::create() )
    , calloutoffset_( visBase::PolygonOffset::create() )
    , anchor_( visBase::Anchor::create() )
    , faceset_( visBase::FaceSet::create() )
    , marker_( visBase::Marker::create() )
    , object2display_( visBase::Transformation::create() )
    , rotation_( visBase::Rotation::create() )
    , rotdragger_( visBase::RotationDragger::create() )
    , rotfeedback_( visBase::TriangleStripSet::create() )
    , rotfeedbackactive_( visBase::TriangleStripSet::create() )
    , translationdragger_( visBase::Dragger::create() )
    , scale_( 0 )
    , zscale_( SI().zScale() )
    , displaytrans_( 0 )
    , isdragging_( false )
    , moved( this )
    , rotfeedbackpos_( 0, 0, 0 )
    , rotfeedbackradius_( 1 )
{
    mAddChild( marker_, false );
    marker_->setType( MarkerStyle3D::Sphere );
    setMaterial( 0 );

    object2display_->ref();
    addChild( object2display_->getInventorNode() );

    rotdragger_->ref();
    rotdragger_->turnOn(true); //To create switch internally
    rotdragger_->doAxisRotate();
    addChild( rotdragger_->getInventorNode() );
    rotfeedback_->setVertexOrdering( 
	   visBase::VertexShape::cClockWiseVertexOrdering() );
    rotfeedback_->setShapeType( visBase::VertexShape::cSolidShapeType() );
    rotdragger_->changed.notify( mCB( this, Callout, dragChanged ));
    rotdragger_->finished.notify( mCB( this, Callout, dragStop ));

    rotfeedback_->ref();
    rotfeedback_->removeSwitch();
    setupRotFeedback();
    rotdragger_->setOwnFeedback( rotfeedback_, false );

    rotfeedbackactive_->ref();
    rotfeedbackactive_->removeSwitch();
    rotfeedbackactive_->replaceShape( rotfeedback_->getShape() );
    
    rotfeedback_->setVertexOrdering( 0 );
    rotfeedback_->setShapeType( visBase::VertexShape::cSolidShapeType() );
    rotfeedbackactive_->setCoordinates( rotfeedback_->getCoordinates() );
    rotdragger_->setOwnFeedback( rotfeedbackactive_, true );
    rotation_->ref();
    addChild( rotation_->getInventorNode() );

    anchor_->ref();
    addChild ( anchor_->getInventorNode() );

    calloutoffset_->setStyle( visBase::PolygonOffset::Filled );
    calloutoffset_->setFactor( -2 );
    calloutoffset_->setUnits( -2 );
    anchor_->addObject( calloutoffset_ );

    faceset_->removeSwitch();
    anchor_->addObject( faceset_ );
    
    faceset_->getCoordinates()->setPos( mPickPosIdx, Coord3(0,0,0) );
    faceset_->setVertexOrdering( 0 );
    faceset_->setShapeType( visBase::VertexShape::cUnknownShapeType() );

    translationdragger_->ref();
    translationdragger_->setDraggerType( visBase::Dragger::Translate2D );
    translationdragger_->setRotation( Coord3(0,0,1), M_PI_2 );
    translationdragger_->started.notify( mCB( this, Callout, dragStart ));
    translationdragger_->motion.notify( mCB( this, Callout, dragChanged ));
    translationdragger_->finished.notify( mCB( this, Callout, dragStop ));
    addChild( translationdragger_->getInventorNode() );

    fronttext_->removeSwitch();
    anchor_->addObject( fronttext_ );

    backtextrotation_->set( Coord3( 0, 1, 0 ), M_PI );
    anchor_->addObject( backtextrotation_ );

    backtext_->removeSwitch();
    anchor_->addObject( backtext_ );
}


Callout::~Callout()
{
    anchor_->unRef();
    marker_->unRef();
    object2display_->unRef();
    rotation_->unRef();
    rotfeedback_->unRef();
    rotfeedbackactive_->unRef();
    rotdragger_->unRef();
    translationdragger_->unRef();
    if ( scale_ ) scale_->unRef();
    if ( displaytrans_ ) displaytrans_->unRef();
}

NotifierAccess&	Callout::urlClick()
{ return anchor_->click; }


int Callout::getMarkerID() const
{ return marker_->id(); }


const CallBacker* Callout::getAnchor() const
{ return anchor_; }

Sphere Callout::getDirection() const
{
    Coord3 textpos = fronttext_->position();
    textpos = Coord3( textpos.x, 0, textpos.y );
    if ( scale_ ) textpos = scale_->transform( textpos );
    textpos.z *= zscale_;

    Sphere res;
    res.theta = (float) atan2( textpos.x, textpos.z );
    res.radius = (float) textpos.abs();

    Quaternion phi( 0, 0, 0, 0 );
    rotation_->get( phi );
    static const Quaternion rot2( Coord3( 1, 0, 0 ), -M_PI_2 );
    phi *= rot2;
    Coord3 axis;
    phi.getRotation( axis, res.phi );
    if ( axis.dot(Coord3(0,0,1))<0 ) res.phi = -res.phi;
    return res;
}


void Callout::setPick( const Pick::Location& loc )
{
    if ( isdragging_ ) return;

    marker_->setCenterPos( loc.pos );
    Coord3 pickpos = loc.pos;
    if ( displaytrans_ )
	pickpos = displaytrans_->transform( pickpos );

    object2display_->setTranslation( pickpos );

    Coord3 textpos( sin(loc.dir.theta)*loc.dir.radius, 0,
	            cos(loc.dir.theta)*loc.dir.radius );
    textpos.z /= zscale_;
    if ( scale_ ) textpos = scale_->transformBack( textpos );

    fronttext_->setPosition( Coord3(textpos.x,textpos.z, mTextLift ));

    const Quaternion rot1( Coord3( 0,0,1 ), loc.dir.phi );
    static const Quaternion rot2( Coord3( 1, 0, 0 ), M_PI_2 );

    rotation_->set( rot1*rot2 );
    rotdragger_->set( rot1 );

    BufferString text;
    if ( loc.getText(CalloutDisplay::sKeyText(),text) )
	setText( text.buf() );

    anchor_->enable( loc.getText( CalloutDisplay::sKeyURL(), text ) );
}


void Callout::setTextSize( float ns )
{
    const FontData fd( mNINT32(ns) );
    fronttext_->setFontData(fd);
    backtext_->setFontData(fd);
    updateCoords();

    rotfeedbackradius_ = ns/1.5f;
    setupRotFeedback();

    const Coord3 feedbacksz( ns/3, ns/3, ns/3 );
    translationdragger_->setSize( feedbacksz );
}


void Callout::dragStart( CallBacker* cb )
{
    dragstarttextpos_ = fronttext_->position();
    dragstarttextpos_.z = 0;
    dragstartdraggerpos_ = translationdragger_->getPos();
}


void Callout::dragChanged( CallBacker* cb )
{
    if ( cb==rotdragger_ )
    {
	isdragging_ = true;
	const Quaternion rot1 = rotdragger_->get();

	static const Quaternion rot2( Coord3( 1, 0, 0 ), M_PI_2 );

	rotation_->set( rot1*rot2 );
    }
    else if ( cb==translationdragger_ )
    {
	isdragging_ = true;
	Coord3 dragpos = translationdragger_->getPos();
	dragpos.z = 0;
	Coord3 newpos = dragstarttextpos_+ dragpos -dragstartdraggerpos_;
	newpos.z = mTextLift;
	fronttext_->setPosition( newpos );
	updateCoords();
    }
}


void Callout::dragStop( CallBacker* cb )
{
    if ( !isdragging_ )
	return;

    isdragging_ = false;
    moved.trigger();
}


void Callout::setMarkerMaterial( visBase::Material* mat )
{ marker_->setMaterial( mat ); }


void Callout::setActiveFeedbackMaterial( visBase::Material* mat )
{
    rotfeedbackactive_->setMaterial( mat );
    mDynamicCastGet( SoSeparator*, sep,
	   	     translationdragger_->getShape("translatorActive") );
    if ( !sep )
	return;

    for ( int idx=0; idx<sep->getNumChildren(); idx++ )
    {
	mDynamicCastGet( SoMaterial*, omat, sep->getChild( idx ) );
	if ( !omat ) continue;

	sep->insertChild( mat->getInventorNode(), idx );
	sep->removeChild( omat );
    }
}


void Callout::setFeedbackMaterial( visBase::Material* mat )
{
    rotfeedback_->setMaterial( mat );
    mDynamicCastGet( SoSeparator*, sep,
	    	     translationdragger_->getShape("translator") );
    if ( !sep )
	return;

    for ( int idx=0; idx<sep->getNumChildren(); idx++ )
    {
	mDynamicCastGet( SoMaterial*, omat, sep->getChild( idx ) );
	if ( !omat ) continue;

	sep->insertChild( mat->getInventorNode(), idx );
	sep->removeChild( omat );
    }
}


void Callout::setBoxMaterial( visBase::Material* mat )
{ faceset_->setMaterial( mat ); }


void Callout::setTextMaterial( visBase::Material* mat )
{
    fronttext_->setMaterial( mat );
    backtext_->setMaterial( mat );
}


void Callout::setScale( visBase::Transformation* nt )
{
    if ( scale_ )
    {
	removeChild( scale_->getInventorNode() );
	scale_->unRef();
    }

    const int insertidx = childIndex( rotdragger_->getInventorNode() );

    scale_ = nt;
    scale_->ref();
    insertChild( insertidx, scale_->getInventorNode() );
}
    

void Callout::displayMarker( bool yn )
{
    marker_->turnOn( yn );
    rotdragger_->turnOn( yn );
    translationdragger_->turnOn( yn );
}


bool Callout::isMarkerDisplayed() const
{
    return marker_->isOn();
}


void Callout::setDisplayTransformation( const mVisTrans* nt )
{
    if ( displaytrans_ )
    {
	pErrMsg( "Object not designed for this" );
	return;
    }

    marker_->setDisplayTransformation( nt );

    displaytrans_ = nt;
    if ( displaytrans_ )
	displaytrans_->ref();
}


void Callout::setText( const char* txt )
{
    fronttext_->setText( txt );
    backtext_->setText( txt );
    updateCoords();
}


void Callout::updateCoords()
{
    Coord3 minpos, maxpos;
    if ( fronttext_->getBoundingBox(minpos, maxpos) )
    {
	minpos.z = maxpos.z = 0;
	const Coord3 center = (minpos+maxpos)/2;
	Coord3 hwidth = (maxpos-minpos)/2;
	hwidth.x *= 1.1;
	minpos = center-hwidth;
	maxpos = center+hwidth;

	const Coord3 c00 = minpos;
	const Coord3 c01( minpos.x, maxpos.y, 0 );
	const Coord3 c11( maxpos );
	const Coord3 c10( maxpos.x, minpos.y, 0 );
	faceset_->getCoordinates()->setPos( 0, c00 );
	faceset_->getCoordinates()->setPos( 1, (2*c00+c01)/3 );
	faceset_->getCoordinates()->setPos( 2, (c00+2*c01)/3 );
	faceset_->getCoordinates()->setPos( 3, c01 );
	faceset_->getCoordinates()->setPos( 4, (2*c01+c11)/3 );
	faceset_->getCoordinates()->setPos( 5, (c01+2*c11)/3 );
	faceset_->getCoordinates()->setPos( 6, c11 );
	faceset_->getCoordinates()->setPos( 7, (2*c11+c10)/3 );
	faceset_->getCoordinates()->setPos( 8, (c11+2*c10)/3 );
	faceset_->getCoordinates()->setPos( 9, c10 );
	faceset_->getCoordinates()->setPos( 10, (2*c10+c00)/3 );
	faceset_->getCoordinates()->setPos( 11, (c10+2*c00)/3 );

	const bool dragcorner11 = fabs(c11.x)>fabs(c01.x);
	const Coord3& dragcorner = dragcorner11 ? c11 : c01;

	rotfeedbackpos_ =  Coord3( dragcorner.x, dragcorner.z, dragcorner.y );
	if ( dragcorner11 ) 
	    rotfeedbackradius_ = fronttext_->getFontData().pointSize()/1.5f;
	else
	    rotfeedbackradius_ = -fronttext_->getFontData().pointSize()/1.5f;

	setupRotFeedback();
	if ( !isdragging_ )
	    translationdragger_->setPos( dragcorner );
    }

    if ( faceset_->nrCoordIndex()<2 )
    {
	for ( int idx=0; idx<4; idx++ )
	    faceset_->setCoordIndex( idx, idx*3 );

	faceset_->setCoordIndex( mLastBoxCI, -1 );
    }

    Coord3 backtextpos = fronttext_->position() +
	 Coord3( maxpos.x-minpos.x-backtext_->getFontData().pointSize()/10,
		 0, 0 );
    backtextpos = backtextrotation_->transform( backtextpos );
    backtext_->setPosition( Coord3(backtextpos.x, backtextpos.y, mTextLift ) );

    updateArrow();
}


void Callout::updateArrow()
{
    Interval<double> xrange, yrange;
    if ( faceset_->getCoordinates()->size(false) != 
	 faceset_->getCoordinates()->size(true) )
	return;

    for ( int idx=0; idx<12 && idx<faceset_->getCoordinates()->size(true);
	    idx+=3 )
    {
	const Coord3 pos = faceset_->getCoordinates()->getPos( idx );
	if ( !idx )
	{
	    xrange.start = xrange.stop = pos.x;
	    yrange.start = yrange.stop = pos.y;
	}
	else
	{
	    xrange.include( pos.x );
	    yrange.include( pos.y );
	}
    }

    if ( faceset_->getCoordinates()->size(true)>mPickPosIdx )
    {
	const Coord3 pickpos =
	    faceset_->getCoordinates()->getPos( mPickPosIdx );
	if ( !mIsZero( pickpos.z, 1e-3) || !xrange.includes(pickpos.x,false) ||
	     !yrange.includes(pickpos.y,false) )
	{
	    float minsqdist;
	    int startidx;

	    for ( int idx=0; idx<12; idx++ )
	    {
		const int nextidx = (idx+1)%12;
		const Coord3 pos = faceset_->getCoordinates()->getPos( idx );
		const Coord3 nextpos =
		    faceset_->getCoordinates()->getPos( nextidx );

		const float sqdist = (float) (pickpos.sqDistTo(pos) +
		    		     pickpos.sqDistTo(nextpos));

		if ( !idx || sqdist<minsqdist )
		{
		    startidx = idx;
		    minsqdist = sqdist;
		}
	    }

	    const int nextidx = (startidx+1)%12;
	    faceset_->setCoordIndex( mLastBoxCI+1, startidx );
	    faceset_->setCoordIndex( mLastBoxCI+2, mPickPosIdx );
	    faceset_->setCoordIndex( mLastBoxCI+3, nextidx );
	    faceset_->setCoordIndex( mLastBoxCI+4, -1 );
	    return;
	}
    }

    faceset_->removeCoordIndexAfter( mLastBoxCI );
}


#define mArrowAngle (M_PI/8)


void Callout::setupRotFeedback()
{
    const float angleperstep=M_PI/16;
    const int nrsteps = 4;
    const float width = rotfeedbackradius_*0.03f;
    const int coneres = 16;
    const int cylinderres = 4;

    TypeSet<Coord3> conebase;
    const Coord3 center( rotfeedbackradius_, 0, 0 );
    for ( int idx=0; idx<coneres; idx++ )
    {
	const float angle = (float) (M_PI*2)/coneres*idx;
	const float sina = sin( angle ) * width * 5;
	const float cosa = cos( angle ) * width * 5;

	conebase += Coord3( rotfeedbackradius_+cosa, 0, sina );
    }

    TypeSet<Coord3> circlepos;
    for ( int idx=0; idx<cylinderres; idx++ )
    {
	const float angle = (float) (M_PI*2)/cylinderres*idx;
	const float sina = sin( angle ) * width;
	const float cosa = cos( angle ) * width;

	circlepos += Coord3( rotfeedbackradius_+cosa, 0, sina );
    }

    int ci = 0, cii=0;

    float curangle = -(angleperstep*nrsteps)/2;
    float arrowangle = (float) (curangle - mArrowAngle);

    Quaternion rot( Coord3( 0, 0, 1 ), arrowangle );
    rotfeedback_->getCoordinates()->setPos( ci++,
	    rotfeedbackpos_+rot.rotate( center ) );

    rot.setRotation( Coord3( 0, 0, 1 ), curangle );
    rotfeedback_->getCoordinates()->setPos( ci++,
	    rotfeedbackpos_+rot.rotate( center ) );

    for ( int idx=0; idx<conebase.size(); idx++ )
    {
	rotfeedback_->getCoordinates()->setPos( ci++,
		rotfeedbackpos_+rot.rotate( conebase[idx] ) );
    }

    int topidx = 0;
    int centeridx = topidx+1;
    int baseidx = centeridx+1;

    for ( int idx=0; idx<conebase.size(); idx++ )
    {
	const int nextidx = (idx+1)%conebase.size();
	rotfeedback_->setCoordIndex( cii++, topidx );
	rotfeedback_->setCoordIndex( cii++, baseidx+idx );
	rotfeedback_->setCoordIndex( cii++, baseidx+nextidx );
	rotfeedback_->setCoordIndex( cii++, centeridx );
	rotfeedback_->setCoordIndex( cii++, -1 );
    }

    int partstart = ci;
    for ( int idx=0; idx<circlepos.size(); idx++ )
    {
	rotfeedback_->getCoordinates()->setPos( ci++,
		rotfeedbackpos_+rot.rotate( circlepos[idx] ) );
    }

    curangle += angleperstep;
    for ( int idx=0; idx<nrsteps; idx++, curangle+=angleperstep )
    {
	rot.setRotation( Coord3( 0, 0, 1 ), curangle );
	for ( int idy=0; idy<circlepos.size(); idy++ )
	{
	    rotfeedback_->getCoordinates()->setPos( ci++,
		    rotfeedbackpos_+rot.rotate( circlepos[idy] ) );
	}

	for ( int idy=0; idy<=circlepos.size(); idy++ )
    	{
	    const int index = idy%circlepos.size();
	    rotfeedback_->setCoordIndex( cii++, partstart+index );
	    rotfeedback_->setCoordIndex( cii++,
		    partstart+index+circlepos.size() );
	}

	rotfeedback_->setCoordIndex( cii++, -1 );
	partstart += circlepos.size();
    }

    baseidx = ci;

    for ( int idx=0; idx<conebase.size(); idx++ )
    {
	rotfeedback_->getCoordinates()->setPos( ci++,
	    rotfeedbackpos_+rot.rotate( conebase[idx] ) );
    }

    centeridx = ci;
    rotfeedback_->getCoordinates()->setPos( ci++,
	    rotfeedbackpos_+rot.rotate( center ) );

    curangle += (float) mArrowAngle-angleperstep;
    rot.setRotation( Coord3( 0, 0, 1 ), curangle );

    topidx =  ci;
    rotfeedback_->getCoordinates()->setPos( ci++,
	    rotfeedbackpos_+rot.rotate( center ) );

    for ( int idx=0; idx<conebase.size(); idx++ )
    {
	const int nextidx = (idx+1)%conebase.size();
	rotfeedback_->setCoordIndex( cii++, topidx );
	rotfeedback_->setCoordIndex( cii++, baseidx+nextidx );
	rotfeedback_->setCoordIndex( cii++, baseidx+idx );
	rotfeedback_->setCoordIndex( cii++, centeridx );
	rotfeedback_->setCoordIndex( cii++, -1 );
    }
}


CalloutDisplay::CalloutDisplay()
    : boxmaterial_(visBase::Material::create())
    , textmaterial_(visBase::Material::create())
    , activedraggermaterial_(visBase::Material::create())
    , scale_(1)
{
    boxmaterial_->ref();
    activedraggermaterial_->ref();
    activedraggermaterial_->setColor( Color(255,255,0) );
    boxmaterial_->setColor( Color(178,178,178) );
    boxmaterial_->setDiffIntensity( 0.4, 0 );
    textmaterial_->ref();
    textmaterial_->setColor( Color(0, 0, 0) );
}


CalloutDisplay::~CalloutDisplay()
{
    setSceneEventCatcher(0);
    boxmaterial_->unRef();
    textmaterial_->unRef();
    activedraggermaterial_->unRef();

    if ( scene_ )
	scene_->zstretchchange.remove( mCB(this,CalloutDisplay,zScaleChangeCB));
}


void CalloutDisplay::setDisplayTransformation( const mVisTrans* tr )
{
    visSurvey::LocationDisplay::setDisplayTransformation( tr );
    fullRedraw();
}


void CalloutDisplay::setScale( float ns )
{
    scale_ = ns*10;
    for ( int idx=group_->size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( Callout*, call, group_->getObject(idx) );
	call->setTextSize( scale_ );
    }
}


void CalloutDisplay::setBoxColor( const Color& col )
{ boxmaterial_->setColor( col ); }

const Color& CalloutDisplay::getBoxColor() const
{ return boxmaterial_->getColor(); }


void CalloutDisplay::setScene( visSurvey::Scene* scene )
{
    if ( scene_ )
	scene_->zstretchchange.remove( mCB(this,CalloutDisplay,zScaleChangeCB));

    visSurvey::SurveyObject::setScene( scene );

    if ( scene_ )
	scene_->zstretchchange.notify( mCB(this,CalloutDisplay,zScaleChangeCB));

    zScaleChangeCB( 0 );
}


void CalloutDisplay::zScaleChangeCB( CallBacker* )
{
    if ( group_->size() )
	setScaleTransform( group_->getObject(0) );
}


void CalloutDisplay::directionChangeCB( CallBacker* cb )
{
    mDynamicCastGet( Callout*, call, cb );
    const int idx = group_->getFirstIdx( call );
    if ( idx<0 )
	return;

    (*set_)[idx].dir = call->getDirection();
    Pick::SetMgr::ChangeData cd( Pick::SetMgr::ChangeData::Changed,
				set_, idx );
    picksetmgr_->reportChange( 0, cd );
}


void CalloutDisplay::urlClickCB( CallBacker* cb )
{
    int child = -1;
    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet( Callout*, call, group_->getObject(idx) );
	if ( call->getAnchor()==cb )
	{ child = idx; break; }
    }
	    
    if ( child<0 )
	return;

    BufferString url;
    if ( (*set_)[child].text &&
	    (*set_)[child].getText( CalloutDisplay::sKeyURL(), url ) )
	uiDesktopServices::openUrl( url );
}


void CalloutDisplay::setScaleTransform( visBase::DataObject* dobj ) const
{
    if ( !scene_ ) return;
    mDynamicCastGet(Callout*,call,dobj)
    call->getScale()->
	setScale(Coord3(1,1,2/(scene_->getZScale()*scene_->getZStretch())) );
}


float CalloutDisplay::getScale() const
{
    return scale_;
}


visBase::VisualObject* CalloutDisplay::createLocation() const
{
    Callout* res = Callout::create();
    res->setMarkerMaterial( 0 );
    res->setBoxMaterial( boxmaterial_ );
    res->setTextMaterial( textmaterial_ );
    res->setFeedbackMaterial( material_ );
    res->setActiveFeedbackMaterial( activedraggermaterial_ );
    res->setSelectable( false );
    res->setTextSize( scale_ );
    res->moved.notify( mCB( const_cast<CalloutDisplay*>(this), CalloutDisplay,
			directionChangeCB ) );
    res->urlClick().notify(
	    mCB(const_cast<CalloutDisplay*>(this), CalloutDisplay,urlClickCB ));


    if ( group_->size() )
    {
	mDynamicCastGet( Callout*, call, group_->getObject(0) );
	res->setScale( call->getScale() );
	res->displayMarker( call->isMarkerDisplayed() );
    }
    else
    {
	res->setScale( visBase::Transformation::create() );
	setScaleTransform( res );
    }

    if ( scene_ )
	res->setZScale( scene_->getZScale() );

    return res;
}


void CalloutDisplay::showManipulator( bool yn )
{
    if ( isManipulatorShown()==yn )
	return;

    for ( int idx=group_->size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( Callout*, callout, group_->getObject(idx) );
	if ( !callout )
	    continue;

	callout->displayMarker( yn );
    }
}
	

bool CalloutDisplay::isManipulatorShown() const
{
    if ( !group_->size() )
	return false;

    mDynamicCastGet( Callout*, callout, group_->getObject(0) );
    if ( !callout )
	return false;

    return callout->isMarkerDisplayed();
}


int CalloutDisplay::isMarkerClick(const TypeSet<int>& path) const
{
    for ( int idx=group_->size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( Callout*, callout, group_->getObject(idx) );
	if ( !callout )
	    continue;
	
	if ( path.indexOf(callout->getMarkerID())!=-1 )
	    return idx;
    }

    return -1;
}


void CalloutDisplay::setPosition( int idx, const Pick::Location& pick )
{
    mDynamicCastGet( Callout*, call, group_->getObject(idx) );
    call->setPick( pick );
}

}; // namespace
