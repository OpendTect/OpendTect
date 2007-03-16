/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2004
-*/

static const char* rcsID = "$Id: visvolrenscalarfield.cc,v 1.5 2007-03-16 11:24:07 cvsnanne Exp $";

#include "visvolrenscalarfield.h"

#include "arraynd.h"
#include "draw.h"
#include "envvars.h"
#include "viscolortab.h"
#include "valseries.h"
#include "viscolortabindexer.h"

#undef YES
#undef NO
#include <Inventor/nodes/SoGroup.h>

#include <VolumeViz/nodes/SoTransferFunction.h>
#include <VolumeViz/nodes/SoVolumeData.h>

visBase::FactoryEntry visBase::VolumeRenderScalarField::oldnameentry(
		(visBase::FactPtr) visBase::VolumeRenderScalarField::create,
		"VolumeRender::VolumeTexture");


using namespace visBase;

mCreateFactoryEntry( VolumeRenderScalarField );

#define NRCOLORS 256

VolumeRenderScalarField::VolumeRenderScalarField()
    : transferfunc_( new SoTransferFunction )
    , voldata_( new SoVolumeData )
    , root_( new SoGroup )
    , dummytexture_( 255 )
    , indexcache_( 0 )
    , ownsindexcache_( true )
    , datacache_( 0 )
    , ownsdatacache_( true )
    , sz0_( 1 )
    , sz1_( 1 )
    , sz2_( 1 )
    , ctab_( 0 )
    , blendcolor_( Color::White )
{
    root_->ref();
    root_->addChild( voldata_ );
    voldata_->setVolumeData( SbVec3s(1,1,1),
	    		    &dummytexture_, SoVolumeData::UNSIGNED_BYTE );
    if ( GetEnvVarYN("DTECT_VOLREN_NO_PALETTED_TEXTURE") )
	voldata_->usePalettedTexture = FALSE;

    root_->addChild( transferfunc_ );
    setColorTab( *VisColorTab::create() );
    turnOn( true );
}


VolumeRenderScalarField::~VolumeRenderScalarField()
{
    if ( ownsindexcache_ ) delete [] indexcache_;
    if ( ownsdatacache_ ) delete datacache_;
    root_->unref();
    ctab_->unRef();
}


bool VolumeRenderScalarField::turnOn( bool yn )
{
    const bool wason = isOn();
     if ( !yn )
	voldata_->setVolumeData( SbVec3s(1,1,1),
	    		    &dummytexture_, SoVolumeData::UNSIGNED_BYTE );
     else if ( indexcache_ )
	 voldata_->setVolumeData( SbVec3s(sz2_,sz1_,sz0_),
				 indexcache_, SoVolumeData::UNSIGNED_BYTE );

    return wason;
}


bool VolumeRenderScalarField::isOn() const
{
    SbVec3s size;
    void* ptr;
    SoVolumeData::DataType dt;
    return voldata_->getVolumeData(size,ptr,dt) && ptr==indexcache_;
}


void VolumeRenderScalarField::setScalarField( const Array3D<float>* sc )
{
    if ( !sc )
    {
	turnOn( false );
	return;
    }

    const bool isresize = sc->info().getSize(0)!=sz0_ ||
			 sc->info().getSize(1)!=sz1_ ||
			 sc->info().getSize(2)!=sz2_;

    const int64 totalsz = sc->info().getTotalSz();

    bool doset = false;
    if ( isresize )
    {
	sz0_ = sc->info().getSize( 0 );
	sz1_ = sc->info().getSize( 1 );
	sz2_ = sc->info().getSize( 2 );
	doset = true;

	if ( ownsindexcache_ ) delete [] indexcache_;
	indexcache_ = 0;
    }

    if ( ownsdatacache_ ) delete datacache_;

    ownsdatacache_ = false;
    datacache_ = sc->getStorage();
    if ( !datacache_ )
    {
	ValueSeries<float>* myvalser = new ArrayValueSeries<float>( totalsz );
	if ( !myvalser || !myvalser->isOK() )
	{
	    delete myvalser;
	}
	else
	{
	    ArrayNDIter iter( sc->info() );
	    int64 idx = 0;

	    do
	    {
		myvalser->setValue( idx++, sc->get( iter.getPos() ) );
	    } while ( iter.next() );

	    datacache_ = myvalser;
	    ownsdatacache_ = true;
	}
    }

    //TODO: if 8-bit data & some flags, use data itself
    

    if ( ctab_->autoScale() )
    {
	clipData();
    }
    else
    {
	makeIndices( doset );
    }
}


