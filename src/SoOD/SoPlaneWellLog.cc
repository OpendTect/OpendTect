/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoPlaneWellLog.cc,v 1.2 2003-10-17 14:58:39 nanne Exp $";


#include "SoPlaneWellLog.h"

#include <Inventor/actions/SoGLRenderAction.h>

#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoLineSet.h>

#include <Inventor/sensors/SoFieldSensor.h>


SO_KIT_SOURCE(SoPlaneWellLog);

void SoPlaneWellLog::initClass()
{
    SO_KIT_INIT_CLASS( SoPlaneWellLog, SoBaseKit, "BaseKit");
}


SoPlaneWellLog::SoPlaneWellLog()
    : valuesensor( new SoFieldSensor(SoPlaneWellLog::valueChangedCB,this) )
{
    SO_KIT_CONSTRUCTOR(SoPlaneWellLog);

    SO_KIT_ADD_CATALOG_ENTRY(topSeparator,SoSeparator,false,this, ,false);
    SO_KIT_ADD_CATALOG_ENTRY(line1Switch,SoSwitch,false,
	    		     topSeparator,line2Switch,false);
    SO_KIT_ADD_CATALOG_ENTRY(line2Switch,SoSwitch,false,
	    		     topSeparator, ,false);

    SO_KIT_ADD_CATALOG_ENTRY(group1,SoSeparator,false,
	    		     line1Switch,group2,false);
    SO_KIT_ADD_CATALOG_ENTRY(col1,SoBaseColor,false,
	    		     group1,coords1,false);
    SO_KIT_ADD_CATALOG_ENTRY(coords1,SoCoordinate3,false,
	    		     group1,lineset1,false);
    SO_KIT_ADD_CATALOG_ENTRY(lineset1,SoLineSet,false,
	    		     group1, ,false);

    SO_KIT_ADD_CATALOG_ENTRY(group2,SoSeparator,false,
	    		     line2Switch, ,false);
    SO_KIT_ADD_CATALOG_ENTRY(col2,SoBaseColor,false,
	    		     group2,coords2,false);
    SO_KIT_ADD_CATALOG_ENTRY(coords2,SoCoordinate3,false,
	    		     group2,lineset2,false);
    SO_KIT_ADD_CATALOG_ENTRY(lineset2,SoLineSet,false,
	    		     group2, ,false);
    SO_KIT_INIT_INSTANCE();

    sw1ptr = (SoSwitch*)getAnyPart("line1Switch",true);
    sw2ptr = (SoSwitch*)getAnyPart("line2Switch",true);
    col1ptr = (SoBaseColor*)getAnyPart("col1",true);
    col2ptr = (SoBaseColor*)getAnyPart("col2",true);
    coord1ptr = (SoCoordinate3*)getAnyPart("coords1",true);
    coord2ptr = (SoCoordinate3*)getAnyPart("coords2",true);
    line1ptr = (SoLineSet*)getAnyPart("lineset1",true);
    line2ptr = (SoLineSet*)getAnyPart("lineset2",true);

    sw1ptr->whichChild = -3;
    sw2ptr->whichChild = -3;

    SO_KIT_ADD_FIELD( path1, (0,0,0) );
    SO_KIT_ADD_FIELD( path2, (0,0,0) );
    SO_KIT_ADD_FIELD( log1, (0) );
    SO_KIT_ADD_FIELD( log2, (0) );
    SO_KIT_ADD_FIELD( maxval1, (0) );
    SO_KIT_ADD_FIELD( maxval2, (0) );
    SO_KIT_ADD_FIELD( width, (400) );

    valuesensor->attach( &log1 );
    valuesensor->attach( &log2 );
    valuesensor->attach( &width );

    clearLog(1);
    clearLog(2);
}


SoPlaneWellLog::~SoPlaneWellLog()
{
    delete valuesensor;
}


void SoPlaneWellLog::setWidth( float wdth_ )
{
    width.setValue( wdth_ );
}


void SoPlaneWellLog::setLineColor( const SbVec3f& col, int lognr )
{
    SoBaseColor* color = lognr==1 ? col1ptr : col2ptr;
    color->rgb.setValue( col );
}


