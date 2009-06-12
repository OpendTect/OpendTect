/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2004
-*/

static const char* rcsID = "$Id: visvolrenscalarfield.cc,v 1.22 2009-06-12 17:22:32 cvskris Exp $";

#include "visvolrenscalarfield.h"

#include "arraynd.h"
#include "draw.h"
#include "envvars.h"
#include "iopar.h"
#include "valseries.h"
#include "viscolortab.h"
#include "visdataman.h"
#include "settings.h"

#include <Inventor/nodes/SoGroup.h>

#include <VolumeViz/nodes/SoTransferFunction.h>
#include <VolumeViz/nodes/SoVolumeData.h>

mCreateFactoryEntry( visBase::VolumeRenderScalarField );

namespace visBase
{

#define mNrColors 256
static const char* sKeyColTabID = "ColorTable ID";


VolumeRenderScalarField::VolumeRenderScalarField()
    : transferfunc_( 0 )
    , voldata_( 0 )
    , root_( new SoGroup )
    , dummytexture_( 255 )
    , indexcache_( 0 )
    , ownsindexcache_( true )
    , datacache_( 0 )
    , ownsdatacache_( true )
    , sz0_( 1 )
    , sz1_( 1 )
    , sz2_( 1 )
    , blendcolor_( Color::White() )
    , useshading_( true )
{
    root_->ref();

    useshading_ = Settings::common().isTrue( "dTect.Use VolRen shading" );
    if ( !useshading_ )
	SetEnvVar( "CVR_DISABLE_PALETTED_FRAGPROG", "1" );
}


VolumeRenderScalarField::~VolumeRenderScalarField()
{
    if ( ownsindexcache_ ) delete [] indexcache_;
    if ( ownsdatacache_ ) delete datacache_;
    root_->unref();
}


bool VolumeRenderScalarField::turnOn( bool yn )
{
    if ( !voldata_ ) return false;
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
    if ( !voldata_ ) 
	return false;

    SbVec3s size;
    void* ptr;
    SoVolumeData::DataType dt;
    
    return voldata_->getVolumeData(size,ptr,dt) && ptr==indexcache_;
}


void VolumeRenderScalarField::setScalarField( const Array3D<float>* sc,
					      bool mine, TaskRunner* tr )
{
    if ( !sc )
    {
	turnOn( false );
	if ( ownsdatacache_ ) delete datacache_;
	datacache_ = 0;
	return;
    }

    const bool isresize = sc->info().getSize(0)!=sz0_ ||
			  sc->info().getSize(1)!=sz1_ ||
			  sc->info().getSize(2)!=sz2_;

    const od_int64 totalsz = sc->info().getTotalSz();

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

    ownsdatacache_ = mine;
    datacache_ = sc->getStorage();
    if ( !datacache_ || !datacache_->arr() )
    {
	MultiArrayValueSeries<float,float>* myvalser =
	    new MultiArrayValueSeries<float,float>( totalsz );
	if ( !myvalser || !myvalser->isOK() )
	    delete myvalser;
	else
	{
	    sc->getAll( *myvalser );

	    datacache_ = myvalser;
	    ownsdatacache_ = true;
	}
    }

    //TODO: if 8-bit data & some flags, use data itself
    if ( mapper_.setup_.type_!=ColTab::MapperSetup::Fixed )
	mapper_.setData( datacache_, totalsz, tr );
    
    makeIndices( doset, tr );
}


void VolumeRenderScalarField::setColTabSequence( const ColTab::Sequence& s,
						 TaskRunner* tr )
{
    sequence_ = s;
    makeColorTables();
}


const ColTab::Mapper& VolumeRenderScalarField::getColTabMapper()
{ return mapper_; }


const ColTab::Sequence& VolumeRenderScalarField::getColTabSequence()
{ return sequence_; }


void VolumeRenderScalarField::setColTabMapperSetup(const ColTab::MapperSetup& m,
					           TaskRunner* tr )
{
    mapper_.setup_ = m;
    makeIndices( false, tr );
}


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
    if ( !voldata_ )
	return;

    const SbBox3f size( x.start, y.start, z.start, x.stop, y.stop, z.stop );
    voldata_->setVolumeSize( size );
}


Interval<float> VolumeRenderScalarField::getVolumeSize( int dim ) const
{
     const SbBox3f size = voldata_->getVolumeSize();
     return Interval<float>( size.getMin()[dim], size.getMax()[dim] );
}


SoNode* VolumeRenderScalarField::getInventorNode()
{
    if ( !voldata_ )
    {
	voldata_ = new SoVolumeData;
	root_->addChild( voldata_ );

	setVolumeSize( Interval<float>(-0.5,0.5), Interval<float>(-0.5,0.5),
		       Interval<float>(-0.5,0.5) );
	voldata_->setVolumeData( SbVec3s(1,1,1),
	    		    &dummytexture_, SoVolumeData::UNSIGNED_BYTE );
	if ( GetEnvVarYN("DTECT_VOLREN_NO_PALETTED_TEXTURE") )
	    voldata_->usePalettedTexture = FALSE;

	transferfunc_ = new SoTransferFunction;
	makeColorTables();

	root_->addChild( transferfunc_ );
    }

    return root_;
}


void VolumeRenderScalarField::makeColorTables()
{
    if ( !transferfunc_ )
	return;

    const float redfactor = (float) blendcolor_.r()/(255*255);
    const float greenfactor = (float) blendcolor_.g()/(255*255);
    const float bluefactor = (float) blendcolor_.b()/(255*255);
    const float opacityfactor = (float) (255-blendcolor_.t())/(255*255);

    const bool didnotify = transferfunc_->colorMap.enableNotify( false );
    int cti = 0;
    for ( int idx=0; idx<mNrColors; idx++ )
    {
	const float relval = ((float) idx)/(mNrColors-2);
	const ::Color col = sequence_.color( relval );
	transferfunc_->colorMap.set1Value( cti++, col.r()*redfactor );
	transferfunc_->colorMap.set1Value( cti++, col.g()*greenfactor );
	transferfunc_->colorMap.set1Value( cti++, col.b()*bluefactor );
	transferfunc_->colorMap.set1Value( cti++, 1.0-col.t()*opacityfactor );
    }

    transferfunc_->predefColorMap = SoTransferFunction::NONE;

    transferfunc_->colorMap.enableNotify(didnotify);
    transferfunc_->colorMap.touch();
}


void VolumeRenderScalarField::makeIndices( bool doset, TaskRunner* tr )
{
    if ( !datacache_ )
	return;

    const od_int64 totalsz = sz0_*sz1_*sz2_;

    if ( !indexcache_ )
    {
	indexcache_ = new unsigned char[totalsz];
	ownsindexcache_ = true;
    }

    ColTab::MapperTask<unsigned char> indexer( mapper_, totalsz,
	mNrColors, *datacache_, indexcache_ );

    if ( (tr&&!tr->execute( indexer ) ) || !indexer.execute() )
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


int VolumeRenderScalarField::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res != 1 ) return res;

    int coltabid;
    if ( !par.get( sKeyColTabID, coltabid ) ) return -1;
    RefMan<DataObject> dataobj = DM().getObject( coltabid );
    if ( !dataobj ) return 0;
    mDynamicCastGet(VisColorTab*,coltab,dataobj.ptr());
    if ( !coltab ) return -1;

    setColTabMapperSetup( coltab->colorMapper().setup_, 0 );
    setColTabSequence( coltab->colorSeq().colors(), 0 );

    return 1;
}


} // namespace visBase
