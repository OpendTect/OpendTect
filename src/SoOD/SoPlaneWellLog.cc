/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: SoPlaneWellLog.cc,v 1.56 2012-08-10 03:50:04 cvsaneesh Exp $";

#include "SoPlaneWellLog.h"
#include "SoCameraInfoElement.h"
#include "SoCameraInfo.h"


#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/sensors/SoTimerSensor.h>

#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/nodes/SoTriangleStripSet.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>

SO_KIT_SOURCE(SoPlaneWellLog);

void SoPlaneWellLog::initClass()
{
    SO_KIT_INIT_CLASS( SoPlaneWellLog, SoBaseKit, "BaseKit");
    SO_ENABLE( SoGLRenderAction, SoCameraInfoElement );
}


SoPlaneWellLog::SoPlaneWellLog()
    : timesensor( new SoTimerSensor() )
    , revscale1(false)
    , revscale2(false)
    , fillrevscale1(false)
    , fillrevscale2(false)
    , seisstyle1(false)
    , seisstyle2(false)
    , isfilled1(true)
    , isfilled2(true)
    , screensize(0,0)	     
    , time(0.0)		     
    , resizewhenzooming(false)
    , constantsizefactor(1)  
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
    SO_KIT_ADD_CATALOG_ENTRY(hints1,SoShapeHints,false,
			     trishape1, material1,false);
    SO_KIT_ADD_CATALOG_ENTRY(material1,SoMaterial,false,
			     trishape1, mbinding1,false);
    SO_KIT_ADD_CATALOG_ENTRY(mbinding1,SoMaterialBinding,false,
			     trishape1, coordtri1,false);
    SO_KIT_ADD_CATALOG_ENTRY(coordtri1,SoCoordinate3,false,
 			     trishape1,triset1,false);
    SO_KIT_ADD_CATALOG_ENTRY(triset1,SoIndexedTriangleStripSet,false,
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
    SO_KIT_ADD_CATALOG_ENTRY(hints2,SoShapeHints,false,
			     trishape2, material2,false);
    SO_KIT_ADD_CATALOG_ENTRY(material2,SoMaterial,false,
			     trishape2, mbinding2,false);
    SO_KIT_ADD_CATALOG_ENTRY(mbinding2,SoMaterialBinding,false,
			     trishape2, coordtri2,false);
    SO_KIT_ADD_CATALOG_ENTRY(coordtri2,SoCoordinate3,false,
			     trishape2,triset2,false);
    SO_KIT_ADD_CATALOG_ENTRY(triset2,SoIndexedTriangleStripSet,false,
			     trishape2, "",false);


    SO_KIT_INIT_INSTANCE();

    SO_KIT_ADD_FIELD( path1, (0,0,0) );
    SO_KIT_ADD_FIELD( path2, (0,0,0) );
    SO_KIT_ADD_FIELD( log1, (0) );
    SO_KIT_ADD_FIELD( log2, (0) );
    SO_KIT_ADD_FIELD( filllog1, (0) );
    SO_KIT_ADD_FIELD( filllog2, (0) );
    SO_KIT_ADD_FIELD( maxval1, (0) );
    SO_KIT_ADD_FIELD( maxval2, (0) );
    SO_KIT_ADD_FIELD( fillmaxval1, (0) );
    SO_KIT_ADD_FIELD( fillmaxval2, (0) );
    SO_KIT_ADD_FIELD( minval1, (100) );
    SO_KIT_ADD_FIELD( minval2, (100) );
    SO_KIT_ADD_FIELD( fillminval1, (100) );
    SO_KIT_ADD_FIELD( fillminval2, (100) );
    SO_KIT_ADD_FIELD( shift1, (0) );
    SO_KIT_ADD_FIELD( shift2, (0) );
    SO_KIT_ADD_FIELD( screenWidth1, (40) );
    SO_KIT_ADD_FIELD( screenWidth2, (40) );

    valuesensor = new SoFieldSensor(SoPlaneWellLog::valueChangedCB,this);
    valuesensor->attach( &log1 );
    valuesensor->attach( &log2 );
    valuesensor->attach( &screenWidth1 );
    valuesensor->attach( &screenWidth2 );

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
    if (timesensor) 
	delete timesensor;
}


