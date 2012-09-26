/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2009
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "SoAxes.h"

#include <Inventor/system/gl.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>



static GLubyte nbmp[] = { 0xE3, 0x46, 0x46, 0x4A, 0x52, 0x62, 0x62, 0xC7 };
static GLubyte ebmp[] = { 0xfe, 0x80, 0x80, 0xF0, 0xF0, 0x80, 0x80, 0xfe };
static GLubyte zbmp[] = { 0xff, 0x41, 0x21, 0x10, 0x08, 0x84, 0x82, 0xff };

SO_NODE_SOURCE(SoAxes);

SoAxes::SoAxes()
{
    SO_NODE_CONSTRUCTOR(SoAxes);
    SO_NODE_ADD_FIELD( linelength_, (4.5) );
    SO_NODE_ADD_FIELD( baseradius_, (0.4) );
    SO_NODE_ADD_FIELD( textcolor_, (1.0f,1.0f,1.0f) );
}


SoAxes::~SoAxes()
{
}


void SoAxes::initClass()
{
    SO_NODE_INIT_CLASS( SoAxes, SoShape, "Shape" );
}


void SoAxes::computeBBox( SoAction* action, SbBox3f& box, SbVec3f& center )
{
    center.setValue(0.0f, 0.0f, 0.0f);
    box.setBounds(SbVec3f(-1, -1, -1 ), SbVec3f(1, 1, 1));
}


void SoAxes::generatePrimitives( SoAction* action )
{ 
// This is supposed to be empty. There are no primitives.
}


void SoAxes::GLRender( SoGLRenderAction* action )
{
    if ( !shouldGLRender(action) ) return;

    SoMaterialBundle mb(action);
    mb.sendFirst();
  
    glPushMatrix();
   
    const float length = linelength_.getValue();
    const float rad = baseradius_.getValue();

    drawArrow( 1, length, rad );
    drawArrow( 2, length, rad );
    drawArrow( 3, length, rad );

    drawSphere( rad*0.75f, 0, 0, 0 );
 
    GLint unpack;
    glGetIntegerv( GL_UNPACK_ALIGNMENT, &unpack );
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

 
    glDisable( GL_LIGHTING );
    
    const char* ret = getenv( "OD_AXES_SHOW_TRIANGLE" );
    if ( ret )
    {    
	glColor3f( 1.0, 1.0, 1.0 );
	glBegin( GL_TRIANGLES ) ;
	glVertex2f( 0, 0 );
	glVertex2f( length - 0.4f, 0 );
	glVertex2f( 0, length - 0.4f );
	glEnd();
    }

    glClear(GL_DEPTH_BUFFER_BIT);
    glColor3f( textcolor_.getValue()[0], textcolor_.getValue()[1],
	       textcolor_.getValue()[2] );
    glRasterPos3d( 0, 0, -length - 0.2 );
    glBitmap(8, 8, 0, 0, 0, 0, zbmp );
    glRasterPos3d( length + 0.2, 0, 0 );
    glBitmap(8, 8, 0, 0, 0, 0, ebmp );
    glRasterPos3d( 0, length + 0.2, 0 );
    glBitmap(8, 8, 0, 0, 0, 0, nbmp );

    glEnable( GL_LIGHTING );		
    glPopMatrix();
}


