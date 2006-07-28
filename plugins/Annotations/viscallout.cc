/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          Jan 2005
 RCS:           $Id: viscallout.cc,v 1.2 2006-07-28 21:58:28 cvskris Exp $
________________________________________________________________________

-*/

#include "viscallout.h"

#include "pickset.h"
#include "viscoord.h"
#include "viscube.h"
#include "vismarker.h"
#include "visfaceset.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "visrotationdragger.h"
#include "vistext.h"
#include "vistransform.h"

namespace Annotations
{

class Callout : public visBase::VisualObjectImpl
{
public:
    static Callout*		create()
				mCreateDataObj(Callout);

    Sphere			getDirection() const;
    Notifier<Callout>		moved;

    void			setPick(const Pick::Location&);
    void			setTextSize(float ns);
    void			setMarkerMaterial(visBase::Material*);
    void			setBoxMaterial(visBase::Material*);
    void			setTextMaterial(visBase::Material*);
    void			setScale(visBase::Transformation*);
    visBase::Transformation*	getScale() { return scale_; }
    void			reportChangedScale() { updateArrow(); }

    void			displayMarker(bool);
    void			setDisplayTransformation(mVisTrans*);
    int				getMarkerID() const { return marker_->id(); }

protected:
    				~Callout();
    void			updateCoords();
    void			updateArrow();
    void			setText(const char*);
    void			dragChanged(CallBacker*);
    void			dragStop(CallBacker*);

    visBase::Transformation*	displaytrans_;

    visBase::Marker*		marker_; //In normal space

    visBase::Transformation*	object2display_; //Trans to object space
    visBase::Rotation*		rotation_; 
    visBase::Transformation*	scale_;	 

    visBase::TextBox*		text_;		//In object space
    visBase::FaceSet*		faceset_;	
    visBase::IndexedPolyLine*	border_;	

    visBase::RotationDragger*	rotdragger_;	
    visBase::Cube*		rotfeedback_;	
    visBase::Cube*		rotfeedbackactive_;
    bool			isdragging_;
};


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
    , text_( visBase::TextBox::create() )
    , faceset_( visBase::FaceSet::create() )
    , border_( visBase::IndexedPolyLine::create() )
    , marker_( visBase::Marker::create() )
    , object2display_( visBase::Transformation::create() )
    , rotation_( visBase::Rotation::create() )
    , rotdragger_( visBase::RotationDragger::create() )
    , rotfeedback_( visBase::Cube::create() )
    , rotfeedbackactive_( visBase::Cube::create() )
    , scale_( 0 )
    , displaytrans_( 0 )
    , isdragging_( false )
    , moved( this )
{
    mAddChild( marker_, false );

    object2display_->ref();
    addChild( object2display_->getInventorNode() );

    rotdragger_->ref();
    rotdragger_->doAxisRotate();
    addChild( rotdragger_->getInventorNode() );
    rotdragger_->changed.notify( mCB( this, Callout, dragChanged ));
    rotdragger_->finished.notify( mCB( this, Callout, dragStop ));

    rotfeedback_->ref();
    rotfeedback_->removeSwitch();
    rotdragger_->setOwnFeedback( rotfeedback_, false );
    rotfeedbackactive_->ref();
    rotfeedbackactive_->removeSwitch();
    
    rotfeedbackactive_->setMaterial( visBase::Material::create() );
    rotfeedbackactive_->getMaterial()->setColor( Color(255,0,0) );
    rotdragger_->setOwnFeedback( rotfeedbackactive_, true );
    rotation_->ref();
    addChild( rotation_->getInventorNode() );

    mAddChild( text_, true );
    mAddChild( faceset_, true );
    mAddChild( border_, true );
    border_->setCoordinates( faceset_->getCoordinates() );
    faceset_->getCoordinates()->setPos( mPickPosIdx, Coord3(0,0,0) );

}


Callout::~Callout()
{
    text_->unRef();
    faceset_->unRef();
    border_->unRef();
    marker_->unRef();
    object2display_->unRef();
    rotation_->unRef();
    rotfeedback_->unRef();
    rotfeedbackactive_->unRef();
    rotdragger_->unRef();
    if ( scale_ ) scale_->unRef();
    if ( displaytrans_ ) displaytrans_->unRef();
}


Sphere Callout::getDirection() const
{
    Coord3 boxpos = rotation_->transform( text_->position() );
    if ( scale_ ) boxpos = scale_->transform( boxpos );
    boxpos = object2display_->transform( boxpos );
    if ( displaytrans_ ) boxpos = displaytrans_->transformBack( boxpos );

    return cartesian2Spherical( boxpos-marker_->centerPos(), true );
}


void Callout::setPick( const Pick::Location& loc )
{
    if ( isdragging_ ) return;

    marker_->setCenterPos( loc.pos );

    const Coord3 vector = spherical2Cartesian( loc.dir, true );
    Coord3 boxpos = loc.pos+vector;
    Coord3 pickpos = loc.pos;
    if ( displaytrans_ )
    {
	boxpos = displaytrans_->transform( boxpos );
	pickpos = displaytrans_->transform( pickpos );
    }

    object2display_->setTranslation( pickpos );
    boxpos = object2display_->transformBack( boxpos );
    if ( scale_ ) boxpos = scale_->transformBack( boxpos );
    boxpos = rotation_->transformBack( boxpos );
    text_->setPosition( boxpos );

    const Quaternion rot1( Coord3( 0,0,1 ), loc.dir.phi );
    static const Quaternion rot2( Coord3( 1, 0, 0 ), M_PI_2 );

    rotation_->set( rot1*rot2 );
    rotdragger_->set( rot1 );

    if ( loc.text && strcmp( loc.text->buf(), text_->getText() ) )
	setText( loc.text->buf() );
}


void Callout::setTextSize( float ns )
{
    text_->setSize(ns);
    updateCoords();

    const Coord3 feedbacksz( ns/5, ns/5, ns/5 );
    rotfeedback_->setWidth(feedbacksz);
    rotfeedbackactive_->setWidth(feedbacksz);
}


void Callout::dragChanged( CallBacker* )
{
    isdragging_ = true;
    const Quaternion rot1 = rotdragger_->get();

    static const Quaternion rot2( Coord3( 1, 0, 0 ), M_PI_2 );

    rotation_->set( rot1*rot2 );
}


void Callout::dragStop( CallBacker* )
{
    if ( !isdragging_ )
	return;

    isdragging_ = false;
    moved.trigger();
}


void Callout::setMarkerMaterial( visBase::Material* mat )
{ marker_->setMaterial( mat ); }


void Callout::setBoxMaterial( visBase::Material* mat )
{ faceset_->setMaterial( mat ); }


void Callout::setTextMaterial( visBase::Material* mat )
{ text_->setMaterial( mat ); }


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
    Coord3 minpos, maxpos;
    if ( text_->getBoundingBox(minpos, maxpos) )
    {
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

	const Coord3 feedbackpos( c11.x, c11.z, c11.y );
	rotfeedback_->setCenterPos( feedbackpos );
	rotfeedbackactive_->setCenterPos( feedbackpos );
    }

