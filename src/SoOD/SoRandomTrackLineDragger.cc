/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2002
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoRandomTrackLineDragger.cc,v 1.1 2003-01-01 09:22:26 kristofer Exp $";

#include "SoRandomTrackLineDragger.h"

#include "errh.h"

#include "Inventor/draggers/SoDragPointDragger.h"
#include "Inventor/nodes/SoCoordinate3.h"
#include "Inventor/nodes/SoTriangleStripSet.h"
#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoMaterial.h"

SO_KIT_SOURCE(SoRandomTrackLineDragger);

void SoRandomTrackLineDragger::initClass()
{
    SO_KIT_INIT_CLASS(SoRandomTrackLineDragger, SoDragger, "Dragger" );
}


SoRandomTrackLineDragger::SoRandomTrackLineDragger()
{
    SO_KIT_CONSTRUCTOR(SoRandomTrackLineDragger);
    SO_KIT_ADD_CATALOG_LIST_ENTRY(subDraggers,SoGroup, false,
		    geomSeparator, feedbackSwitch, SoDragPointDragger, false );
    SO_KIT_ADD_CATALOG_ENTRY(feedbackSwitch,SoSwitch, false,
				geomSeparator, , false );
    SO_KIT_ADD_CATALOG_ENTRY(feedback, SoSeparator, false,
	    			feedbackSwitch, , false );
    SO_KIT_ADD_CATALOG_ENTRY(feedbackCoords, SoCoordinate3, false,
	    			feedback, feedbackMaterial, false );
    SO_KIT_ADD_CATALOG_ENTRY(feedbackMaterial, SoMaterial, false,
	    			feedback, feedbackStrip , false );
    SO_KIT_ADD_CATALOG_ENTRY(feedbackStrip, SoTriangleStripSet, false,
	    			feedback, , false );

    SO_KIT_ADD_FIELD(knots,(0,0));
    SO_KIT_ADD_FIELD(z0, (0));
    SO_KIT_ADD_FIELD(z1, (1));
    knots.set1Value(1, SbVec2f(1,0));

    SO_KIT_INIT_INSTANCE();

    SoSwitch* sw = SO_GET_ANY_PART( this, "feedbackSwitch", SoSwitch );
    setSwitchValue( sw, SO_SWITCH_NONE );

    knotsfieldsensor = new SoFieldSensor(&fieldChangeCB, this );
    knotsfieldsensor->setPriority(0);
    z0fieldsensor = new SoFieldSensor(&fieldChangeCB, this );
    z0fieldsensor->setPriority(0);
    z1fieldsensor = new SoFieldSensor(&fieldChangeCB, this );
    z1fieldsensor->setPriority(0);

    setUpConnections( true, true );
}


SoRandomTrackLineDragger::~SoRandomTrackLineDragger()
{
    delete knotsfieldsensor;
    delete z0fieldsensor;
    delete z1fieldsensor;
}


void SoRandomTrackLineDragger::startCB( void* parent, SoDragger* )
{
    SoRandomTrackLineDragger* myself =
	reinterpret_cast<SoRandomTrackLineDragger*>(parent);
    myself->dragStart();
}


void SoRandomTrackLineDragger::motionCB( void* parent, SoDragger* dragger )
{
    SoRandomTrackLineDragger* myself =
	reinterpret_cast<SoRandomTrackLineDragger*>(parent);
    myself->drag(dragger);
}


void SoRandomTrackLineDragger::finishCB( void* parent, SoDragger* )
{
    SoRandomTrackLineDragger* myself =
	reinterpret_cast<SoRandomTrackLineDragger*>(parent);
    myself->dragFinish();
}


void SoRandomTrackLineDragger::fieldChangeCB( void* parent, SoSensor* )
{
    SoRandomTrackLineDragger* myself = 
	reinterpret_cast<SoRandomTrackLineDragger*>(parent);

    myself->updateDraggers();
}


void SoRandomTrackLineDragger::dragStart()
{
    SoSwitch* sw = SO_GET_ANY_PART( this, "feedbackSwitch", SoSwitch );
    setSwitchValue( sw, 0 );
}