void SoAxes::drawArrow( int typ, float lnt, float rad )
{
    float angl = 0, xdir = 0, ydir = 0, ht = 0, cnht = 0;
    float nrm[3];
    int idx = 0;

    ht = lnt;
    cnht = ht / 3;
    
    glPushMatrix();

    glEnable( GL_LIGHT0 );						
    glEnable( GL_LIGHTING );						
    glEnable( GL_COLOR_MATERIAL );

    if ( typ == 1 )
    {
	ht = lnt;
	cnht = ht / 3;

	glColor3ub( 0, 255, 0 ) ;

	glBegin( GL_TRIANGLE_STRIP ) ;

	for ( idx=0; idx<=100; idx++ )
	{
	    angl = (float) ( idx * 2 * M_PI / 100 );
            xdir = rad * sin( angl );
	    ydir = rad * cos( angl );

	    nrm[0] = xdir / rad;
	    nrm[1] = cos( atan((rad/2)/cnht) );
	    nrm[2] = ydir / rad;
                    
	    glNormal3fv( nrm );
	    glVertex3f( 0 , ht, 0 );
	    glVertex3f( xdir, ht-cnht, ydir );
	}
        	
	glEnd();
            
	glBegin( GL_TRIANGLE_STRIP ) ;

	for ( idx=0; idx<=100; idx++ )
	{
	    angl = (float) ( idx * 2 * M_PI / 100 );
	    glNormal3f( 0, -1, 0 ); 

	    xdir = 0.5f * rad * sin( angl );
	    ydir = 0.5f * rad * cos( angl );
	    glVertex3f( xdir , ht-cnht, ydir );
	
	    xdir = rad * sin( angl );
	    ydir = rad * cos( angl );
	    glVertex3f( xdir, ht-cnht, ydir );
	}
        	
	glEnd();
            
	glBegin( GL_TRIANGLE_STRIP ) ;

	for ( idx=0; idx<=100; idx++ )
	{
	    angl = (float) ( idx * 2 * M_PI / 100 );
	    xdir = 0.5f * rad * sin( angl );
	    ydir = 0.5f * rad * cos( angl );
            	
	    nrm[0] = sin( angl );
	    nrm[2] = cos( angl );
	    nrm[1] = 0;
                
	    glNormal3fv( nrm );
	    glVertex3f( xdir, ht-cnht, ydir);
	    glVertex3f( xdir, 0, ydir );
	}
        	
	glEnd();

	glBegin( GL_TRIANGLE_STRIP ) ;

	for ( idx=0; idx<=100; idx++ )
	{
	    angl = (float) ( idx * 2 * M_PI / 100 );
	    xdir = 0.5f * rad * sin( angl );
	    ydir = 0.5f * rad * cos( angl );
            
	    glNormal3f( 0, -1, 0 ); 
	    glVertex3f( 0, 0, 0 );
	    glVertex3f( xdir, 0, ydir );
	}
        	
	glEnd();
    }


    if ( typ == 2 )
    {
	ht = -1 * lnt;
	cnht = ht / 3;
        	 
	glColor3ub ( 64, 64, 255 ) ;

	glBegin ( GL_TRIANGLE_STRIP ) ;

	for ( idx=0; idx<=100; idx++ )
	{
	    angl = (float) ( idx * 2 * M_PI / 100 );
	    xdir = rad * sin( angl );
	    ydir = rad * cos( angl );
                
	    nrm[0] = ( xdir/rad );
	    nrm[1] = ( ydir/rad );
	    nrm[2] = -1 * ( cos( atan((rad/2)/cnht) ) );
                
	    glNormal3fv( nrm );
	    glVertex3f( 0 , 0, ht );
	    glVertex3f( xdir, ydir, ht-cnht );
	}
        	
	glEnd();
            
	glBegin ( GL_TRIANGLE_STRIP ) ;

	for ( idx=0; idx<=100; idx++ )
	{
	    angl = (float) ( idx * 2 * M_PI / 100 );
	    glNormal3f( 0, 0, 1 ); 
	    
	    xdir = 0.5f * rad * sin( angl );
	    ydir = 0.5f * rad * cos( angl );
            glVertex3f( xdir, ydir, ht-cnht );
	    
	    xdir = rad * sin( angl );
	    ydir = rad * cos( angl );
            glVertex3f( xdir, ydir, ht-cnht );
	}
        	
	glEnd();
            
	glBegin ( GL_TRIANGLE_STRIP ) ;

	for ( idx=0; idx<=100; idx++ )
	{
	    angl = (float) ( idx * 2 * M_PI / 100 );
	    xdir = 0.5f * rad * sin( angl );
	    ydir = 0.5f * rad * cos( angl );
            	
	    nrm[0] = xdir / ( 0.5f*rad );
	    nrm[1] = ydir / ( 0.5f*rad ) ;
	    nrm[2] = 0;
                
	    glNormal3fv( nrm );
	    glVertex3f( xdir, ydir, ht-cnht );
	    glVertex3f( xdir, ydir, 0 );
	}
        	
	glEnd();

	glBegin ( GL_TRIANGLE_STRIP ) ;

	for ( idx=0; idx<=100; idx++ )
	{
	    angl = (float) ( idx * 2 * M_PI / 100 );
	    xdir = 0.5f * rad * sin( angl );
	    ydir = 0.5f * rad * cos( angl );
            
	    glNormal3f( 0, 0, 1 ); 
	    glVertex3f( 0, 0, 0);
	    glVertex3f( xdir, ydir, 0);
	}
        	
	glEnd();
    }

    
    if ( typ == 3 )
    {

	glColor3ub ( 255, 0, 0 ) ;
	glBegin ( GL_TRIANGLE_STRIP ) ;

	for ( idx=0; idx<=100; idx++ )
	{
	    angl = (float) ( idx * 2 * M_PI / 100 );
	    xdir = rad * sin( angl );
	    ydir = rad * cos( angl );

	    nrm[0] = cos( atan((rad/2)/cnht) );
	    nrm[1] = xdir / rad;
	    nrm[2] = ydir / rad;
            	
	    glNormal3fv( nrm );
	    glVertex3f( ht, 0, 0 );
	    glVertex3f( ht-cnht, xdir, ydir );
	}
        	
	glEnd();
            
	glBegin ( GL_TRIANGLE_STRIP ) ;

	for ( idx=0; idx<=100; idx++ )
	{
	    angl = (float) ( idx * 2 * M_PI / 100 );
    	    
	    glNormal3f( -1, 0, 0 );
    	
	    xdir = 0.5f * rad * sin( angl );
	    ydir = 0.5f * rad * cos( angl );
	    glVertex3f( ht-cnht, xdir, ydir );
    	
	    xdir = rad * sin( angl );
	    ydir = rad * cos( angl );
	    glVertex3f( ht-cnht, xdir, ydir );
	}
        	
	glEnd();
            

	glBegin ( GL_TRIANGLE_STRIP ) ;

	for ( idx=0; idx<=100; idx++ )
	{
	    angl = (float) ( idx * 2 * M_PI / 100 );
	    xdir = 0.5f * rad * sin( angl );
	    ydir = 0.5f * rad * cos( angl );

	    nrm[0] = 0;
	    nrm[1] = xdir / ( rad * 0.5f );
	    nrm[2] = ydir / ( rad * 0.5f );
                
	    glNormal3fv( nrm );
	    glVertex3f( ht-cnht, xdir, ydir );
	    glVertex3f( 0, xdir, ydir );
	}
        	
	glEnd();


	glBegin ( GL_TRIANGLE_STRIP ) ;

	for ( idx=0; idx<=100; idx++ )
	{
	    angl = (float) ( idx * 2 * M_PI / 100 );
	    xdir = 0.5f * rad * sin( angl );
	    ydir = 0.5f * rad * cos( angl );
                
	    glNormal3f( -1, 0, 0 );
	    glVertex3f( 0, 0, 0 );
	    glVertex3f( 0, xdir, ydir );
	}
        	
	glEnd();
    }

    glDisable( GL_LIGHTING );
    glPopMatrix();
}


