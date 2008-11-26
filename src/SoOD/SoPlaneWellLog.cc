/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoPlaneWellLog.cc,v 1.13 2008-11-26 16:54:39 cvsbruno Exp $";

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
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/nodes/SoTriangleStripSet.h>

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
    SO_KIT_ADD_CATALOG_ENTRY(trishape1,SoSeparator,false,
	    		     group1,lineshape1,false);
    SO_KIT_ADD_CATALOG_ENTRY(lineshape1,SoSeparator,false,
	    		     group1,"",false);
    SO_KIT_ADD_CATALOG_ENTRY(col1,SoBaseColor,false,
	    		     lineshape1,drawstyle1,false);
    SO_KIT_ADD_CATALOG_ENTRY(drawstyle1,SoDrawStyle,false,
	    		     lineshape1,coords1,false);
    SO_KIT_ADD_CATALOG_ENTRY(coords1,SoCoordinate3,false,
	    		     lineshape1,lineset1,false);
    SO_KIT_ADD_CATALOG_ENTRY(lineset1,SoLineSet,false,
	    		     lineshape1, "",false);
    SO_KIT_ADD_CATALOG_ENTRY(material1,SoMaterial,false,
  	    		     trishape1, coordtri1,false);
    SO_KIT_ADD_CATALOG_ENTRY(coordtri1,SoCoordinate3,false,
	    		     trishape1,triset1,false);
    SO_KIT_ADD_CATALOG_ENTRY(triset1,SoTriangleStripSet,false,
	    		     trishape1, "",false);

    SO_KIT_ADD_CATALOG_ENTRY(group2,SoSeparator,false,
	    		     line2Switch, "",false);
    SO_KIT_ADD_CATALOG_ENTRY(trishape2,SoSeparator,false,
	    		     group2,lineshape2,false);
    SO_KIT_ADD_CATALOG_ENTRY(lineshape2,SoSeparator,false,
	    		     group2,"",false);
    SO_KIT_ADD_CATALOG_ENTRY(col2,SoBaseColor,false,
	    		     lineshape2,drawstyle2,false);
    SO_KIT_ADD_CATALOG_ENTRY(drawstyle2,SoDrawStyle,false,
	    		     lineshape2,coords2,false);
    SO_KIT_ADD_CATALOG_ENTRY(coords2,SoCoordinate3,false,
	    		     lineshape2,lineset2,false);
    SO_KIT_ADD_CATALOG_ENTRY(lineset2,SoLineSet,false,
	    		     lineshape2, "",false);
    SO_KIT_ADD_CATALOG_ENTRY(material2,SoMaterial,false,
  	    		     trishape2, coordtri2,false);
    SO_KIT_ADD_CATALOG_ENTRY(coordtri2,SoCoordinate3,false,
	    		     trishape2,triset2,false);
    SO_KIT_ADD_CATALOG_ENTRY(triset2,SoTriangleStripSet,false,
	    		     trishape2, "",false);

    
    
    SO_KIT_INIT_INSTANCE();

    SO_KIT_ADD_FIELD( path1, (0,0,0) );
    SO_KIT_ADD_FIELD( path2, (0,0,0) );
    SO_KIT_ADD_FIELD( log1, (0) );
    SO_KIT_ADD_FIELD( log2, (0) );
    SO_KIT_ADD_FIELD( maxval1, (0) );
    SO_KIT_ADD_FIELD( maxval2, (0) );
    SO_KIT_ADD_FIELD( minval1, (0) );
    SO_KIT_ADD_FIELD( minval2, (0) );
    SO_KIT_ADD_FIELD( shift1, (0) );
    SO_KIT_ADD_FIELD( shift2, (0) );
    SO_KIT_ADD_FIELD( style1, (0) );
    SO_KIT_ADD_FIELD( style2, (0) );
    SO_KIT_ADD_FIELD( filling1, (0) );
    SO_KIT_ADD_FIELD( filling2, (0) );
    SO_KIT_ADD_FIELD( screenWidth, (40) );
    screenWidth.setValue( 40 );

    valuesensor->attach( &log1 );
    valuesensor->attach( &log2 );
    valuesensor->attach( &screenWidth );

    clearLog(1);
    clearLog(2);
    valchanged = true;
    currentres = 1;
    lognr = 0;
}


