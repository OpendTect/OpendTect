/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          November 2003
 RCS:           $Id: SoArrow.cc,v 1.1 2003-11-28 15:38:56 nanne Exp $
________________________________________________________________________

-*/


#include "SoArrow.h"

#include "GL/gl.h"
#include <Inventor/actions/SoGLRenderAction.h>



SO_NODE_SOURCE(SoArrow);

SoArrow::SoArrow()
{
    SO_NODE_CONSTRUCTOR(SoArrow);
    SO_NODE_ADD_FIELD( lineLength, (5) );
    SO_NODE_ADD_FIELD( lineWidth, (1) );
    SO_NODE_ADD_FIELD( headHeight, (2) );
    SO_NODE_ADD_FIELD( headWidth, (1) );
}


SoArrow::~SoArrow()
{
}


void SoArrow::initClass()
{
    SO_NODE_INIT_CLASS( SoArrow, SoShape, "Shape" );
}


void SoArrow::computeBBox( SoAction* action, SbBox3f& box, SbVec3f& center )
{
}


void SoArrow::generatePrimitives( SoAction* action )
{ 
// This is supposed to be empty. There are no primitives.
}


void SoArrow::GLRender( SoGLRenderAction* action )
{
    const float width = lineWidth.getValue();
    const float length = lineLength.getValue();
    const float hh = headHeight.getValue();
    const float hw = headWidth.getValue();
    
    glLineWidth( width );
    glPushMatrix();

    glBegin(GL_LINES);
    glVertex3f( 0, 0, 0 );
    glVertex3f( length, 0, 0 );
    glEnd();
    
    glDisable(GL_CULL_FACE);
    glBegin(GL_TRIANGLES);
    glVertex3f( length+hh, 0, 0);
    glVertex3f( length, hw/2, 0 );
    glVertex3f( length, -hw/2, 0 );
    glVertex3f( length+hh, 0, 0 );
    glVertex3f( length, 0, hw/2 );
    glVertex3f( length, 0, -hw/2 );
    glEnd();

    glBegin(GL_QUADS);
    glVertex3f( length, hw/2, 0 );
    glVertex3f( length, 0, hw/2 );
    glVertex3f( length, -hw/2, 0 );
    glVertex3f( length, 0, -hw/2 );
    glEnd();

    glPopMatrix();
}
