/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoPlaneWellLog.cc,v 1.12 2008-08-25 10:28:56 cvsnanne Exp $";


#include "SoPlaneWellLog.h"
#include "SoCameraInfoElement.h"
#include "SoCameraInfo.h"

#include <Inventor/actions/SoGLRenderAction.h>

#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoLineSet.h>

#include <Inventor/sensors/SoFieldSensor.h>


SO_KIT_SOURCE(SoPlaneWellLog);

void SoPlaneWellLog::initClass()
{
    SO_KIT_INIT_CLASS( SoPlaneWellLog, SoBaseKit, "BaseKit");
    SO_ENABLE( SoGLRenderAction, SoCameraInfoElement );
}


SoPlaneWellLog::SoPlaneWellLog()
    : valuesensor( new SoFieldSensor(SoPlaneWellLog::valueChangedCB,this) )
    , revscale1(false)
    , revscale2(false)
{
    SO_KIT_CONSTRUCTOR(SoPlaneWellLog);

    SO_KIT_ADD_CATALOG_ENTRY(topSeparator,SoSeparator,false,this, "",false);
    SO_KIT_ADD_CATALOG_ENTRY(line1Switch,SoSwitch,false,
	    		     topSeparator,line2Switch,false);
    SO_KIT_ADD_CATALOG_ENTRY(line2Switch,SoSwitch,false,
	    		     topSeparator, "",false);

    SO_KIT_ADD_CATALOG_ENTRY(group1,SoSeparator,false,
	    		     line1Switch, "",false);
    SO_KIT_ADD_CATALOG_ENTRY(col1,SoBaseColor,false,
	    		     group1,drawstyle1,false);
    SO_KIT_ADD_CATALOG_ENTRY(drawstyle1,SoDrawStyle,false,
	    		     group1,coords1,false);
    SO_KIT_ADD_CATALOG_ENTRY(coords1,SoCoordinate3,false,
	    		     group1,lineset1,false);
    SO_KIT_ADD_CATALOG_ENTRY(lineset1,SoLineSet,false,
	    		     group1, "",false);

    SO_KIT_ADD_CATALOG_ENTRY(group2,SoSeparator,false,
	    		     line2Switch, "",false);
    SO_KIT_ADD_CATALOG_ENTRY(col2,SoBaseColor,false,
	    		     group2,drawstyle2,false);
    SO_KIT_ADD_CATALOG_ENTRY(drawstyle2,SoDrawStyle,false,
	    		     group2,coords2,false);
    SO_KIT_ADD_CATALOG_ENTRY(coords2,SoCoordinate3,false,
	    		     group2,lineset2,false);
    SO_KIT_ADD_CATALOG_ENTRY(lineset2,SoLineSet,false,
	    		     group2, "",false);
    SO_KIT_INIT_INSTANCE();

    SO_KIT_ADD_FIELD( path1, (0,0,0) );
    SO_KIT_ADD_FIELD( path2, (0,0,0) );
    SO_KIT_ADD_FIELD( log1, (0) );
    SO_KIT_ADD_FIELD( log2, (0) );
    SO_KIT_ADD_FIELD( maxval1, (0) );
    SO_KIT_ADD_FIELD( maxval2, (0) );
    SO_KIT_ADD_FIELD( screenWidth, (40) );
    screenWidth.setValue( 40 );

    valuesensor->attach( &log1 );
    valuesensor->attach( &log2 );
    valuesensor->attach( &screenWidth );

    clearLog(1);
    clearLog(2);
    valchanged = true;
    currentres = 1;
}


SoPlaneWellLog::~SoPlaneWellLog()
{
    delete valuesensor;
}


void SoPlaneWellLog::setLineColor( const SbVec3f& col, int lognr )
{
    SoBaseColor* color = SO_GET_ANY_PART( this,
	    lognr==1 ? "col1" : "col2", SoBaseColor );
    color->rgb.setValue( col );
}


const SbVec3f& SoPlaneWellLog::lineColor( int lognr ) const
{
    SoPlaneWellLog* myself = const_cast<SoPlaneWellLog*>(this);
    SoBaseColor* color = SO_GET_ANY_PART( myself,
	    lognr==1 ? "col1" : "col2", SoBaseColor );
    return color->rgb[0];
}


void SoPlaneWellLog::setLineWidth( float width, int lognr )
{
    SoDrawStyle* ds = SO_GET_ANY_PART( this,
	    lognr==1 ? "drawstyle1" : "drawstyle2", SoDrawStyle );
    ds->lineWidth.setValue( width );
}


float SoPlaneWellLog::lineWidth( int lognr ) const
{
    SoPlaneWellLog* myself = const_cast<SoPlaneWellLog*>(this);
    SoDrawStyle* ds = SO_GET_ANY_PART( myself,
	    lognr==1 ? "drawstyle1" : "drawstyle2", SoDrawStyle );
    return ds->lineWidth.getValue();
}


void SoPlaneWellLog::showLog( bool yn, int lognr )
{
    SoSwitch* sw = SO_GET_ANY_PART( this,
	    lognr==1 ? "line1Switch" : "line2Switch", SoSwitch );
    sw->whichChild = yn ? SO_SWITCH_ALL : SO_SWITCH_NONE;
    valuesensor->trigger();
}


