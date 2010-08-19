/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visannot.cc,v 1.32 2010-08-19 08:21:17 cvsranojay Exp $";

#include "visannot.h"
#include "vistext.h"
#include "visdatagroup.h"
#include "vispickstyle.h"
#include "vismaterial.h"
#include "ranges.h"
#include "samplingdata.h"
#include "axislayout.h"
#include "iopar.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoIndexedLineSet.h"
#include "Inventor/nodes/SoCoordinate3.h"
#include "Inventor/nodes/SoSwitch.h"

mCreateFactoryEntry( visBase::Annotation );

namespace visBase
{

const char* Annotation::textprefixstr()	    { return "Text "; }
const char* Annotation::cornerprefixstr()   { return "Corner "; }
const char* Annotation::showtextstr()	    { return "Show Text"; }
const char* Annotation::showscalestr()	    { return "Show Scale"; }

Annotation::Annotation()
    : VisualObjectImpl( false )
    , coords(new SoCoordinate3)
    , textswitch(new SoSwitch)
    , scaleswitch(new SoSwitch)
    , pickstyle(PickStyle::create())
    , texts(0)
{
    annotcolor_ = Color::White();
    pickstyle->ref();
    addChild( pickstyle->getInventorNode() );
    pickstyle->setStyle( PickStyle::Unpickable );

    addChild( coords );

    static float pos[8][3] =
    {
	{ 0, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 }, { 0, 1, 1 },
	{ 1, 0, 0 }, { 1, 0, 1 }, { 1, 1, 0 }, { 1, 1, 1 }
    };

    coords->point.setValues( 0, 8, pos );

    SoIndexedLineSet* line = new SoIndexedLineSet;
    addChild( line );
    line->setName( "Survey box" );

    int coordidx = 0;
    int indexes[] = { 0, 1, 2, 3, 0, -1 };
    line->coordIndex.setValues( coordidx, 6, indexes );
    coordidx += 6;

    indexes[0] = 4; indexes[1] = 5; indexes[2] = 6; indexes[3] = 7;
    indexes[4] = 4;
    line->coordIndex.setValues( coordidx, 6, indexes );
    coordidx += 6;

    indexes[0] = 0; indexes[1] = 4; indexes[2] = -1;
    line->coordIndex.setValues( coordidx, 3, indexes );
    coordidx += 3;
    
    indexes[0] = 1; indexes[1] = 5;
    line->coordIndex.setValues( coordidx, 3, indexes );
    coordidx += 3;
    
    indexes[0] = 2; indexes[1] = 6;
    line->coordIndex.setValues( coordidx, 3, indexes );
    coordidx += 3;
    
    indexes[0] = 3; indexes[1] = 7;
    line->coordIndex.setValues( coordidx, 2, indexes );

    addChild( textswitch );
    texts = DataObjectGroup::create();
    texts->setSeparate( false );
    texts->ref();
    textswitch->addChild( texts->getInventorNode() );
    textswitch->whichChild = 0;

#define mAddText \
    text = Text2::create(); text->setJustification( Text2::Right ); \
    texts->addObject( text );

    Text2* text = 0; mAddText mAddText mAddText

    addChild( scaleswitch );
    SoGroup* scalegroup = new SoGroup;
    scaleswitch->addChild( scalegroup );
    scaleswitch->whichChild = 0;

    DataObjectGroup* scale = DataObjectGroup::create();
    scale->setSeparate( false );
    scale->ref();
    scalegroup->addChild( scale->getInventorNode() );
    scales += scale;
    
    scale = DataObjectGroup::create();
    scale->setSeparate( false );
    scale->ref();
    scalegroup->addChild( scale->getInventorNode() );
    scales += scale;
    
    scale = DataObjectGroup::create();
    scale->setSeparate( false );
    scale->ref();
    scalegroup->addChild( scale->getInventorNode() );
    scales += scale;
    
    updateTextPos();
}


Annotation::~Annotation()
{
    scales[0]->unRef();
    scales[1]->unRef();
    scales[2]->unRef();
    texts->unRef();
    pickstyle->unRef();
}


void Annotation::showText( bool yn )
{
    textswitch->whichChild = yn ? 0 : SO_SWITCH_NONE;
}


bool Annotation::isTextShown() const
{
    return textswitch->whichChild.getValue() == 0;
}


void Annotation::showScale( bool yn )
{
    scaleswitch->whichChild = yn ? 0 : SO_SWITCH_NONE;
}


bool Annotation::isScaleShown() const
{
    return scaleswitch->whichChild.getValue() == 0;
}


void Annotation::setCorner( int idx, float x, float y, float z )
{
    float c[3] = { x, y, z };
    coords->point.setValues( idx, 1, &c );
    updateTextPos();
}


Coord3 Annotation::getCorner( int idx ) const
{
    SbVec3f pos = coords->point[idx];
    Coord3 res( pos[0], pos[1], pos[2] );
    return res;
}


void Annotation::setText( int dim, const char* string )
{
    Text2* text = (Text2*)texts->getObject( dim );
    if ( text )
	text->setText( string );
}


void Annotation::setTextColor( int dim, const Color& col )
{
    Text2* text = (Text2*)texts->getObject( dim );
    if ( text )
	text->getMaterial()->setColor( col );
}


void Annotation::updateTextColor( const Color& col )
{
    annotcolor_ = col;
    for ( int idx=0; idx<3; idx++ )
    {
	setTextColor( idx, annotcolor_ );
    }
    updateTextPos();
}


void Annotation::updateTextPos()
{
    updateTextPos( 0 );
    updateTextPos( 1 );
    updateTextPos( 2 );
}


void Annotation::updateTextPos( int textid )
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