    if ( faceset_->nrCoordIndex()<2 )
    {
	for ( int idx=0; idx<4; idx++ )
	    faceset_->setCoordIndex( idx, idx*3 );

	faceset_->setCoordIndex( mLastBoxCI, -1 );
    }

    updateArrow();
}


void Callout::updateArrow()
{

    Interval<float> xrange, yrange;
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
	if ( !mIsZero( pickpos.z, 1e-3) || !xrange.includes(pickpos.x) ||
	     !yrange.includes(pickpos.y) )
	{
	    float minsqdist;
	    int startidx;

	    for ( int idx=0; idx<12; idx++ )
	    {
		const int nextidx = (idx+1)%12;
		const Coord3 pos = faceset_->getCoordinates()->getPos( idx );
		const Coord3 nextpos =
		    faceset_->getCoordinates()->getPos( nextidx );

		const float sqdist = pickpos.sqDistance(pos) +
		    		     pickpos.sqDistance(nextpos);

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


CalloutDisplay::CalloutDisplay()
    : markermaterial_( visBase::Material::create() )
    , boxmaterial_( visBase::Material::create() )
    , textmaterial_( visBase::Material::create() )
    , scale_( 1 )
{
    markermaterial_->ref();
    boxmaterial_->ref();
    textmaterial_->ref();
}


CalloutDisplay::~CalloutDisplay()
{
    setSceneEventCatcher(0);
    markermaterial_->unRef();
    boxmaterial_->unRef();
    textmaterial_->unRef();
}


void CalloutDisplay::setScale( float ns )
{
    scale_ = ns*10;
    for ( int idx=group_->size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( Callout*, call, group_->getObject(idx) );
	call->setTextSize(scale_);
    }
}


void CalloutDisplay::setScene( visSurvey::Scene* scene )
{
    if ( scene_ ) scene_->zscalechange.remove(
	    mCB( this, CalloutDisplay, zScaleChangeCB));

    visSurvey::SurveyObject::setScene( scene );

    if ( scene_ ) scene_->zscalechange.notify(
	    mCB( this, CalloutDisplay, zScaleChangeCB));
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


void CalloutDisplay::setScaleTransform( visBase::DataObject* dobj ) const
{
    mDynamicCastGet( Callout*, call, dobj );
    call->getScale()->
	setScale(Coord3(1,1,2/scene_->getZScale()));
}


float CalloutDisplay::getScale() const
{
    return scale_;
}


visBase::VisualObject* CalloutDisplay::createLocation() const
{
    Callout* res = Callout::create();
    res->setMarkerMaterial( markermaterial_ );
    res->setBoxMaterial( boxmaterial_ );
    res->setTextMaterial( textmaterial_ );
    res->setSelectable( true );
    res->setTextSize( scale_ );
    res->moved.notify( mCB( const_cast<CalloutDisplay*>(this), CalloutDisplay,
			directionChangeCB ) );

    if ( group_->size() )
    {
	mDynamicCastGet( Callout*, call, group_->getObject(0) );
	res->setScale( call->getScale() );
    }
    else
    {
	res->setScale( visBase::Transformation::create() );
	setScaleTransform( res );
    }

    return res;
}


int CalloutDisplay::isMarkerClick(const TypeSet<int>& path) const
{
    for ( int idx=group_->size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( Callout*, callout, group_->getObject(idx) );
	if ( !callout )
	    continue;
	
	if ( callout && path.indexOf(callout->getMarkerID())!=-1 )
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