void VolumeRenderScalarField::setColorTab( VisColorTab& ctab )
{
    if ( &ctab==ctab_ )
	return;

    if ( ctab_ )
    {
	ctab_->rangechange.remove(
		mCB(this, VolumeRenderScalarField, colorTabChCB ));
	ctab_->sequencechange.remove(
		mCB(this, VolumeRenderScalarField, colorSeqChCB ));
	ctab_->autoscalechange.remove(
		mCB(this, VolumeRenderScalarField, autoscaleChCB ));
	ctab_->unRef();
    }

    ctab_ = &ctab;
    ctab_->rangechange.notify(
	    mCB( this, VolumeRenderScalarField, colorTabChCB ));
    ctab_->sequencechange.notify(
	    mCB( this, VolumeRenderScalarField,colorSeqChCB));
    ctab_->autoscalechange.notify(
	    mCB( this,VolumeRenderScalarField,autoscaleChCB));
    ctab_->ref();
    ctab_->setNrSteps(255);

    makeColorTables();
    makeIndices( false );
}


VisColorTab& VolumeRenderScalarField::getColorTab()
{ return *ctab_; }


void VolumeRenderScalarField::setBlendColor( const Color& col )
{
    blendcolor_ = col;
    makeColorTables();
}


const Color& VolumeRenderScalarField::getBlendColor() const
{ return blendcolor_; }


const TypeSet<float>& VolumeRenderScalarField::getHistogram() const
{ return histogram_; }


void VolumeRenderScalarField::setVolumeSize(  const Interval<float>& x,
						  const Interval<float>& y,
						  const Interval<float>& z )
{
    const SbBox3f size( x.start, y.start, z.start, x.stop, y.stop, z.stop );
    voldata_->setVolumeSize( size );
}


Interval<float> VolumeRenderScalarField::getVolumeSize( int dim ) const
{
     const SbBox3f size = voldata_->getVolumeSize();
     return Interval<float>( size.getMin()[dim], size.getMax()[dim] );
}


SoNode* VolumeRenderScalarField::getInventorNode()
{ return root_; }


void VolumeRenderScalarField::colorTabChCB(CallBacker*)
{
    makeIndices( false );
    makeColorTables();
}


void VolumeRenderScalarField::colorSeqChCB(CallBacker*)
{
    makeColorTables();
}


void VolumeRenderScalarField::autoscaleChCB(CallBacker*)
{
    clipData();
}


void VolumeRenderScalarField::makeColorTables()
{
    const float redfactor = (float) blendcolor_.r()/(255*255);
    const float greenfactor = (float) blendcolor_.g()/(255*255);
    const float bluefactor = (float) blendcolor_.b()/(255*255);
    const float opacityfactor = (float) (255-blendcolor_.t())/(255*255);

    const bool didnotify = transferfunc_->colorMap.enableNotify( false );
    int cti = 0;
    for ( int idx=0; idx<NRCOLORS; idx++ )
    {
	const ::Color col = ctab_->tableColor( idx );
	transferfunc_->colorMap.set1Value( cti++, col.r()*redfactor );
	transferfunc_->colorMap.set1Value( cti++, col.g()*greenfactor );
	transferfunc_->colorMap.set1Value( cti++, col.b()*bluefactor );
	transferfunc_->colorMap.set1Value( cti++, 1.0-col.t()*opacityfactor );
    }

    transferfunc_->predefColorMap = SoTransferFunction::NONE;

    transferfunc_->colorMap.enableNotify(didnotify);
    transferfunc_->colorMap.touch();
}


void VolumeRenderScalarField::makeIndices( bool doset )
{
    if ( !datacache_ )
	return;

    const int64 totalsz = sz0_*sz1_*sz2_;

    if ( !indexcache_ )
    {
	indexcache_ = new unsigned char[totalsz];
	ownsindexcache_ = true;
    }

    ColorTabIndexer indexer( *datacache_, indexcache_, totalsz, ctab_ );
    if ( !indexer.execute() )
	return;

    int max = 0;
    const unsigned int* histogram = indexer.getHistogram();
    for ( int idx=254; idx>=0; idx-- )
    {
	if ( histogram[idx]>max )
	max = histogram[idx];
    }

    if ( max )
    {
	histogram_.setSize( 255, 0 );
	for ( int idx=254; idx>=0; idx-- )
	    histogram_[idx] = (float) histogram[idx]/max;
    }

    if ( doset )
    {
	voldata_->setVolumeData( SbVec3s(sz2_,sz1_,sz0_),
			         indexcache_, SoVolumeData::UNSIGNED_BYTE );
    }
    else
	voldata_->touch();
}


void VolumeRenderScalarField::clipData()
{
    if ( datacache_ ) ctab_->scaleTo( *datacache_, sz0_*sz1_*sz2_ );
}