void SoPlaneWellLog::resetLogData( int lnr )
{
    SoSFFloat& maxval = lnr==1 ? maxval1 : maxval2;
    maxval.setValue( 0 ); 
    SoSFFloat& minval = lnr==1 ? minval1 : minval2;
    minval.setValue( 100 ); 
    SoSFFloat& fillmaxval = lnr==1 ? fillmaxval1 : fillmaxval2;
    fillmaxval.setValue( 0 ); 
    SoSFFloat& fillminval = lnr==1 ? fillminval1 : fillminval2;
    fillminval.setValue( 100 ); 
}


void SoPlaneWellLog::setLineColor( const SbVec3f& col, int lnr )
{
    SoBaseColor* color = SO_GET_ANY_PART( this,
	    lnr==1 ? "col1" : "col2", SoBaseColor );
    color->rgb.setValue( col );
}


const SbVec3f& SoPlaneWellLog::lineColor( int lnr ) const
{
    SoPlaneWellLog* myself = const_cast<SoPlaneWellLog*>(this);
    SoBaseColor* color = SO_GET_ANY_PART( myself,
	    lnr==1 ? "col1" : "col2", SoBaseColor );
    return color->rgb[0];
}


void SoPlaneWellLog::setFilledLogColorTab( const float colors[][3], int lnr )
{
    SoShapeHints* hints  = SO_GET_ANY_PART( this,
	     lnr==1 ? "hints1" : "hints2", SoShapeHints );
    hints->vertexOrdering.setValue( SoShapeHints::COUNTERCLOCKWISE );

    SoMaterial* material  = SO_GET_ANY_PART( this,
	     lnr==1 ? "material1" : "material2", SoMaterial );
    material->diffuseColor.setValues(0, 256, colors );

    SoMaterialBinding* mbinding  = SO_GET_ANY_PART( this,
	     lnr==1 ? "mbinding1" : "mbinding2", SoMaterialBinding );
    mbinding->value.setValue(SoMaterialBindingElement::PER_VERTEX_INDEXED);
}


void SoPlaneWellLog::setLineWidth( float width, int lnr )
{
    SoDrawStyle* ds = SO_GET_ANY_PART( this,
	    lnr==1 ? "drawstyle1" : "drawstyle2", SoDrawStyle );
    ds->lineWidth.setValue( width );
}


float SoPlaneWellLog::lineWidth( int lnr ) const
{
    SoPlaneWellLog* myself = const_cast<SoPlaneWellLog*>(this);
    SoDrawStyle* ds = SO_GET_ANY_PART( myself,
	    lnr==1 ? "drawstyle1" : "drawstyle2", SoDrawStyle );
    return ds->lineWidth.getValue();
}


void SoPlaneWellLog::showLog( bool yn, int lnr )
{
    SoSwitch* sw = SO_GET_ANY_PART( this,
	    lnr==1 ? "line1Switch" : "line2Switch", SoSwitch );
    sw->whichChild = yn ? SO_SWITCH_ALL : SO_SWITCH_NONE;
    valuesensor->trigger();
}


bool SoPlaneWellLog::logShown( int lnr ) const
{
    SoPlaneWellLog* myself = const_cast<SoPlaneWellLog*>(this);
    SoSwitch* sw = SO_GET_ANY_PART( myself,
	    lnr==1 ? "line1Switch" : "line2Switch", SoSwitch );
    return sw->whichChild.getValue() == SO_SWITCH_ALL;
}


void SoPlaneWellLog::clearLog( int lnr )
{
    SoMFVec3f& path = lnr==1 ? path1 : path2;
    path.deleteValues(0);
    SoMFFloat& log = lnr==1 ? log1 : log2;
    log.deleteValues(0);
    SoMFFloat& filllog = lnr==1 ? filllog1 : filllog2;
    filllog.deleteValues(0);
    SoSFFloat& fillmaxval = lnr==1 ? fillmaxval1 : fillmaxval2;
    fillmaxval.setValue( 0 ); 
    SoSFFloat& fillminval = lnr==1 ? fillminval1 : fillminval2;
    fillminval.setValue( 0 ); 
    SoSFFloat& maxval = lnr==1 ? maxval1 : maxval2;
    maxval.setValue( 0 ); 
    SoSFFloat& minval = lnr==1 ? minval1 : minval2;
    minval.setValue( 0 ); 
    SoSFFloat& shift = lnr==1 ? shift1 : shift2;
    shift.setValue( 0 ); 
}


