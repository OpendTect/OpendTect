/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vistristripset.cc,v 1.5 2002-05-24 11:45:24 kristofer Exp $";

#include "vistristripset.h"
#include "geomposlist.h"
#include "geomtristripset.h"
#include "viscolortab.h"
#include "dataclipper.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoCoordinate3.h"
#include "Inventor/nodes/SoIndexedTriangleStripSet.h"
#include "Inventor/nodes/SoMaterial.h"
#include "Inventor/nodes/SoMaterialBinding.h"
#include "Inventor/nodes/SoSwitch.h"

mCreateFactoryEntry( visBase::TriangleStripSet );


visBase::TriangleStripSet::TriangleStripSet()
    : coords( new SoCoordinate3 )
    , strips( new SoIndexedTriangleStripSet )
    , data( 0 )
    , colortable( 0 )
    , stripset( 0 )
{
    textureswitch = new SoSwitch;
    addChild( textureswitch );
    SoGroup* texturegroup = new SoSeparator;
    textureswitch->addChild( texturegroup );
    ctmaterial = new SoMaterial;
    texturegroup->addChild( ctmaterial );
    SoMaterialBinding* mbind = new SoMaterialBinding;
    mbind->value = SoMaterialBinding::PER_VERTEX_INDEXED;
    texturegroup->addChild( mbind );
    textureswitch->whichChild = SO_SWITCH_NONE;

    addChild( coords );
    addChild( strips );
}


visBase::TriangleStripSet::~TriangleStripSet()
{
    if ( stripset )
    {
	stripset->poslist.removeNotification(
				mCB( this, TriangleStripSet, updateCoords ));
	stripset->indexchnotifier.remove(
				mCB( this, TriangleStripSet, updateIndexes ));
    }

    delete [] data;

    if ( colortable )
    {
	colortable->unRef();
	colortable->change.remove(
		mCB( this,visBase::TriangleStripSet, updateTexture));
    }
}


void visBase::TriangleStripSet::setStrips( Geometry::TriangleStripSet* ns, 
					   bool connect )
{
    if ( stripset )
    {
	stripset->poslist.removeNotification(
				mCB( this, TriangleStripSet, updateCoords ));
	stripset->indexchnotifier.remove(
				mCB( this, TriangleStripSet, updateIndexes ));
    }

    stripset = ns;
    updateCoords();
    updateIndexes();

    if ( !connect ) stripset = 0;
    else
    {
	stripset->poslist.notify(mCB( this, TriangleStripSet, updateCoords ));
	stripset->indexchnotifier.notify(
				 mCB( this, TriangleStripSet, updateIndexes ));
    }
}


void visBase::TriangleStripSet::setValues( const float* newdata )
{
    int sz = coords->point.getNum();

    delete [] data; data = new float[sz];

    memcpy( data, newdata, sz*sizeof(float));

    updateTexture();
}


void visBase::TriangleStripSet::setColorTab( VisColorTab* nr )
{
    if ( colortable )
    {
	colortable->change.remove(
		mCB( this,visBase::TriangleStripSet, updateTexture));
	colortable->unRef();
    }

    colortable = nr;
    colortable->ref();
    colortable->change.notify(mCB( this,visBase::TriangleStripSet,
		updateTexture));

    updateTexture();
}


void visBase::TriangleStripSet::setClipRate(float n )
{
    cliprate = n;
    clipData();
    autoscale = true;
}


void visBase::TriangleStripSet::useTexture( bool n )
{
    textureswitch->whichChild = n ? 0 : SO_SWITCH_NONE;
}


bool visBase::TriangleStripSet::usesTexture() const
{
    return textureswitch->whichChild.getValue() == 0;
}


void visBase::TriangleStripSet::clipData()
{
    if ( !data ) return;
    int nrvalues = coords->point.getNum();
    DataClipper clipper( cliprate );
    clipper.setApproxNrValues( nrvalues, 10000 );
    clipper.putData( data, nrvalues );
    clipper.calculateRange();

    colortable->scaleTo( clipper.getRange() );
}


void visBase::TriangleStripSet::updateCoords( CallBacker* )
{
    coords->point.setValues( 0, stripset->poslist.highestId()+1,
		    (const float (*)[3]) stripset->poslist.getCoords( 0 ) );
}


void visBase::TriangleStripSet::updateIndexes( CallBacker* )
{
    strips->coordIndex.setValues( 0, stripset->getNrIds(), stripset->getIds() );
}


void visBase::TriangleStripSet::updateTexture()
{
    if ( !data ) return;

    if ( !colortable )
    {
	colortable = visBase::VisColorTab::create();
	colortable->ref();
	colortable->change.notify(mCB( this,visBase::TriangleStripSet,
		    updateTexture));
    }

    const int nrsteps = 1024;

    colortable->setNrSteps( nrsteps );

    for ( int idx=0; idx<nrsteps; idx++ )
    {
	Color col = colortable->tableColor( idx );
	ctmaterial->diffuseColor.set1Value( idx, col.r(), col.g(), col.b() );
	ctmaterial->transparency.set1Value( idx, col.t() );
    }

    Color col = colortable->color( mUndefValue );
    ctmaterial->diffuseColor.set1Value( nrsteps, col.r(), col.g(), col.b() );
    ctmaterial->transparency.set1Value( nrsteps, col.t() );

    const int nrvalues = coords->point.getNum();

    for ( int idx=0; idx<nrvalues; idx++ )
    {
	int ctidx = colortable->colIndex( data[idx] );
	strips->materialIndex.set1Value( idx, ctidx );
    }
}