SoPlaneWellLog::~SoPlaneWellLog()
{
    if (valuesensor) 
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


void SoPlaneWellLog::setLogFillColor( const SbVec3f& col, int lognr )
{
/*
    SoBaseColor* color = SO_GET_ANY_PART( this,
	    lognr==1 ? "col1" : "col2", SoBaseColor );
    color->rgb.setValue( col );*/
}


const SbVec3f& SoPlaneWellLog::logFillColor( int lognr ) const
{
    SoPlaneWellLog* myself = const_cast<SoPlaneWellLog*>(this);
    SoBaseColor* color = SO_GET_ANY_PART( myself,
	    lognr==1 ? "col1" : "col2", SoBaseColor );
    return color->rgb[0];
}


void SoPlaneWellLog::setSeisFillColor( const SbVec3f& col, int lognr )
{
    SoMaterial* material  = SO_GET_ANY_PART( this,
	     lognr==1 ? "material1" : "material2", SoMaterial );
    material->ambientColor.setValue( col );
}


const SbVec3f& SoPlaneWellLog::seisFillColor( int lognr ) const
{
    SoPlaneWellLog* myself = const_cast<SoPlaneWellLog*>(this);
    SoMaterial* material = SO_GET_ANY_PART( myself,
	    lognr==1 ? "material1" : "material2", SoMaterial );
    return material->ambientColor[0];
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
    SoSFFloat& minval = lognr==1 ? minval1 : minval2;
    minval.setValue( 0 ); 
    SoSFFloat& shift = lognr==1 ? shift1 : shift2;
    shift.setValue( 0 ); 
    SoSFBool& style = lognr==1 ? style1 : style2;
    style.setValue( 0 ); 
    SoSFBool& filling = lognr==1 ? filling1 : filling2;
    filling.setValue( 0 ); 
}


void SoPlaneWellLog::setLogValue( int index, const SbVec3f& crd, float val, 
				  int lognr )
{
    SoMFVec3f& path = lognr==1 ? path1 : path2;
    SoMFFloat& log = lognr==1 ? log1 : log2;
    SoSFFloat& maxval = lognr==1 ? maxval1 : maxval2;
    SoSFFloat& minval = lognr==1 ? minval1 : minval2;
    path.set1Value( index, crd );
    log.set1Value( index, val );
    if ( val > maxval.getValue() ) maxval.setValue( val );
    if ( val < minval.getValue() ) minval.setValue( val );
}

#define sMaxNrSamplesRot 250

void SoPlaneWellLog::buildLog( int lognr, const SbVec3f& projdir, int res )
{
    SoCoordinate3* coords = SO_GET_ANY_PART( this,
            lognr==1 ? "coords1" : "coords2", SoCoordinate3 );
    coords->point.deleteValues(0);
    
    SoCoordinate3* coordtri = SO_GET_ANY_PART( this,
	    lognr==1 ? "coordtri1" : "coordtri2", SoCoordinate3 );
    coordtri->point.deleteValues(0);
    
    SoMFVec3f& path = lognr==1 ? path1 : path2;
    SoMFFloat& log = lognr==1 ? log1 : log2;
    SoSFFloat& maxval = lognr==1 ? maxval1 : maxval2;
    SoSFFloat& minval = lognr==1 ? minval1 : minval2;
    SoSFFloat& shift = lognr==1 ? shift1 : shift2;
    SoSFBool& style = lognr==1 ? style1 : style2;
    SoSFBool& filling = lognr==1 ? filling1 : filling2;
    
    bool styleB = style.getValue();
    float minvalF = minval.getValue();
    float maxvalF = maxval.getValue();
    bool fillingB = filling.getValue();
    float shiftprct; 
    shiftprct = (minvalF - maxvalF) * ( shift.getValue() / 100.0 );

    bool& revscale = lognr==1 ? revscale1 : revscale2;
    const int pathsz = path.getNum();
    int nrsamp = pathsz;

    float step = 1;
    if ( !res && nrsamp > sMaxNrSamplesRot )
    {
	step = (float)nrsamp / sMaxNrSamplesRot;
	nrsamp = sMaxNrSamplesRot;
    }

    float meanlogval = ( maxvalF - minvalF ) / 2;

    for ( int idx=0; idx<nrsamp; idx++ )
    {
	int index = int(idx*step+.5);
	
	float logval = log[index];
	if ( revscale ) logval = maxval.getValue() - logval;
	if ( logval < 0 ) logval = 0;
	if ( logval > maxval.getValue() ) logval = maxval.getValue();
        if ( logval < minval.getValue() ) logval = minval.getValue();
    

	SbVec3f shiftcrd;
        shiftcrd *= 0;
	SbVec3f normalshift = getProjCoords( path, index, projdir, 
					     maxval, shiftprct, lognr );
        shiftcrd += normalshift;

	SbVec3f newcrd = path[index];
	SbVec3f normal = getProjCoords( path, index, projdir, 
				        maxval, logval, lognr );
        SbVec3f linecrd = newcrd + normal - shiftcrd; 
        coords->point.set1Value( idx, linecrd );

	normal = getProjCoords( path, index, projdir, 
				maxval, meanlogval, lognr );
	SbVec3f seisfillcrd  = newcrd + normal - shiftcrd;

	fillTriangles( idx, styleB, logval, meanlogval, coordtri,
	        	seisfillcrd, linecrd );

        if ( !styleB && fillingB )
	fillFirstLog( idx, coordtri, newcrd, linecrd );
    }

    const int nrcrds = coords->point.getNum();
    SoLineSet* lineset = SO_GET_ANY_PART( this,
	    lognr==1 ? "lineset1" : "lineset2", SoLineSet );
    lineset->numVertices.setValue( nrcrds );
	
    const int nrcrdtri = coordtri->point.getNum();
    SoTriangleStripSet* triset = SO_GET_ANY_PART( this,
	    lognr==1 ? "triset1" : "triset2", SoTriangleStripSet );
    triset->numVertices.setValue( nrcrdtri );
    currentres = res;
}


void SoPlaneWellLog::fillTriangles( const int idx, const bool styleB, float logval, 
				    float meanlogval, SoCoordinate3* coordtri,							SbVec3f& seisfillcrd, SbVec3f& linecrd )
{
	if ( styleB )
	{
	    if (logval < meanlogval)
	    {
		coordtri->point.set1Value( 2*idx, seisfillcrd );
		coordtri->point.set1Value( 2*idx+1, linecrd );
	    }   
	    else
	    {
		coordtri->point.set1Value( 2*idx, seisfillcrd );
		coordtri->point.set1Value( 2*idx+1, seisfillcrd );
	    }
	}
}


void SoPlaneWellLog::fillFirstLog( const int idx, SoCoordinate3* coordtri,
       			           SbVec3f& newcrd, SbVec3f& linecrd )
{
	coordtri->point.set1Value( 2*idx, newcrd );
	    coordtri->point.set1Value( 2*idx+1, linecrd );
}


SbVec3f SoPlaneWellLog::getProjCoords( const SoMFVec3f& path, const int index,
				       const SbVec3f& projdir, const SoSFFloat&
				       maxval, const float val, int lognr )
{
	SbVec3f pt1, pt2;	
	if ( !index )
	{
	    pt1 = path[index];
	    pt2 = path[index+1];
	}
	else if ( index == path.getNum()-1 )
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
	SbVec3f normalshift = getNormal( pt1, pt2, projdir );
	normal.normalize();
	
	const float scaledmeanval = val * worldwidth / maxval.getValue();
	const float fact = scaledmeanval / normal.length();
	normal *= lognr==1 ?  -fact : fact;
	return ( normal );
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


void SoPlaneWellLog::setShift( float sht, int lognr )
{
    SoSFFloat& shift = lognr==1 ? shift1 : shift2;
    shift.setValue( sht ); 
}


void SoPlaneWellLog::setLogStyle( bool stl, int lognr )
{
    SoSFBool& style = lognr==1 ? style1 : style2;
    style.setValue( stl ); 
}


void SoPlaneWellLog::setLogFill( bool fill, int lognr )
{
    SoSFBool& filling = lognr==1 ? filling1 : filling2;
    filling.setValue( fill ); 
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