void SoPlaneWellLog::setLogValue( int index, const SbVec3f& crd, float val, 
				  int lnr )
{
    SoMFVec3f& path = lnr==1 ? path1 : path2;
    SoMFFloat& log = lnr==1 ? log1 : log2;
    SoSFFloat& maxval = lnr==1 ? maxval1 : maxval2;
    SoSFFloat& minval = lnr==1 ? minval1 : minval2;
    path.set1Value( index, crd );
    log.set1Value( index, val );
    if ( val <= 100 )
    {
	if ( val > maxval.getValue() ) maxval.setValue( val );
	if ( val < minval.getValue() ) minval.setValue( val );
    }
}


void SoPlaneWellLog::setFillLogValue( int index, float fillval, int lnr )
{
    SoMFFloat& filllog    = lnr==1 ? filllog1 : filllog2;

    filllog.set1Value( index, fillval );
}


void SoPlaneWellLog::setFillExtrValue( float maxval, float minval, int lnr )
{
    SoSFFloat& fillmaxval = lnr==1 ? fillmaxval1 : fillmaxval2;
    SoSFFloat& fillminval = lnr==1 ? fillminval1 : fillminval2;
    fillmaxval.setValue( maxval );
    fillminval.setValue( minval );
}


void SoPlaneWellLog::buildLog(int lnr, const SbVec3f& projdir, int res )
{
    SoCoordinate3* coords = SO_GET_ANY_PART( this,
             lnr==1 ? "coords1" : "coords2", SoCoordinate3 );
    coords->point.deleteValues(0);
    
    SoCoordinate3* coordtri = SO_GET_ANY_PART( this,
	    lnr==1 ? "coordtri1" : "coordtri2", SoCoordinate3 );
    coordtri->point.deleteValues(0);
   
    SoIndexedTriangleStripSet* triset = SO_GET_ANY_PART( this,
	    lnr==1 ? "triset1" : "triset2", SoIndexedTriangleStripSet );
    triset->coordIndex.deleteValues(0,-1);
    triset->materialIndex.deleteValues(0,-1);
	
    const bool isfilled = lnr==1 ? isfilled1 : isfilled2;
    const bool seisstyle = lnr==1 ? seisstyle1 : seisstyle2;
    const bool islinedisplayed = lnr==1 ? islinedisp1 : islinedisp2;

    if ( !seisstyle )
    {
	if ( islinedisplayed )
	    buildSimpleLog( lnr, projdir, res );
	if ( isfilled )
	    buildFilledLog( lnr, projdir, res );
    }
    else
	buildSeismicLog( lnr, projdir, res );
}


#define sMaxNrSamplesRot 15
void SoPlaneWellLog::buildSimpleLog(int lnr, const SbVec3f& projdir, int res) 
{
    SoLineSet* lineset = SO_GET_ANY_PART( this,
	     lnr==1 ? "lineset1":"lineset2" , SoLineSet );

    SoCoordinate3* coords = SO_GET_ANY_PART( this,
             lnr==1 ? "coords1" : "coords2", SoCoordinate3 );
    
    SoMFVec3f& path   = lnr==1 ? path1   : path2;
    SoMFFloat& log    = lnr==1 ? log1    : log2;
    SoSFFloat& maxval = lnr==1 ? maxval1 : maxval2;

    if ( log.getNum() == 0 )
	return;
    
    bool revscale = lnr==1 ? revscale1 : revscale2;
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
	float logval = log[index];
	if ( logval > 100 )
	    logval = 0;
	
	if ( revscale ) logval = maxval.getValue() - logval;
	
	SbVec3f newcrd = path[index];
	SbVec3f normal = getProjCoords( path, index, projdir, 
				        maxval, logval, lnr );
        SbVec3f linecrd = newcrd + normal; 
	coords->point.set1Value( idx, linecrd );
	const int nrcrds = coords->point.getNum();
	lineset->numVertices.setValue( nrcrds );
    }
    currentres = res;
}


