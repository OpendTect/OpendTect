/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoPlaneWellLog.cc,v 1.1 2003-09-29 10:18:03 kristofer Exp $";


#include "SoPlaneWellLog.h"

#include "Inventor/actions/SoGLRenderAction.h"

#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>

#include "Inventor/nodes/SoCoordinate3.h"
#include "Inventor/nodes/SoIndexedFaceSet.h"
#include "Inventor/nodes/SoMaterial.h"
#include "Inventor/nodes/SoSeparator.h"

#include "Inventor/sensors/SoFieldSensor.h"

SO_KIT_SOURCE(SoPlaneWellLog);

void SoPlaneWellLog::initClass()
{
    SO_KIT_INIT_CLASS( SoPlaneWellLog, SoBaseKit, "BaseKit");
}


SoPlaneWellLog::SoPlaneWellLog()
    : valuesensor( new SoFieldSensor(SoPlaneWellLog::valueChangedCB, this) )
{
    SO_KIT_CONSTRUCTOR(SoPlaneWellLog);

    SO_KIT_ADD_CATALOG_ENTRY(topSeparator,SoSeparator,false,this, ,false);
    SO_KIT_ADD_CATALOG_ENTRY(coords,SoCoordinate3,false,
	    		     topSeparator,materials,false);
    SO_KIT_ADD_CATALOG_ENTRY(materials,SoMaterial,false,
	    		     topSeparator,faceset1,false);
    SO_KIT_ADD_CATALOG_ENTRY(faceset1,SoIndexedFaceSet,false,
	    		     topSeparator,faceset2,false);
    SO_KIT_ADD_CATALOG_ENTRY(faceset2,SoIndexedFaceSet,false,
	    		     topSeparator, ,false);

    SO_KIT_ADD_FIELD( wellpath, (0,0,0) );
    SO_KIT_ADD_FIELD( values, (0,0) );
    SO_KIT_ADD_FIELD( maxRadius, (1) );
    SO_KIT_ADD_FIELD( clipRate, (0.05) );

    valuesensor->attach( &maxRadius );
    valuesensor->attach( &values );
}


SoPlaneWellLog::~SoPlaneWellLog()
{
    delete valuesensor;
}


void SoPlaneWellLog::valueChangedCB( void* data, SoSensor* )
{
    SoPlaneWellLog* thisp = reinterpret_cast<SoPlaneWellLog*>( data );
    //TODO: Do sorting, clipping and compute each node's radius
}


void SoPlaneWellLog::GLRender(SoGLRenderAction* action)
{
    SoState* state = action->getState();

    const SbViewVolume& vv = SoViewVolumeElement::get(state);
    const SbMatrix& mat = SoModelMatrixElement::get(state);

    SbVec3f projectiondir = vv.getProjectionDirection();
    projectiondir.normalize(); //TODO: Check in gdb if this is needed

    //TODO go through each node and compute the vector-orientation that fits
    // with the path and is perpendicular with the view-axis
    // Everything has to be computed in worlddomain. Use
    // mat.multVecMatrix(localvec, worldvec);
    // to convert a local-vector to a world-vector
    //
    // Then, set the coordinates
    inherited::GLRender( action );
}