void SoAxes::drawSphere( float rad, float u, float v, float w )
{
    float rf = 0, rs = 0, hf = 0, hs = 0, anglx = 0, angly = 0;
   
    glEnable( GL_LIGHT0 );						
    glEnable( GL_LIGHTING );						
    glEnable( GL_COLOR_MATERIAL );

    glColor3ub( 255, 255, 0 );

    for ( int j=0; j<=100; j++ )
    {
	angly = (float) ( j *  M_PI / 100 );
	rs = rad * sin( angly );
	hs = rad * cos( angly );

	glBegin( GL_QUAD_STRIP );

	for( int i=0; i<=100; i++ )
	{ 
	    anglx = (float) ( i * 2 * M_PI / 100 );
	    if( j!=0 )
	    {
		glNormal3f( sin(anglx)*rf/rad, hf/rad, cos(anglx)*rf/rad );
		glVertex3f( sin(anglx)*rf+u, hf+v, cos(anglx)*rf+w );
		glNormal3f( sin(anglx)*rs/rad, hs/rad, cos(anglx)*rs/rad );
		glVertex3f( sin(anglx)*rs+u, hs+v, cos(anglx)*rs+w );
	    }
	}
	rf = rs;
	hf = hs;
	glEnd();
    }

    glDisable( GL_LIGHTING );
}