void SoPlaneWellLog::buildSeismicLog(int lnr, const SbVec3f& projdir, int res)
{
    SoCoordinate3* coords = SO_GET_ANY_PART( this,
             lnr==1 ? "coords1" : "coords2", SoCoordinate3 );
   
    SoCoordinate3* coordtri = SO_GET_ANY_PART( this,
	    lnr==1 ? "coordtri1" : "coordtri2", SoCoordinate3 );
	
    SoLineSet* lineset = SO_GET_ANY_PART( this,
	     lnr==1 ? "lineset1":"lineset2" , SoLineSet );
    
    SoIndexedTriangleStripSet* triset = SO_GET_ANY_PART( this,
	    lnr==1 ? "triset1" : "triset2", SoIndexedTriangleStripSet );

    SoMFVec3f& path   = lnr==1 ? path1   : path2;
    SoMFFloat& log    = lnr==1 ? log1    : log2;
    SoSFFloat& maxval = lnr==1 ? maxval1 : maxval2;
    SoSFFloat& minval = lnr==1 ? minval1 : minval2;
    SoSFFloat& shift  = lnr==1 ? shift1  : shift2;

    if ( log.getNum() == 0 )
	return;

    float minvalF = minval.getValue();
    float maxvalF = maxval.getValue();
    float meanvalF = 0;
    float shiftprct = (minvalF - maxvalF) * ( shift.getValue() / 100.0f );
    float meanlogval = ( maxvalF - minvalF ) / 2;

    const int pathsz = path.getNum();
    int nrsamp = pathsz;
    float step = 1;
    float prevval = 0;

    if ( !res && nrsamp > sMaxNrSamplesRot )
    {
	step = (float)nrsamp / sMaxNrSamplesRot;
	nrsamp = sMaxNrSamplesRot;
    }

    int* coloridx = new int [2*nrsamp+1];  
    int* indices = new int [2*nrsamp+1];
    
    for ( int idx=0; idx<nrsamp; idx++ )
    {
	int index = int(idx*step+.5);
	float logval = log[index];
	if ( lnr == 1 ) logval = maxval.getValue() - logval;
	meanvalF += logval/nrsamp;
    }
    meanlogval = meanvalF;

    for ( int idx=0; idx<2*nrsamp+1; idx++ )
    {
	indices[idx] = idx;
	coloridx[idx] = 1;
    }

    for ( int idx=0; idx<nrsamp; idx++ )
    {
	int index = int(idx*step+.5);
	float logval = log[index];
	if ( lnr == 1 )   logval = maxval.getValue() - logval;
	if ( logval < 0 )   logval = prevval;
	if ( logval > 100 ) logval = prevval;
	
	SbVec3f shiftcrd; 
	shiftcrd *= 0;
	SbVec3f normalshift = getProjCoords( path, index, projdir, 
					     maxval, shiftprct, lnr );
        shiftcrd += normalshift;
	SbVec3f newcrd = path[index];
	SbVec3f normal = getProjCoords( path, index, projdir, 
				    maxval, logval, lnr );
        SbVec3f linecrd = newcrd + normal - shiftcrd; 
	coords->point.set1Value( idx, linecrd );

	normal = getProjCoords( path, index, projdir, 
				maxval, meanlogval, lnr );
	SbVec3f seisfillcrd  = newcrd + normal - shiftcrd;
	
	coordtri->point.set1Value( 2*idx, seisfillcrd  );
	if ( lnr == 1 )
	{
	    if ( logval < meanlogval )
		coordtri->point.set1Value( 2*idx+1, linecrd );
	    else
		coordtri->point.set1Value( 2*idx+1, seisfillcrd );
	}
	else if ( lnr == 2 )
	{
	    if ( logval > meanlogval )
		coordtri->point.set1Value( 2*idx+1, linecrd );
	    else
		coordtri->point.set1Value( 2*idx+1, seisfillcrd );
	}
	const int nrcrds = coords->point.getNum();
	lineset->numVertices.setValue( nrcrds );
	prevval = logval;
    }
    triset->coordIndex.setValues( 0, 2*nrsamp, indices );
    triset->materialIndex.setValues( 0, 2*nrsamp, coloridx );
    currentres = res;

    delete [] coloridx;
    delete [] indices;
}


