/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visannot.cc,v 1.1 2002-02-06 22:29:14 kristofer Exp $";

#include "visannot.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoIndexedLineSet.h"
#include "Inventor/nodes/SoCoordinate3.h"
#include "Inventor/nodes/SoText2.h"
#include "Inventor/nodes/SoTranslation.h"


visBase::Annotation::Annotation()
    : coords( new SoCoordinate3 )
{
    root->addChild( coords );

    static float pos[8][3] =
    {
	{ 0, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 }, { 0, 1, 1 },
	{ 1, 0, 0 }, { 1, 0, 1 }, { 1, 1, 0 }, { 1, 1, 1 }
    };

    coords->point.setValues( 0, 8, pos );

    SoIndexedLineSet* line = new SoIndexedLineSet;
    int indexes[] = { 0, 1, 2, 3 , 0};
    line->coordIndex.setValues( 0, 5, indexes );
    root->addChild( line );

    line = new SoIndexedLineSet;
    indexes[0] = 4; indexes[1] = 5; indexes[2] = 6; indexes[3] = 7;
    indexes[4] = 4;
    line->coordIndex.setValues( 0, 5,  indexes);
    root->addChild( line );

    line = new SoIndexedLineSet;
    indexes[0] = 0; indexes[1] = 4;
    line->coordIndex.setValues( 0, 2, indexes );
    root->addChild( line );
    
    line = new SoIndexedLineSet;
    indexes[0] = 1; indexes[1] = 5;
    line->coordIndex.setValues( 0, 2, indexes );
    root->addChild( line );
    
    line = new SoIndexedLineSet;
    indexes[0] = 2; indexes[1] = 6;
    line->coordIndex.setValues( 0, 2, indexes );
    root->addChild( line );
    
    line = new SoIndexedLineSet;
    indexes[0] = 3; indexes[1] = 7;
    line->coordIndex.setValues( 0, 2, indexes );
    root->addChild( line );

    texts += new SoText2;
    texts += new SoText2;
    texts += new SoText2;

    textpositions += new SoTranslation;
    textpositions += new SoTranslation;
    textpositions += new SoTranslation;

    SoSeparator* text = new SoSeparator;
    text->addChild( textpositions[0] );
    text->addChild( texts[0] );
    root->addChild( text );

    text = new SoSeparator;
    text->addChild( textpositions[1] );
    text->addChild( texts[1] );
    root->addChild( text );

    text = new SoSeparator;
    text->addChild( textpositions[2] );
    text->addChild( texts[2] );
    root->addChild( text );

    texts[0]->justification = SoText2::CENTER;
    texts[1]->justification = SoText2::CENTER;
    texts[2]->justification = SoText2::CENTER;

    updateTextPos();
}


void visBase::Annotation::setCorner( int idx, float x, float y, float z )
{
    float c[3] = { x, y, z };
    coords->point.setValues( idx, 1, &c );
    updateTextPos();
}


void visBase::Annotation::setText( int dim, const char* text )
{
    texts[dim]->string = text;
}
    


void visBase::Annotation::updateTextPos()
{
    SbVec3f p0 = coords->point[0];
    SbVec3f p1 = coords->point[1];
    SbVec3f tp;
    tp[0] = (p0[0]+p1[0]) / 2;
    tp[1] = (p0[1]+p1[1]) / 2;
    tp[2] = (p0[2]+p1[2]) / 2;
    textpositions[0]->translation = tp;

    p0 = coords->point[0];
    p1 = coords->point[3];
    tp[0] = (p0[0]+p1[0]) / 2;
    tp[1] = (p0[1]+p1[1]) / 2;
    tp[2] = (p0[2]+p1[2]) / 2;
    textpositions[1]->translation = tp;

    p0 = coords->point[0];
    p1 = coords->point[4];
    tp[0] = (p0[0]+p1[0]) / 2;
    tp[1] = (p0[1]+p1[1]) / 2;
    tp[2] = (p0[2]+p1[2]) / 2;
    textpositions[2]->translation = tp;
}