const SbVec3f& SoPlaneWellLog::lineColor( int lognr ) const
{
    SoBaseColor* color = lognr==1 ? col1ptr : col2ptr;
    return color->rgb[0];
}


void SoPlaneWellLog::showLog( bool yn, int lognr )
{
    SoSwitch* sw = lognr==1 ? sw1ptr : sw2ptr;
    sw->whichChild = yn ? -3 : -1;
}


bool SoPlaneWellLog::logShown( int lognr ) const
{
    SoSwitch* sw = lognr==1 ? sw1ptr : sw2ptr;
    return sw->whichChild.getValue() == -3;
}


void SoPlaneWellLog::clearLog( int lognr )
{
    SoMFVec3f& path = lognr==1 ? path1 : path2;
    path.deleteValues(0);
    SoMFFloat& log = lognr==1 ? log1 : log2;
    log.deleteValues(0);
    SoSFFloat& maxval = lognr==1 ? maxval1 : maxval2;
    maxval.setValue( 0 );
}


void SoPlaneWellLog::setLogValue( int index, const SbVec3f& crd, float val, 
				  int lognr )
{
    SoMFVec3f& path = lognr==1 ? path1 : path2;
    SoMFFloat& log = lognr==1 ? log1 : log2;
    SoSFFloat& maxval = lognr==1 ? maxval1 : maxval2;
    path.set1Value( index, crd );
    log.set1Value( index, val );
    if ( val > maxval.getValue() ) maxval.setValue( val );
}


void SoPlaneWellLog::buildLog( int lognr, const SbVec3f& projdir )
{
    SoCoordinate3* coords = lognr==1 ? coord1ptr : coord2ptr;
    coords->point.deleteValues(0);
    SoMFVec3f& path = lognr==1 ? path1 : path2;
    SoMFFloat& log = lognr==1 ? log1 : log2;
    SoSFFloat& maxval = lognr==1 ? maxval1 : maxval2;

    const int pathsz = path.getNum();
    for ( int idx=0; idx<pathsz; idx++ )
    {
	SbVec3f pt1, pt2;
	if ( !idx )
	{
	    pt1 = path[idx];
	    pt2 = path[idx+1];
	}
	else if ( idx == pathsz-1 )
	{
	    pt1 = path[idx-1];
	    pt2 = path[idx];
	}
	else
	{
	    pt1 = path[idx-1];
	    pt2 = path[idx+1];
	}

	SbVec3f normal = getNormal( pt1, pt2, projdir );
	normal.normalize();

	const float scaledval = log[idx]*width.getValue()/maxval.getValue();
	const float fact = scaledval / normal.length();
	normal *= lognr==1 ? -fact : fact;
	SbVec3f newcrd = path[idx];
	newcrd += normal;
	coords->point.set1Value( idx, newcrd );
    }

    lognr==1 ? line1ptr->numVertices.setValue( pathsz )
	     : line2ptr->numVertices.setValue( pathsz );
}


SbVec3f SoPlaneWellLog::getNormal( const SbVec3f& pt1, const SbVec3f& pt2, 
				   const SbVec3f& projdir )
{
    SbVec3f diff = pt2;
    diff -= pt1;
    return diff.cross( projdir );
}


void SoPlaneWellLog::valueChangedCB( void* data, SoSensor* )
{
    SoPlaneWellLog* thisp = reinterpret_cast<SoPlaneWellLog*>( data );
    //TODO: Do sorting, clipping and compute each node's radius
}


void SoPlaneWellLog::GLRender( SoGLRenderAction* action )
{
    SoState* state = action->getState();

    const SbViewVolume& vv = SoViewVolumeElement::get(state);
    const SbMatrix& mat = SoModelMatrixElement::get(state);

    SbVec3f projectiondir = vv.getProjectionDirection();
    if ( sw1ptr->whichChild.getValue() == -3 )
	buildLog( 1, projectiondir );
    if ( sw2ptr->whichChild.getValue() == -3 )
	buildLog( 2, projectiondir );
    //TODO Did not use mat, check!
    // Use mat.multVecMatrix(localvec, worldvec);
    // to convert a local-vector to a world-vector
 
    inherited::GLRender( action );
}