void SoPlaneWellLog::buildFilledLog(int lnr, const SbVec3f& projdir, int res)
{
    SoCoordinate3* coordtri = SO_GET_ANY_PART( this,
	    lnr==1 ? "coordtri1" : "coordtri2", SoCoordinate3 );
   
    SoIndexedTriangleStripSet* triset = SO_GET_ANY_PART( this,
	    lnr==1 ? "triset1" : "triset2", SoIndexedTriangleStripSet );

    SoMFVec3f& path = lnr==1 ? path1 : path2;
    SoMFFloat& log = lnr==1 ? log1 : log2;
    SoMFFloat& filllog = lnr==1 ? filllog1 : filllog2;
    SoSFFloat& maxval = lnr==1 ? maxval1 : maxval2;
    SoSFFloat& fillmaxval = lnr==1 ? fillmaxval1 : fillmaxval2;
    SoSFFloat& fillminval = lnr==1 ? fillminval1 : fillminval2;

    if ( filllog.getNum() == 0 )
	return;

    float maxvalF = maxval.getValue();
    float fillminvalF = fillminval.getValue();
    float fillmaxvalF = fillmaxval.getValue();
    float colstep = ( fillmaxvalF - fillminvalF ) / 255;
    int   colindex = 0;

    bool fillrevscale = lnr==1 ? fillrevscale1 : fillrevscale2;
    bool revscale = lnr==1 ? revscale1 : revscale2;
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
	float filllogval = filllog[index];
	float logval = log[index];
	colindex = (int)((filllogval-fillminvalF)/colstep);
	if ( revscale ) 
	    logval = maxval.getValue() - logval;
	if ( colindex > 255 ) colindex = 255;
	if ( colindex < 0 ) colindex = 0;
	if ( logval <= 100 && filllogval <= 100 && logval>=0 && filllogval>=0 )
	{
	    triset->materialIndex.set1Value( 2*idx, colindex );
	    triset->materialIndex.set1Value( 2*idx+1, colindex );
	}
	else
	{
	    logval = 0; filllogval = 0;
	    triset->materialIndex.set1Value( 2*idx, 0 );
	    triset->materialIndex.set1Value( 2*idx+1, 0 );
	}
	SbVec3f pathcrd = path[index];
	SbVec3f newcrd = fillrevscale ? pathcrd + getProjCoords( path, index, 
					    projdir, maxval, maxvalF, lnr )
				  : pathcrd;
	SbVec3f normal = getProjCoords( path, index, projdir, 
					maxval, logval, lnr );
        SbVec3f linecrd = pathcrd + normal; 
	
	coordtri->point.set1Value( 2*idx, newcrd );
	coordtri->point.set1Value( 2*idx+1, linecrd );
	triset->coordIndex.set1Value( 2*idx, 2*idx );
	triset->coordIndex.set1Value( 2*idx+1, 2*idx+1 );
    }
    
    currentres = res;
}


SbVec3f SoPlaneWellLog::getProjCoords( const SoMFVec3f& path, const int index,
				       const SbVec3f& projdir, const SoSFFloat&
				       maxval, const float val, int lnr )
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
    normal.normalize();
    
    float maxvalF = maxval.getValue();
    const float scaledmeanval = val * worldwidth / maxvalF;
    const float fact = (maxvalF == 0) ? 0 : scaledmeanval / normal.length();
    normal *= lnr==1 ?  -fact : fact;
    return ( normal );
} 


SbVec3f SoPlaneWellLog::getNormal( const SbVec3f& pt1, const SbVec3f& pt2, 
				   const SbVec3f& projdir )
{
    SbVec3f res = SbVec3f(0,0,-1).cross( projdir );
    //Followed old code, pt1,pt2 are not really used.

    if ( res.length()<1e-6 )
	res = SbVec3f( 1, 0, 0 ); 

    return res;
}


