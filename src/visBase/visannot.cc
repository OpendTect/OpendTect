/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visannot.cc,v 1.14 2002-05-13 14:34:40 bert Exp $";

#include "visannot.h"
#include "vistext.h"
#include "vissceneobjgroup.h"
#include "ranges.h"
#include "samplingdata.h"
#include "axisinfo.h"
#include "iopar.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoIndexedLineSet.h"
#include "Inventor/nodes/SoCoordinate3.h"
#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoPickStyle.h"


const char* visBase::Annotation::textprefixstr = "Text ";
const char* visBase::Annotation::cornerprefixstr = "Corner ";
const char* visBase::Annotation::showtextstr = "Show Text";
const char* visBase::Annotation::showscalestr = "Show Scale";


mCreateFactoryEntry( visBase::Annotation );

visBase::Annotation::Annotation()
    : coords( new SoCoordinate3 )
    , textswitch( new SoSwitch )
    , scaleswitch( new SoSwitch )
    , texts( 0 )
{
    SoPickStyle* pickstyle = new SoPickStyle;
    addChild( pickstyle );
    pickstyle->style = SoPickStyle::UNPICKABLE;

    addChild( coords );

    static float pos[8][3] =
    {
	{ 0, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 }, { 0, 1, 1 },
	{ 1, 0, 0 }, { 1, 0, 1 }, { 1, 1, 0 }, { 1, 1, 1 }
    };

    coords->point.setValues( 0, 8, pos );

    SoIndexedLineSet* line = new SoIndexedLineSet;
    int indexes[] = { 0, 1, 2, 3 , 0};
    line->coordIndex.setValues( 0, 5, indexes );
    addChild( line );

    line = new SoIndexedLineSet;
    indexes[0] = 4; indexes[1] = 5; indexes[2] = 6; indexes[3] = 7;
    indexes[4] = 4;
    line->coordIndex.setValues( 0, 5,  indexes);
    addChild( line );

    line = new SoIndexedLineSet;
    indexes[0] = 0; indexes[1] = 4;
    line->coordIndex.setValues( 0, 2, indexes );
    addChild( line );
    
    line = new SoIndexedLineSet;
    indexes[0] = 1; indexes[1] = 5;
    line->coordIndex.setValues( 0, 2, indexes );
    addChild( line );
    
    line = new SoIndexedLineSet;
    indexes[0] = 2; indexes[1] = 6;
    line->coordIndex.setValues( 0, 2, indexes );
    addChild( line );
    
    line = new SoIndexedLineSet;
    indexes[0] = 3; indexes[1] = 7;
    line->coordIndex.setValues( 0, 2, indexes );
    addChild( line );

    addChild( textswitch );
    texts = visBase::SceneObjectGroup::create();
    texts->setSeparate(false);
    texts->ref();
    textswitch->addChild( texts->getData() );
    textswitch->whichChild = 0;
    Text* text = Text::create(); texts->addObject( text );
    text = Text::create(); texts->addObject( text );
    text = Text::create(); texts->addObject( text );


    addChild( scaleswitch );
    SoGroup* scalegroup = new SoGroup;
    scaleswitch->addChild(scalegroup);
    scaleswitch->whichChild = 0;

    visBase::SceneObjectGroup* scale = visBase::SceneObjectGroup::create();
    scale->setSeparate(false);
    scale->ref();
    scalegroup->addChild( scale->getData() );
    scales += scale;
    
    scale = visBase::SceneObjectGroup::create();
    scale->setSeparate(false);
    scale->ref();
    scalegroup->addChild( scale->getData() );
    scales += scale;
    
    scale = visBase::SceneObjectGroup::create();
    scale->setSeparate(false);
    scale->ref();
    scalegroup->addChild( scale->getData() );
    scales += scale;
    
    updateTextPos();
}


visBase::Annotation::~Annotation()
{
    scales[0]->unRef();
    scales[1]->unRef();
    scales[2]->unRef();
    texts->unRef();
}


void visBase::Annotation::showText( bool yn )
{
    textswitch->whichChild = yn ? 0 : SO_SWITCH_NONE;
}


bool visBase::Annotation::isTextShown() const
{
    return textswitch->whichChild.getValue()==0;
}


void visBase::Annotation::showScale( bool yn )
{
    scaleswitch->whichChild = yn ? 0 : SO_SWITCH_NONE;
}


