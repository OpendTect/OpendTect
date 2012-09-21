/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2003
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "visrandomtrack.h"

#include "SoRandomTrackLineDragger.h"
#include "vistexture2.h"
#include "vistristripset.h"
#include "viscoord.h"
#include "visevent.h"
#include "vistexturecoords.h"
#include "visdataman.h"
#include "viscoltabmod.h"
#include "vistransform.h"
#include "iopar.h"
#include "errh.h"

#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoMaterial.h>

mCreateFactoryEntry( visBase::RandomTrack );

namespace visBase
{

const char* RandomTrack::textureidstr = "Texture ID";
const char* RandomTrack::draggersizestr = "DraggerSize";

RandomTrack::RandomTrack()
    : VisualObjectImpl(false)
    , dragger(0)
    , draggerswitch(0)
    , depthrg(0,1)
    , knotmovement(this)
    , knotnrchange(this)
    , sectionidx(-1)
    , transformation(0)
{
    SoShapeHints* hints = new SoShapeHints;
    hints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    addChild( hints );

    addKnot( Coord( 0, 0 ) );
    addKnot( Coord( 1, 0 ) );
}


RandomTrack::~RandomTrack()
{
    for ( int idx=0; idx<sections.size(); idx++ )
    {
	removeChild( sections[idx]->getInventorNode() );
	sections[idx]->unRef();
    }

    if ( transformation ) transformation->unRef();
}


void RandomTrack::setDisplayTransformation( const mVisTrans* tf )
{
    if ( transformation ) transformation->unRef();
    transformation = tf;
    if ( transformation ) transformation->ref();
    rebuild();
}


const mVisTrans* RandomTrack::getDisplayTransformation() const
{
    return transformation;
}


void RandomTrack::showDragger( bool yn )
{
    if ( yn )
    {
	if ( !draggerswitch )
	{
	    createDragger();
	}

	draggerswitch->whichChild = 0;
	moveDraggerToObjectPos();

    }
    else if ( !yn && draggerswitch )
    {
	draggerswitch->whichChild = SO_SWITCH_NONE;
    }
    
}


bool RandomTrack::isDraggerShown() const
{
    return draggerswitch &&draggerswitch->whichChild.getValue()!=SO_SWITCH_NONE;
}


void RandomTrack::moveObjectToDraggerPos()
{
    if ( !dragger ) return;
    
    for ( int idx=0; idx<knots.size(); idx++ )
    {
	SbVec2f kp = dragger->knots[idx];
	knots[idx].x = kp[0];
	knots[idx].y = kp[1];
    }

    depthrg.start = dragger->z0.getValue();
    depthrg.stop = dragger->z1.getValue();
    rebuild();
    dragger->showFeedback(false);
}


void RandomTrack::moveDraggerToObjectPos()
{
    if ( !dragger ) return;

    if ( dragger->knots.getNum()>knots.size() )
	dragger->knots.deleteValues(knots.size());

    for ( int idx=0; idx<knots.size(); idx++ )
    {
	dragger->knots.set1Value( idx, (float) knots[idx].x, 
					(float) knots[idx].y );
    }

    dragger->z0 = depthrg.start;
    dragger->z1 = depthrg.stop;
}


int RandomTrack::nrKnots() const
{ return knots.size(); }


void RandomTrack::setTrack( const TypeSet<Coord>& posset )
{
    copy( knots, posset );
    rebuild();
}


void RandomTrack::addKnot( const Coord& pos )
{
/*
    Always rebuild dragger. If draggers not shown, show and hide them again.
    TODO: make separate function for rebuilding dragger
    	  (separate from showDragger)
*/
    const bool draggershown = isDraggerShown();
    knots += pos;
    rebuild();
    showDragger( true );		//Rebuild the dragger
    knotnrchange.trigger();
    if ( !draggershown )
	showDragger( false );
}


void RandomTrack::insertKnot( int idx, const Coord& pos )
{
    const bool draggershown = isDraggerShown();
    knots.insert( idx, pos );
    rebuild();
    showDragger( true );		//Rebuild the dragger
    knotnrchange.trigger();
    if ( !draggershown )
	showDragger( false );
}


Coord RandomTrack::getKnotPos( int idx ) const
{ return knots[idx]; }


Coord RandomTrack::getDraggerKnotPos( int idx ) const
{
    if ( !dragger ) return getKnotPos( idx );
    SbVec2f draggerpos = dragger->knots[idx];
    return Coord( draggerpos[0], draggerpos[1] );
}


void RandomTrack::setKnotPos( int idx, const Coord& pos )
{
    if ( idx < nrKnots() )
    {
	knots[idx] = pos;
	rebuild();
    }
    else
	addKnot( pos );
}


void RandomTrack::setDraggerKnotPos( int idx, const Coord& pos )
{
    if ( !dragger ) { setKnotPos( idx, pos ); return; }
    dragger->knots.set1Value( idx, (float) pos.x, (float) pos.y );
}


void RandomTrack::removeKnot( int idx )
{
    if ( knots.size()< 3 )
    {
	pErrMsg("Can't remove knot");
	return;
    }

    knots.remove( idx );
    rebuild();

    if ( dragger )
	dragger->knots.deleteValues( idx, 1 );

    knotnrchange.trigger();
}


void RandomTrack::setDepthInterval( const Interval<float>& drg )
{
    depthrg = drg;
    rebuild();
}


const Interval<float> RandomTrack::getDepthInterval() const
{ return depthrg; }


void RandomTrack::setDraggerDepthInterval( const Interval<float>& intv )
{
    if ( !dragger ) return;
    dragger->z0 = intv.start;
    dragger->z1 = intv.stop;
}


const Interval<float> RandomTrack::getDraggerDepthInterval() const
{
    if ( !dragger ) return getDepthInterval();
    return Interval<float>( dragger->z0.getValue(), dragger->z1.getValue() );
}


#define mSetRange( dim )  \
    createDragger(); \
    SbVec3f xyzstart = dragger->xyzStart.getValue(); \
    SbVec3f xyzstop = dragger->xyzStop.getValue(); \
    SbVec3f xyzstep = dragger->xyzStep.getValue(); \
 \
    xyzstart[dim] = rg.start; \
    xyzstop[dim] = rg.stop; \
    xyzstep[dim] = rg.step; \
 \
    dragger->xyzStart.setValue( xyzstart ); \
    dragger->xyzStop.setValue( xyzstop ); \
    dragger->xyzStep.setValue( xyzstep )

void RandomTrack::setXrange( const StepInterval<float>& rg )
{
    mSetRange( 0 );
}

void RandomTrack::setYrange( const StepInterval<float>& rg )
{
    mSetRange( 1 );
}

void RandomTrack::setZrange( const StepInterval<float>& rg )
{
    mSetRange( 2 );
}


void RandomTrack::setDraggerSize( const Coord3& nz )
{
    createDragger();
    SoScale* size =
	dynamic_cast<SoScale*>(dragger->getPart("subDraggerScale", true ));
    size->scaleFactor.setValue( (float) nz.x, (float) nz.y, (float) nz.z );
}


Coord3 RandomTrack::getDraggerSize() const
{
    if ( !dragger ) return Coord3(0,0,0);
    SoScale* size =
	dynamic_cast<SoScale*>(dragger->getPart( "subDraggerScale", true ));
    SbVec3f pos = size->scaleFactor.getValue();
    Coord3 res( pos[0], pos[1], pos[2] );
    return res;
}


void RandomTrack::setClipRate( Interval<float> nc )
{
    const int nrsections = sections.size();
    for ( int idx=0; idx<nrsections; idx++ )
    {
	Texture2* texture = sections[idx]->getTexture2();
	texture->setClipRate( nc );
    }
}


Interval<float> RandomTrack::clipRate() const
{ return sections[0]->getTexture2()->clipRate(); }


void RandomTrack::setAutoScale( bool nc )
{
    const int nrsections = sections.size();
    for ( int idx=0; idx<nrsections; idx++ )
    {
	Texture2* texture = sections[idx]->getTexture2();
	texture->setAutoScale( nc );
    }
}


bool RandomTrack::autoScale() const
{ return sections[0]->getTexture2()->autoScale(); }


void RandomTrack::setColorTab( VisColorTab& nc )
{
    const int nrsections = sections.size();
    for ( int idx=0; idx<nrsections; idx++ )
    {
	Texture2* texture = sections[idx]->getTexture2();
	texture->setColorTab( nc );
    }
}


VisColorTab& RandomTrack::getColorTab()
{ return sections[0]->getTexture2()->getColorTab(); }


const TypeSet<float>& RandomTrack::getHistogram() const
{ return sections[0]->getTexture2()->getHistogram(); }


void RandomTrack::setMaterial( Material* mat )
{
    VisualObjectImpl::setMaterial( mat );
    const int nrsections = sections.size();
    for ( int idx=0; idx<nrsections; idx++ )
	sections[idx]->setMaterial( mat );
}


Material* RandomTrack::getMaterial()
{ return sections[0]->getMaterial(); }


void RandomTrack::useTexture( bool yn )
{
    for ( int idx=0; idx<sections.size(); idx++ )
	sections[idx]->getTexture2()->turnOn(yn);
}


bool RandomTrack::usesTexture() const
{
    return sections.size() ? sections[0]->getTexture2()->isOn() : false;
}


void RandomTrack::setData( int section, const Array2D<float>* data,
       				    int type )
{
    Texture2* texture = sections[section]->getTexture2();
    texture->setData( data, (Texture::DataType)type );
}


void RandomTrack::setColorPars( bool rev, bool useclip,
                                         const Interval<float>& intv )
{
    const int nrsections = sections.size();
    for ( int idx=0; idx<nrsections; idx++ )
    {
	Texture2* texture = sections[idx]->getTexture2();
	VisColTabMod& ctm = texture->getColTabMod();
	ctm.doReverse( rev );
	ctm.useClipping( useclip );
	useclip ? ctm.setClipRate(intv.start,intv.stop) : ctm.setRange(intv);
    }
}


const Interval<float>& RandomTrack::getColorDataRange() const
{
    return sections[0]->getTexture2()->getColTabMod().getRange();
}


void RandomTrack::rebuild()
{
    while ( sections.size()<knots.size()-1 )
    {
	TriangleStripSet* strip = TriangleStripSet::create();
	Texture2* texture = Texture2::create();
	strip->setTexture2( texture );
	strip->ref();
	if ( sections.size() )
	{
	    strip->setCoordinates( sections[0]->getCoordinates() );
	    strip->getTexture2()->setColorTab( 
		    sections[0]->getTexture2()->getColorTab() );
	    strip->setMaterial( sections[0]->getMaterial() );
	    strip->getTexture2()->setResolution( 
		    sections[0]->getTexture2()->getResolution() );
	}

	TextureCoords* texturecoords = TextureCoords::create();
	strip->setTextureCoords( texturecoords );
	texturecoords->addCoord( Coord( 0, 0 ) );
	texturecoords->addCoord( Coord( 0, 1 ) );
	texturecoords->addCoord( Coord( 1, 0 ) );
	texturecoords->addCoord( Coord( 1, 1 ) );

	strip->setTextureCoordIndex( 0, 0 );
	strip->setTextureCoordIndex( 1, 1 );
	strip->setTextureCoordIndex( 2, 2 );
	strip->setTextureCoordIndex( 3, 3 );

	sections += strip;
	addChild( strip->getInventorNode() );
    }

    while ( sections.size()>knots.size()-1 )
    {
	const int idx = sections.size()-1;
	TriangleStripSet* strip = sections[idx];
	removeChild( strip->getInventorNode() );
	strip->unRef();
	sections.remove( idx );
    }

    if ( sections.isEmpty() ) return;

    Coordinates* coords = sections[0]->getCoordinates();
    if ( transformation ) coords->setDisplayTransformation( transformation );

    for ( int idx=0; idx<knots.size(); idx++ )
    {
	coords->setPos( idx*2,
		Coord3( knots[idx].x, knots[idx].y, depthrg.start ));
	coords->setPos( idx*2+1,
		Coord3( knots[idx].x, knots[idx].y, depthrg.stop ));

	if ( !idx ) continue;

	sections[idx-1]->setCoordIndex( 0, (idx-1)*2 );
	sections[idx-1]->setCoordIndex( 1, (idx-1)*2+1 );
	sections[idx-1]->setCoordIndex( 2, idx*2 );
	sections[idx-1]->setCoordIndex( 3, idx*2+1 );
	sections[idx-1]->setCoordIndex( 4, -1 );
    }

}


void RandomTrack::createDragger()
{
    if ( draggerswitch ) return;

    dragger = new SoRandomTrackLineDragger;
    dragger->addMotionCallback( RandomTrack::motionCB, this );
    dragger->addStartCallback( RandomTrack::startCB, this );
    draggerswitch = new SoSwitch;
    insertChild( 0, draggerswitch );
    draggerswitch->addChild( dragger );

    SoMaterial* newmaterial = new SoMaterial;

    newmaterial->transparency.setValue( 0.9 );
    dragger->setPart( "feedbackMaterial", newmaterial );
}


void RandomTrack::motionCB( void* data,
				     SoRandomTrackLineDragger* dragger)
{
    RandomTrack* myself = (RandomTrack*) data;
    myself->knotmovement.trigger( dragger->getMovingKnot() );
}


void RandomTrack::startCB( void* data,
				    SoRandomTrackLineDragger* dragger)
{ }


void RandomTrack::setResolution( int res )
{
    for ( int idx=0; idx<sections.size(); idx++ )
	sections[idx]->getTexture2()->setResolution( res );
}


int RandomTrack::getResolution() const
{
    return sections[0]->getTexture2()->getResolution();
}


void RandomTrack::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    int textureid = sections[0]->getTexture2()->id();
    par.set( textureidstr, textureid );