bool SoPlaneWellLog::logShown( int lognr ) const
{
    SoPlaneWellLog* myself = const_cast<SoPlaneWellLog*>(this);
    SoSwitch* sw = SO_GET_ANY_PART( myself,
	    lognr==1 ? "line1Switch" : "line2Switch", SoSwitch );
    return sw->whichChild.getValue() == SO_SWITCH_ALL;
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

#define sMaxNrSamplesRot 250

void SoPlaneWellLog::buildLog( int lognr, const SbVec3f& projdir, int res )
{
    SoCoordinate3* coords = SO_GET_ANY_PART( this,
	    lognr==1 ? "coords1" : "coords2", SoCoordinate3 );
    coords->point.deleteValues(0);
    SoMFVec3f& path = lognr==1 ? path1 : path2;
    SoMFFloat& log = lognr==1 ? log1 : log2;
    SoSFFloat& maxval = lognr==1 ? maxval1 : maxval2;
    bool& revscale = lognr==1 ? revscale1 : revscale2;

    const int pathsz = path.getNum();
    int nrsamp = pathsz;

    float step = 1;
    if ( !res && nrsamp > sMaxNrSamplesRot )
    {
	step = (float)nrsamp / sMaxNrSamplesRot;
	nrsamp = sMaxNrSamplesRot;
    }

    for ( int idx=0; idx<nrsamp; idx++ )
    {
	int index = int(idx*step+.5);
	SbVec3f pt1, pt2;
	if ( !index )
	{
	    pt1 = path[index];
	    pt2 = path[index+1];
	}
	else if ( index == pathsz-1 )
	{
	    pt1 = path[index-1];
	    pt2 = path[index];
	}
	else
	{
	    pt1 = path[index-1];
	    pt2 = path[index+1];
	}

	SbVec3f normal = getNormal( pt1, pt2, projdir );
	normal.normalize();

	float logval = log[index];
	if ( revscale ) logval = maxval.getValue() - logval;
	if ( logval < 0 ) logval = 0;
	if ( logval > maxval.getValue() ) logval = maxval.getValue();
	const float scaledval = logval * worldwidth / maxval.getValue();
	const float fact = scaledval / normal.length();
	normal *= lognr==1 ? -fact : fact;
	SbVec3f newcrd = path[index];
	newcrd += normal;
	coords->point.set1Value( idx, newcrd );
    }

    const int nrcrds = coords->point.getNum();
    SoLineSet* lineset = SO_GET_ANY_PART( this,
	    lognr==1 ? "lineset1" : "lineset2", SoLineSet );
    lineset->numVertices.setValue( nrcrds );
    currentres = res;
}


SbVec3f SoPlaneWellLog::getNormal( const SbVec3f& pt1, const SbVec3f& pt2, 
				   const SbVec3f& projdir )
{
    SbVec3f diff = pt2;
    diff -= pt1; diff.setValue( 0, 0, -1 ); //TODO:
    return diff.cross( projdir );
}


void SoPlaneWellLog::valueChangedCB( void* data, SoSensor* )
{
    SoPlaneWellLog* thisp = reinterpret_cast<SoPlaneWellLog*>( data );
    thisp->valchanged = true;
}


bool SoPlaneWellLog::shouldGLRender( int newres )
{
    if ( !path1.getNum() && !path2.getNum() ) return false;

    bool dorender = !newres || !(newres==currentres);
    return ( valchanged || dorender );
}


int SoPlaneWellLog::getResolution( SoState* state )
{
    int32_t camerainfo = SoCameraInfoElement::get(state);
    bool ismov = camerainfo&(SoCameraInfo::MOVING|SoCameraInfo::INTERACTIVE);
    return ismov ? 0 : 1; 
}


void SoPlaneWellLog::GLRender( SoGLRenderAction* action )
{
    SoState* state = action->getState();
    state->push();

    int newres = getResolution( state );
    if ( shouldGLRender(newres) )
    {
	const SbViewVolume& vv = SoViewVolumeElement::get(state);
	const SbViewportRegion& vp = SoViewportRegionElement::get(state);
	float nsize = screenWidth.getValue() / 
	    				float(vp.getViewportSizePixels()[1]);

	SoSwitch* sw = SO_GET_ANY_PART( this, "line1Switch", SoSwitch );
	SbVec3f projectiondir = vv.getProjectionDirection();
	if ( path1.getNum()>0 && sw->whichChild.getValue()==SO_SWITCH_ALL )
	{
	    const int hnum = path1.getNum() / 2;
	    worldwidth = vv.getWorldToScreenScale( path1[hnum], nsize );
	    buildLog( 1, projectiondir, newres );
	}

	sw = SO_GET_ANY_PART( this, "line2Switch", SoSwitch );
	if ( path2.getNum()>0 && sw->whichChild.getValue()==SO_SWITCH_ALL )
	{
	    const int hnum = path2.getNum() / 2;
	    worldwidth = vv.getWorldToScreenScale( path2[hnum], nsize );
	    buildLog( 2, projectiondir, newres );
	}

	valchanged = false;
    }

    state->pop();
    inherited::GLRender( action );
}
