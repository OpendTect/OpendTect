/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 2007
-*/

static const char* rcsID = "$Id$";


#include "SoScale3Dragger.h"

#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/projectors/SbLineProjector.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/SbLinear.h>

#include "SoShapeScale.h"


SO_KIT_SOURCE(SoScale3Dragger);

void SoScale3Dragger::initClass()
{
    SO_KIT_INIT_CLASS(SoScale3Dragger, SoDragger, "Dragger" );
}


#define mAddTransCatalogEntries( nm ) \
    SO_KIT_ADD_CATALOG_ENTRY( nm, SoSeparator, true, nm##Switch, \
	    		      nm##Active, true ); \
    SO_KIT_ADD_CATALOG_ENTRY( nm##Active, SoSeparator, true, nm##Switch, \
	    		      "", true )

SoScale3Dragger::SoScale3Dragger()
    : lineProj_ ( new SbLineProjector )
{
    SO_KIT_CONSTRUCTOR(SoScale3Dragger);

    SO_KIT_ADD_CATALOG_ENTRY( xMaxTransSwitch, SoSwitch, false, geomSeparator,
	    		      xMinTransSwitch, false );
    SO_KIT_ADD_CATALOG_ENTRY( xMinTransSwitch, SoSwitch, false, geomSeparator,
	    		      yMaxTransSwitch, false );
    SO_KIT_ADD_CATALOG_ENTRY( yMaxTransSwitch, SoSwitch, false, geomSeparator,
	    		      yMinTransSwitch, false );
    SO_KIT_ADD_CATALOG_ENTRY( yMinTransSwitch, SoSwitch, false, geomSeparator,
	    		      zMaxTransSwitch, false );
    SO_KIT_ADD_CATALOG_ENTRY( zMaxTransSwitch, SoSwitch, false, geomSeparator,
	    		      zMinTransSwitch, false );
    SO_KIT_ADD_CATALOG_ENTRY( zMinTransSwitch, SoSwitch, false, geomSeparator,
	    		      trans000Switch, false );
    SO_KIT_ADD_CATALOG_ENTRY( trans000Switch, SoSwitch, false, geomSeparator,
	    		      trans001Switch, false );
    SO_KIT_ADD_CATALOG_ENTRY( trans001Switch, SoSwitch, false, geomSeparator,
	    		      trans010Switch, false );
    SO_KIT_ADD_CATALOG_ENTRY( trans010Switch, SoSwitch, false, geomSeparator,
	    		      trans011Switch, false );
    SO_KIT_ADD_CATALOG_ENTRY( trans011Switch, SoSwitch, false, geomSeparator,
	    		      trans100Switch, false );
    SO_KIT_ADD_CATALOG_ENTRY( trans100Switch, SoSwitch, false, geomSeparator,
	    		      trans101Switch, false );
    SO_KIT_ADD_CATALOG_ENTRY( trans101Switch, SoSwitch, false, geomSeparator,
	    		      trans110Switch, false );
    SO_KIT_ADD_CATALOG_ENTRY( trans110Switch, SoSwitch, false, geomSeparator,
	    		      trans111Switch, false );
    SO_KIT_ADD_CATALOG_ENTRY( trans111Switch, SoSwitch, false, geomSeparator,
	    		      wireframeMaterial, false );
    SO_KIT_ADD_CATALOG_ENTRY( wireframeMaterial,SoMaterial,true,geomSeparator,
	    		      wireframeCoords, true );
    SO_KIT_ADD_CATALOG_ENTRY( wireframeCoords,SoCoordinate3,false,geomSeparator,
	    		      wireframePickStyle, true );
    SO_KIT_ADD_CATALOG_ENTRY(wireframePickStyle,SoPickStyle,false,geomSeparator,
	    		     wireframe, true );
    SO_KIT_ADD_CATALOG_ENTRY( wireframe, SoIndexedLineSet, false, geomSeparator,
		    	      "", true );


    mAddTransCatalogEntries(xMaxTrans);
    mAddTransCatalogEntries(xMinTrans);
    mAddTransCatalogEntries(yMaxTrans);
    mAddTransCatalogEntries(yMinTrans);
    mAddTransCatalogEntries(zMaxTrans);
    mAddTransCatalogEntries(zMinTrans);

    mAddTransCatalogEntries(trans000);
    mAddTransCatalogEntries(trans001);
    mAddTransCatalogEntries(trans010);
    mAddTransCatalogEntries(trans011);
    mAddTransCatalogEntries(trans100);
    mAddTransCatalogEntries(trans101);
    mAddTransCatalogEntries(trans110);
    mAddTransCatalogEntries(trans111);


    SO_KIT_ADD_FIELD( scale, (1, 1, 1) );
    SO_KIT_ADD_FIELD( minScale, (0.1, 0.1, 0.1) );
    SO_KIT_ADD_FIELD( maxScale, (10, 10, 10) );

    SO_KIT_INIT_INSTANCE();

    addStartCallback( &SoScale3Dragger::startCB);
    addMotionCallback( &SoScale3Dragger::motionCB);
    addFinishCallback( &SoScale3Dragger::finishCB);
    addValueChangedCallback(&SoScale3Dragger::valueChangedCB );

    fieldsensor_ =
	new SoFieldSensor( &SoScale3Dragger::fieldsensorCB, this );

    fieldsensor_->setPriority( 0 );
    setUpConnections( true, true );
    createDefaultParts();
}


SoScale3Dragger::~SoScale3Dragger()
{
    delete lineProj_;
    delete fieldsensor_;
}


SoSeparator* SoScale3Dragger::createMarker( const SbVec3f& pos,
						SoMaterial* material )
{
    SoSeparator* sep = new SoSeparator;
    sep->ref();
    SoTranslation* translation = new SoTranslation;
    sep->addChild( translation );
    translation->translation.setValue( pos );
    
    SoShapeScale* shapescale = new SoShapeScale;
    sep->addChild( shapescale );
    shapescale->restoreProportions = true;
    shapescale->dorotate = false;
    sep->addChild( material );
    sep->addChild( new SoCube );
    sep->unrefNoDelete();
    return sep;
}


#define mSetDefaultPart( nm, pos ) \
{ \
    SbString name =  #nm; \
    name += "Active"; \
    SoSeparator* p = createMarker( pos, activematerial ); \
    setPartAsDefault( name, p ); \
 \
    p = createMarker( pos, material ); \
    setPartAsDefault( #nm, p ); \
\
    name = #nm;\
    name += "Switch";\
    SoSwitch* sw = SO_GET_ANY_PART( this, name, SoSwitch);\
    switches_.append( sw ); \
    sw->whichChild = 0;\
}


void SoScale3Dragger::createDefaultParts()
{
    SoMaterial* activematerial = new SoMaterial;
    activematerial->ref();
    activematerial->ambientColor.set1Value( 0, 0.8, 0.8, 0 );

    SoMaterial* material = new SoMaterial;
    material->ref();
    material->ambientColor.set1Value( 0, 0.8, 0.8, 0.8 );

    mSetDefaultPart( xMaxTrans, SbVec3f(1.1,0,0) );
    mSetDefaultPart( xMinTrans, SbVec3f(-1.1,0,0) );


    mSetDefaultPart( xMaxTrans, SbVec3f( 1.1, 0, 0 ) );
    mSetDefaultPart( xMinTrans, SbVec3f( -1.1, 0, 0 ) );
    mSetDefaultPart( yMaxTrans, SbVec3f( 0, 1.1, 0 ) );
    mSetDefaultPart( yMinTrans, SbVec3f( 0, -1.1, 0 ) );
    mSetDefaultPart( zMaxTrans, SbVec3f( 0, 0, 1.1 ) );
    mSetDefaultPart( zMinTrans, SbVec3f( 0, 0, -1.1 ) );

    mSetDefaultPart( trans000, SbVec3f( -1, -1, -1 ) );
    mSetDefaultPart( trans001, SbVec3f( -1, -1, 1 ) );
    mSetDefaultPart( trans010, SbVec3f( -1, 1, -1 ) );
    mSetDefaultPart( trans011, SbVec3f( -1, 1, 1 ) );
    mSetDefaultPart( trans100, SbVec3f( 1, -1, -1 ) );
    mSetDefaultPart( trans101, SbVec3f( 1, -1, 1 ) );
    mSetDefaultPart( trans110, SbVec3f( 1, 1, -1 ) );
    mSetDefaultPart( trans111, SbVec3f( 1, 1, 1 ) );

    setPartAsDefault( "wireframeMaterial", material );

    SoCoordinate3* coords = SO_GET_ANY_PART( this, "wireframeCoords",
	    				     SoCoordinate3);

    coords->point.set1Value( 0, SbVec3f( -1, -1, -1 ) );
    coords->point.set1Value( 1, SbVec3f( -1, -1, 1 ) );
    coords->point.set1Value( 2, SbVec3f( -1, 1, -1 ) );
    coords->point.set1Value( 3, SbVec3f( -1, 1, 1 ) );
    coords->point.set1Value( 4, SbVec3f( 1, -1, -1 ) );
    coords->point.set1Value( 5, SbVec3f( 1, -1, 1 ) );
    coords->point.set1Value( 6, SbVec3f( 1, 1, -1 ) );
    coords->point.set1Value( 7, SbVec3f( 1, 1, 1 ) );
    coords->point.set1Value( 8, SbVec3f( -1.1, 0, 0 ) );
    coords->point.set1Value( 9, SbVec3f( 1.1, 0, 0 ) );
    coords->point.set1Value( 10, SbVec3f( 0, -1.1, 0 ) );
    coords->point.set1Value( 11, SbVec3f( 0, 1.1, 0 ) );
    coords->point.set1Value( 12, SbVec3f( 0, 0, -1.1 ) );
    coords->point.set1Value( 13, SbVec3f( 0, 0, 1.1 ) );

    SoPickStyle* pickstyle = SO_GET_ANY_PART( this, "wireframePickStyle",
	    				      SoPickStyle);
    pickstyle->style = SoPickStyle::UNPICKABLE;

    SoIndexedLineSet* lineset = SO_GET_ANY_PART( this, "wireframe",
						 SoIndexedLineSet);
    lineset->coordIndex.set1Value( 0, 0 );
    lineset->coordIndex.set1Value( 1, 1 );
    lineset->coordIndex.set1Value( 2, 3 );
    lineset->coordIndex.set1Value( 3, 2 );
    lineset->coordIndex.set1Value( 4, 0 );
    lineset->coordIndex.set1Value( 5, -1 );

    lineset->coordIndex.set1Value( 6, 4 );
    lineset->coordIndex.set1Value( 7, 5 );
    lineset->coordIndex.set1Value( 8, 7 );
    lineset->coordIndex.set1Value( 9, 6 );
    lineset->coordIndex.set1Value( 10, 4 );
    lineset->coordIndex.set1Value( 11, -1 );

    lineset->coordIndex.set1Value( 12, 0 );
    lineset->coordIndex.set1Value( 13, 4 );
    lineset->coordIndex.set1Value( 14, -1 );

    lineset->coordIndex.set1Value( 15, 1 );
    lineset->coordIndex.set1Value( 16, 5 );
    lineset->coordIndex.set1Value( 17, -1 );

    lineset->coordIndex.set1Value( 18, 2 );
    lineset->coordIndex.set1Value( 19, 6 );
    lineset->coordIndex.set1Value( 20, -1 );

    lineset->coordIndex.set1Value( 21, 3 );
    lineset->coordIndex.set1Value( 22, 7 );
    lineset->coordIndex.set1Value( 23, -1 );

    lineset->coordIndex.set1Value( 24, 8 );
    lineset->coordIndex.set1Value( 25, 9 );
    lineset->coordIndex.set1Value( 26, -1 );

    lineset->coordIndex.set1Value( 27, 10 );
    lineset->coordIndex.set1Value( 28, 11 );
    lineset->coordIndex.set1Value( 29, -1 );

    lineset->coordIndex.set1Value( 30, 12 );
    lineset->coordIndex.set1Value( 31, 13 );
    lineset->coordIndex.set1Value( 32, -1 );

    activematerial->unref();
    material->unref();
}



SbBool SoScale3Dragger::setUpConnections( SbBool onoff, SbBool doitalways )
{
    if ( !doitalways && connectionsSetUp==onoff )
	return onoff;

    if ( onoff )
    {
	SoDragger::setUpConnections( onoff, doitalways );

	fieldsensorCB(this, 0 );
	if ( fieldsensor_->getAttachedField()!=&scale )
	    fieldsensor_->attach(&scale);
    }
    else
    {
	if ( fieldsensor_->getAttachedField() )
	    fieldsensor_->detach();

	SoDragger::setUpConnections( onoff, doitalways );
    }

    return !(connectionsSetUp=onoff);
}


void SoScale3Dragger::startCB( void*, SoDragger* dragger )
{
    SoScale3Dragger* myself =
	(SoScale3Dragger*) dragger;

    myself->dragStart();
}


void SoScale3Dragger::motionCB( void*, SoDragger* dragger )
{
    SoScale3Dragger* myself =
	(SoScale3Dragger*) dragger;

    myself->drag();
}


void SoScale3Dragger::finishCB( void*, SoDragger* dragger )
{
    SoScale3Dragger* myself = (SoScale3Dragger*) dragger;

    myself->finish();
}


void SoScale3Dragger::dragStart()
{
    const SoPath* path = getPickPath();
    for ( int idx=switches_.getLength()-1; idx>=0; idx-- )
    {
	if ( path->containsNode( switches_[idx] ) )
	{
	    switches_[idx]->whichChild = 1;
	    break;
	}
    }

    SbVec3f startpt = getLocalStartingPoint();
    SbVec3f vec( 0, 0, 0 );
    for ( int idx=0; idx<3; idx++ )
    {
	if ( startpt[idx]<-0.5 ) vec[idx]=-1;
	else if ( startpt[idx]>0.5 ) vec[idx]=1;
    }

    lineProj_->setLine(SbLine(startpt, startpt + vec ) );
}


void SoScale3Dragger::drag()
{
    lineProj_->setViewVolume(getViewVolume());
    lineProj_->setWorkingSpace(getLocalToWorldMatrix());

    SbVec3f projPt = lineProj_->project(getNormalizedLocaterPosition());
    SbVec3f startPt = getLocalStartingPoint();

    SbVec3f motion = projPt;
    for ( int idx=0; idx<3; idx++ )
    {
	if ( startPt[idx]!=0.0f ) motion[idx] /= startPt[idx];
	else motion[idx] = 0;

	if ( motion[idx]<minScale.getValue()[idx] ||
	     motion[idx]>maxScale.getValue()[idx] )
	    return;
    }

    setMotionMatrix( appendScale( getStartMotionMatrix(),
		      SbVec3f(fabs(motion[0]),fabs(motion[1]),fabs(motion[2]) ),
		      SbVec3f(0,0,0) ) );
}


void SoScale3Dragger::valueChangedCB( void*, SoDragger* dragger )
{
    SoScale3Dragger* myself = (SoScale3Dragger*) dragger;

    SbMatrix motmat = myself->getMotionMatrix();
    SbVec3f trans, newscale;
    SbRotation rot, scaleorient;
    motmat.getTransform(trans, rot, newscale, scaleorient );
    
    myself->fieldsensor_->detach();
    if ( myself->scale.getValue() != newscale )
	myself->scale = newscale;
    myself->fieldsensor_->attach( &myself->scale );
}

void SoScale3Dragger::finish()
{
    for ( int idx=switches_.getLength()-1; idx>=0; idx-- )
	switches_[idx]->whichChild = 0;
}


void SoScale3Dragger::fieldsensorCB( void* dragger, SoSensor* )
{
    SoScale3Dragger* myself =
        (SoScale3Dragger*) dragger;

    SbMatrix motmat = myself->getMotionMatrix();
    myself->workFieldsIntoTransform(motmat);
    myself->setMotionMatrix(motmat);
}