void SoPlaneWellLog::valueChangedCB( void* data, SoSensor* )
{
    SoPlaneWellLog* thisp = reinterpret_cast<SoPlaneWellLog*>( data );
    thisp->valchanged = true;
}


void SoPlaneWellLog::setShift( float sht, int lnr )
{
    SoSFFloat& shift = lnr==1 ? shift1 : shift2;
    shift.setValue( sht ); 
}


void SoPlaneWellLog::setLogStyle( bool stl, int lnr )
{
    bool& style = lnr==1 ? seisstyle1 : seisstyle2;
    style = stl; 
}


void SoPlaneWellLog::setLineDisplayed( bool isdisp, int lnr )
{
    bool& islinedisplayed = lnr==1 ? islinedisp1 : islinedisp2;
    islinedisplayed = isdisp; 
}


bool SoPlaneWellLog::lineDisp( int lnr ) const
{
    const bool isdisp = lnr==1 ? islinedisp1 : islinedisp2;
    return isdisp;
}


void SoPlaneWellLog::setLogFill( bool isfill, int lnr )
{
    bool& isfilled = lnr==1 ? isfilled1 : isfilled2;
    isfilled = isfill; 
}


void SoPlaneWellLog::setLogConstantSize( bool rsz )
{
    resizewhenzooming = !rsz; 
}


bool SoPlaneWellLog::logConstantSize() const
{
    return !resizewhenzooming; 
}


void SoPlaneWellLog::setLogConstantSizeFactor( float fac )
{
    constantsizefactor = fac;
}


float SoPlaneWellLog::logConstantSizeFactor() const
{
    return constantsizefactor;
}


bool SoPlaneWellLog::shouldGLRender( int newres )
{
    if ( !path1.getNum() && !path2.getNum() ) return false;

    bool dorender = !newres || !(newres==currentres);
    return ( valchanged || dorender );
}


#define mReactionTime 0.3 
bool SoPlaneWellLog::isZooming( SoState* state )
{
    SbVec2s screensz;		
    SbBox3f bbox; bbox.setBounds( SbVec3f(-1,-1,-1), SbVec3f(1,1,1) );
    SoShape::getScreenSize( state, bbox, screensz );
    SbTime curtime = SbTime::getTimeOfDay();
    if ( screensz != screensize )
    { 
	screensize = screensz; 
	time = curtime;
	return true; 
    }
    else if ( ( curtime-time ) < mReactionTime  )
	return true;

    return false;
}


int SoPlaneWellLog::getResolution( SoState* state )
{
    int32_t camerainfo = SoCameraInfoElement::get(state);
    bool ismov = camerainfo&(SoCameraInfo::MOVING|SoCameraInfo::INTERACTIVE);

    if ( resizewhenzooming && isZooming(state) )
	ismov = true;
    
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
	SbVec3f projectiondir = vv.getProjectionDirection();
	float szpixel = float( vp.getViewportSizePixels()[1] );

	SoSwitch* sw = SO_GET_ANY_PART( this, "line1Switch", SoSwitch );
	if ( path1.getNum()>0 && sw->whichChild.getValue()==SO_SWITCH_ALL )
	{
	    const int hnum = path1.getNum() / 2;
	    float nsize1 = screenWidth1.getValue() / szpixel; 
	    worldwidth = vv.getWorldToScreenScale( path1[hnum], nsize1 );
	    if ( !resizewhenzooming ) worldwidth = nsize1*constantsizefactor;
	    buildLog( 1, projectiondir, newres );
	}

	sw = SO_GET_ANY_PART( this, "line2Switch", SoSwitch );
	if ( path2.getNum()>0 && sw->whichChild.getValue()==SO_SWITCH_ALL )
	{
	    const int hnum = path2.getNum() / 2;
	    float nsize2 = screenWidth2.getValue() / szpixel; 
	    worldwidth = vv.getWorldToScreenScale( path2[hnum], nsize2 );
	    if ( !resizewhenzooming ) worldwidth = nsize2*constantsizefactor;
	    buildLog( 2, projectiondir, newres );
	}
	valchanged = false;
    }

    state->pop();
    inherited::GLRender( action );
}