void SoRandomTrackLineDragger::drag(SoDragger* dragger_)
{
    SoDragPointDragger* dragger =
			reinterpret_cast<SoDragPointDragger*>( dragger_ );

    SoNodeKitListPart* partlist =
	SO_GET_ANY_PART( this, "subDraggers", SoNodeKitListPart );

    const int nrdraggers = partlist->getNumChildren();
    int draggerid = -1;
    for ( int idx=0; idx<nrdraggers; idx++ )
    {
	if ( dragger==partlist->getChild(idx) )
	{
	    draggerid = idx;
	    break;
	}
    }

    if ( draggerid==-1 )
    {
	pErrMsg("Hue!");
	return;
    }

    const bool istop = !(draggerid%2);
    const int knotid = draggerid/2;
    const SbVec2f oldpos = knots[knotid];

    SbVec3f newpos = dragger->translation.getValue();
    bool ischanged = false;

    if ( istop )
    {
	const float z0val = z0.getValue();
	if ( !mIS_ZERO(z0val-newpos[2]) )
	{
	    SbBool enabled = z0.enableNotify(false);
	    z0.setValue(newpos[2]);
	    z0.enableNotify(enabled);
	    ischanged = true;
	}
    }
    else
    {
	const float z1val = z1.getValue();
	if ( !mIS_ZERO(z1val-newpos[2]) )
	{
	    SbBool enabled = z1.enableNotify(false);
	    z1.setValue(newpos[2]);
	    z1.enableNotify(enabled);
	    ischanged = true;
	}
    }

    if ( !mIS_ZERO( oldpos[0]-newpos[0]) || !mIS_ZERO( oldpos[1]-newpos[1]) )
    {
	SbBool enabled = knots.enableNotify(false);
	knots.set1Value( knotid, SbVec2f( newpos[0], newpos[1] ));
	knots.enableNotify( enabled );
	ischanged = true;
    }

    if ( ischanged )
	knots.touch();
}


void SoRandomTrackLineDragger::dragFinish()
{
    SoSwitch* sw = SO_GET_ANY_PART( this, "feedbackSwitch", SoSwitch );
    setSwitchValue( sw, SO_SWITCH_NONE );
}


void SoRandomTrackLineDragger::updateDraggers()
{
    SoNodeKitListPart* partlist = 
	SO_GET_ANY_PART( this, "subDraggers", SoNodeKitListPart );

    SoCoordinate3* coords =
	SO_GET_ANY_PART( this, "feedbackCoords", SoCoordinate3 );

    const int nrknots = knots.getNum();

    while ( partlist->getNumChildren()<nrknots*2 )
    {
	SoDragPointDragger* newdragger = new SoDragPointDragger;
	partlist->addChild( newdragger );
	newdragger->addStartCallback( &startCB, this );
	newdragger->addMotionCallback( &motionCB, this );
	newdragger->addFinishCallback( &finishCB, this );
    }

    while ( partlist->getNumChildren()>nrknots*2 )
	partlist->removeChild( partlist->getNumChildren()-1 );

    const int nrdraggers = partlist->getNumChildren();
    for ( int idx=0; idx<nrdraggers; idx++ )
    {
	const int knotid = idx/2;
	const bool istop = !(idx%2);

	const SbVec2f knotpos = knots[knotid];

	SbVec3f draggerpos( knotpos[0], knotpos[0],
				istop ? z0.getValue() : z1.getValue());

	SoDragPointDragger* curdragger =
	    reinterpret_cast<SoDragPointDragger*>(partlist->getChild(idx));

	curdragger->translation.setValue(draggerpos);
	coords->point.set1Value(idx, draggerpos );
    }

    SoTriangleStripSet* strips =
	SO_GET_ANY_PART( this, "feedbackStrip", SoTriangleStripSet );

    strips->numVertices.setValue(nrdraggers);
}


SbBool SoRandomTrackLineDragger::setUpConnections( SbBool onOff,
						   SbBool doItAlways )
{
    if ( !doItAlways && connectionsSetUp==onOff )
	return onOff;

    if ( onOff )
    {
	SoDragger::setUpConnections( onOff, doItAlways );
	fieldChangeCB( this, 0 );
	if ( knotsfieldsensor->getAttachedField()!= &knots )
	    knotsfieldsensor->attach(&knots);
	if ( z0fieldsensor->getAttachedField()!= &z0 )
	    z0fieldsensor->attach(&z0);
	if ( z1fieldsensor->getAttachedField()!= &z1 )
	    z1fieldsensor->attach(&z1);
    }
    else
    {
	if ( knotsfieldsensor->getAttachedField() )
	    knotsfieldsensor->detach();
	if ( z0fieldsensor->getAttachedField() )
	    z0fieldsensor->detach();
	if ( z1fieldsensor->getAttachedField() )
	    z1fieldsensor->detach();

	SoDragger::setUpConnections( onOff, doItAlways );
    }
    
    return !(connectionsSetUp = onOff);
}