    Coord3 size = getDraggerSize();
    par.set( draggersizestr, size.x, size.y, size.z );

    if ( saveids.indexOf(textureid) == -1 ) saveids += textureid;
}


int RandomTrack::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res != 1 ) return res;

    Coord3 size(1,1,1);
    par.get( draggersizestr, size.x, size.y, size.z );
    setDraggerSize( size );

    int textureid;
    if ( !par.get( textureidstr, textureid ) ) return -1;
    DataObject* dataobj = DM().getObject( textureid );
    if ( !dataobj ) return 0;
    mDynamicCastGet(Texture2*,texture_,dataobj);
    if ( !texture_ ) return -1;

    for ( int idx=0; idx<sections.size(); idx++ )
        sections[idx]->setTexture2( texture_ );

    return 1;
}


void RandomTrack::triggerRightClick(const EventInfo* ei)
{
    const TypeSet<int>* path = &ei->pickedobjids;
     sectionidx = -1;
     for ( int idx=0; idx<path->size(); idx++ )
     {
	 for ( int idy=0; idy<sections.size(); idy++ )
	 {
	     if ( (*path)[idx]==sections[idy]->id() )
	     {
		 sectionidx = idy;
		 break;
	     }
	 }
	 
	 if ( sectionidx != -1 )
	     break;
     }

     if ( sectionidx == -1 )
	 return;

     VisualObject::triggerRightClick(ei);
}

}; // namespace visBase