    ((Text2*)texts->getObject(textid))
			->setPosition( Coord3(tp[0],tp[1],tp[2]) );

    int dim = -1;
    if ( mIsEqual(p0[1],p1[1],mDefEps) && mIsEqual(p0[2],p1[2],mDefEps))
	dim = 0;
    else if ( mIsEqual(p0[2],p1[2],mDefEps) && mIsEqual(p0[0],p1[0],mDefEps))
	dim = 1;
    else if ( mIsEqual(p0[1],p1[1],mDefEps) && mIsEqual(p0[0],p1[0],mDefEps) )
	dim = 2;
    else
	return;

    Interval<float> range( p0[dim], p1[dim] );
    const SamplingData<float> sd = AxisLayout( range ).sd;

    for ( int idx=0; ; idx++ )
    {
	float val = sd.atIndex(idx);
	if ( val <= range.start )	continue;
	else if ( val > range.stop )	break;

	Text2* text = Text2::create();
	scales[textid]->addObject( text );
	Coord3 pos( p0[0], p0[1], p0[2] );
	pos[dim] = val;
	text->setPosition( pos );
	text->setText( toString(val) );
	text->getMaterial()->setColor( annotcolor_ );
    }
}


Text2* visBase::Annotation::getText( int dim, int textnr )
{
    DataObjectGroup* group = 0;
    group = scales[dim];
    mDynamicCastGet(Text2*,text,group->getObject(textnr));
    return text;
}


void Annotation::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    BufferString key;
    for ( int idx=0; idx<8; idx++ )
    {
	key = cornerprefixstr();
	key += idx;
	Coord3 pos = getCorner( idx );
	par.set( key, pos.x, pos.y, pos.z );
    }

    for ( int idx=0; idx<3; idx++ )
    {
	key = textprefixstr();
	key += idx;
	Text2* text = (Text2*)texts->getObject( idx );
	if ( !text ) continue;

	par.set( key, (const char*)text->getText() );
    }

    par.setYN( showtextstr(), isTextShown() );
    par.setYN( showscalestr(), isScaleShown() );
}


int Annotation::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res != 1 ) return res;

    BufferString key;
    for ( int idx=0; idx<8; idx++ )
    {
	key = cornerprefixstr();
	key += idx;

	double x, y, z;
	if ( !par.get( key, x, y, z ) )
	    return -1;

	setCorner( idx, x, y, z );
    }

    for ( int idx=0; idx<3; idx++ )
    {
	key = textprefixstr();
	key += idx;

	const char* text = par.find( key );
	if ( !text ) return -1;

	setText( idx, text );
    }

    bool yn = true;
    par.getYN( showtextstr(), yn );
    showText( yn );

    yn = true;
    par.getYN( showscalestr(), yn );
    showScale( yn );

    return 1;
}

}; //namespace