bool visBase::Annotation::isScaleShown() const
{
    return scaleswitch->whichChild.getValue()==0;
}


void visBase::Annotation::setCorner( int idx, float x, float y, float z )
{
    float c[3] = { x, y, z };
    coords->point.setValues( idx, 1, &c );
    updateTextPos();
}


Geometry::Pos  visBase::Annotation::getCorner( int idx ) const
{
    SbVec3f pos = coords->point[idx];
    Geometry::Pos res( pos[0], pos[1], pos[2] );

    return res;
}


void visBase::Annotation::setText( int dim, const char* string )
{
    visBase::Text* text = (visBase::Text*) texts->getObject( dim );
    if ( !text ) return;

    text->setText( string );
}


void  visBase::Annotation::updateTextPos()
{
    updateTextPos( 0 );
    updateTextPos( 1 );
    updateTextPos( 2 );
}


void visBase::Annotation::updateTextPos(int textid)
{
    int pidx0;
    int pidx1;

    if ( textid==0)
    {
	pidx0 = 0;
	pidx1 = 1;
    }
    else if ( textid==1 )
    {
	pidx0 = 0;
	pidx1 = 3;
    }
    else
    {
	pidx0 = 0;
	pidx1 = 4;
    }

    SbVec3f p0 = coords->point[pidx0];
    SbVec3f p1 = coords->point[pidx1];
    SbVec3f tp;

    scales[textid]->removeAll();

    tp[0] = (p0[0]+p1[0]) / 2;
    tp[1] = (p0[1]+p1[1]) / 2;
    tp[2] = (p0[2]+p1[2]) / 2;

    ((visBase::Text*)texts->getObject(textid))
			->setPosition( Geometry::Pos( tp[0], tp[1], tp[2] ));

    int dim = -1;
    if ( mIS_ZERO(p0[1]-p1[1]) && mIS_ZERO(p0[2]-p1[2])) dim = 0;
    else if ( mIS_ZERO(p0[2]-p1[2]) && mIS_ZERO(p0[0]-p1[0])) dim = 1;
    else if ( mIS_ZERO(p0[1]-p1[1]) && mIS_ZERO(p0[0]-p1[0])) dim = 2;
    if ( dim < 0 ) return;

    Interval<double> range( p0[dim], p1[dim] );
    SamplingData<double> sd = AxisInfo::prettySampling(range);

    int idx = 0;
    for ( int idx=0; ; idx++ )
    {
	double val = sd.atIndex(idx);
	if ( val <= range.start )	continue;
	else if ( val>range.stop )	break;

	Text* text = Text::create();
	scales[textid]->addObject( text );
	Geometry::Pos pos( p0[0], p0[1], p0[2] );
	pos[dim] = val;
	text->setPosition( pos );
	BufferString string = val;
	text->setText( string );
    }
}


visBase::Text* visBase::Annotation::getText( int dim, int textnr )
{
    SceneObjectGroup* group = 0;
    group = scales[dim];

    mDynamicCastGet(visBase::Text*,text,group->getObject(textnr));
    return text;
}


void visBase::Annotation::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    BufferString key;
    for ( int idx=0; idx<8; idx++ )
    {
	key = cornerprefixstr;
	key += idx;

	Geometry::Pos pos = getCorner( idx );

	par.set( key, pos.x, pos.y, pos.z );
    }

    for ( int idx=0; idx<3; idx++ )
    {
	key = textprefixstr;
	key += idx;

	visBase::Text* text = (visBase::Text*) texts->getObject( idx );
	if ( !text ) continue;

	par.set( key, (const char*) text->getText() );
    }

    par.setYN( showtextstr, isTextShown() );
    par.setYN( showscalestr, isScaleShown() );
}


int visBase::Annotation::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res!= 1 ) return res;

    BufferString key;
    for ( int idx=0; idx<8; idx++ )
    {
	key = cornerprefixstr;
	key += idx;

	double x, y, z;
	if ( !par.get( key, x, y, z ) )
	    return -1;

	setCorner( idx, x, y, z );
    }

    for ( int idx=0; idx<3; idx++ )
    {
	key = textprefixstr;
	key += idx;

	const char* text = par.find( key );
	if ( !text ) return -1;

	setText( idx, text );
    }

    bool yn;
    if ( !par.getYN( showtextstr, yn ) )
	return -1;

    showText( yn );

    if ( !par.getYN( showscalestr, yn ) )
	return -1;

    showScale( yn );
    return 1;
}


