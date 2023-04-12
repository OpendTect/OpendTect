/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vistopbotimage.h"
#include "vismaterial.h"
#include "vistransform.h"

#include "filespec.h"
#include "imagedeftr.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "odimage.h"
#include "settingsaccess.h"

#include <osgGeo/TexturePlane>
#include <osgGeo/LayeredTexture>
#include <osg/Image>
#include <osgDB/ReadFile>

mCreateFactoryEntry( visBase::TopBotImage )

namespace visBase
{

const char* TopBotImage::sKeyTopLeftCoord()		{ return "TopLeft"; }
const char*	TopBotImage::sKeyBottomRightCoord()	{ return "BotRight"; }


TopBotImage::TopBotImage()
    : VisualObjectImpl(true)
    , trans_(nullptr)
    , laytex_( new osgGeo::LayeredTexture )
    , texplane_( new osgGeo::TexturePlaneNode )
{
    laytex_->ref();
    texplane_->ref();
    layerid_ = laytex_->addDataLayer();
    laytex_->addProcess( new osgGeo::IdentityLayerProcess(*laytex_, layerid_) );
    laytex_->allowShaders( SettingsAccess().doesUserWantShading(false) );
    texplane_->setLayeredTexture( laytex_ );
    addChild( texplane_ );

    setTransparency( 0.0 );
}


TopBotImage::~TopBotImage()
{
    if ( trans_ ) trans_->unRef();
    laytex_->unref();
    texplane_->unref();
}


void TopBotImage::setImageID( const MultiID& id )
{
    odimageid_ = id;
    PtrMan<IOObj> ioobj = IOM().get( id );
    ImageDef def;
    ODImageDefTranslator::readDef( def, *ioobj );
    setRGBImageFromFile( def.filename_ );
    const Coord3 tlcrd( def.tlcoord_.coord(), topLeft().z );
    const Coord3 brcrd( def.brcoord_.coord(), bottomRight().z );
    setPos( tlcrd, brcrd );
}


void TopBotImage::setPos( const Coord3& c1, const Coord3& c2 )
{
    pos0_ = c1;
    pos1_ = c2;
    updateCoords();
}


void TopBotImage::updateCoords()
{
    Coord3 pos0, pos1;
    Transformation::transform( trans_, pos0_, pos0 );
    Transformation::transform( trans_, pos1_, pos1 );

    const Coord3 dif = pos1 - pos0;
    texplane_->setWidth( osg::Vec3(dif.x, -dif.y, 0.0) );

    const Coord3 avg = 0.5 * (pos0+pos1);
    texplane_->setCenter( osg::Vec3(avg.x, avg.y, avg.z) );
}


void TopBotImage::setDisplayTransformation( const mVisTrans* trans )
{
    if ( trans_ ) trans_->unRef();
    trans_ = trans;
    if ( trans_ ) trans_->ref();
    updateCoords();
}


const mVisTrans* TopBotImage::getDisplayTransformation() const
{
    return trans_;
}


void TopBotImage::setTransparency( float val )
{ getMaterial()->setTransparency( val ); }


float TopBotImage::getTransparency() const
{ return getMaterial()->getTransparency(); }


void TopBotImage::setImageFilename( const char* fnm )
{
    filenm_ = fnm;
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile( fnm );
    laytex_->setDataLayerImage( layerid_, image );
    texplane_->setTextureBrickSize( laytex_->maxTextureSize() );
}


const char* TopBotImage::getImageFilename() const
{ return filenm_.buf(); }


void TopBotImage::setRGBImageFromFile( const char* fnm )
{
    filenm_ = fnm;
    uiString errmsg;
    PtrMan<OD::RGBImage> rgbimg =
			    OD::RGBImageLoader::loadRGBImage(filenm_,errmsg);
    if ( !rgbimg )
    {
	pErrMsg( errmsg.getFullString() );
	return;
    }

    setRGBImage( *rgbimg );
}


void TopBotImage::setRGBImage( const OD::RGBImage& rgbimg )
{
    const int totsz = rgbimg.getSize(true) * rgbimg.getSize(false) * 4;
    unsigned char* imgdata = new unsigned char[totsz];
    OD::memCopy( imgdata, rgbimg.getData(), totsz );
    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->setImage( rgbimg.getSize(true), rgbimg.getSize(false), 1,
		     GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE, imgdata,
		     osg::Image::NO_DELETE );
    image->flipVertical();
    laytex_->setDataLayerImage( layerid_, image );
    texplane_->setTextureBrickSize( laytex_->maxTextureSize() );
}


bool TopBotImage::getImageInfo( int& width, int& height, int& pixsz ) const
{
    const osg::Image* image = laytex_ ? laytex_->getCompositeTextureImage()
				      : nullptr;
    if ( !image )
	return false;

    width = image->s();
    height = image->t();
    pixsz = image->getPixelSizeInBits() / 8;
    return true;
}


unsigned const char* TopBotImage::getTextureData() const
{
    const osg::Image* image = laytex_ ? laytex_->getCompositeTextureImage()
				      : nullptr;
    return image ? image->data() : nullptr;
}


bool TopBotImage::getTextureDataInfo( TypeSet<Coord3>& coords,
				      TypeSet<Coord>& texcoords,
				      TypeSet<int>& ps ) const
{
    const std::vector<osg::Geometry*>& geometries = texplane_->getGeometries();
    Interval<float> xrg( mUdf(float), -mUdf(float) );
    Interval<float> yrg( mUdf(float), -mUdf(float) );
    Interval<float> zrg( mUdf(float), -mUdf(float) );
    Interval<float> texxrg( mUdf(float), -mUdf(float) );
    Interval<float> texyrg( mUdf(float), -mUdf(float) );
    for ( unsigned int gidx=0; gidx<geometries.size(); gidx++ )
    {
	const osg::Array* vertarr = geometries[gidx]->getVertexArray();
	const osg::Vec3Array* vertcoords =
			dynamic_cast<const osg::Vec3Array*>( vertarr );
	if ( !vertcoords )
	    continue;

	for ( unsigned int idx=0; idx<vertcoords->size(); idx++ )
	{
	    xrg.include( vertcoords->at(idx)[0], false );
	    yrg.include( vertcoords->at(idx)[1], false );
	    zrg.include( vertcoords->at(idx)[2], false );
	}

	osg::ref_ptr<const osg::Vec2Array> osgcoords =
			    texplane_->getCompositeTextureCoords( gidx );
	if ( !osgcoords.valid() )
	    continue;

	for ( unsigned int idx=0; idx<osgcoords->size(); idx++ )
	{
	    texxrg.include( osgcoords->at(idx)[0], false );
	    texyrg.include( osgcoords->at(idx)[1], false );
	}
    }

    coords.setEmpty();
    Coord3 crd; crd.z = sCast(double,zrg.start);
    crd.setXY( xrg.start, yrg.start ); coords += crd;
    crd.setXY( xrg.stop, yrg.start ); coords += crd;
    crd.setXY( xrg.stop, yrg.stop ); coords += crd;
    crd.setXY( xrg.start, yrg.stop ); coords += crd;

    texcoords.setEmpty();
    Coord texcrd;
    texcrd.setXY( texxrg.start, texyrg.start ); texcoords += texcrd;
    texcrd.setXY( texxrg.stop, texyrg.start ); texcoords += texcrd;
    texcrd.setXY( texxrg.stop, texyrg.stop ); texcoords += texcrd;
    texcrd.setXY( texxrg.start, texyrg.stop ); texcoords += texcrd;

    ps.setEmpty();
    for ( int idx=0; idx<4; idx++ )
	ps += idx;

    return true;
}


void TopBotImage::fillPar( IOPar& iopar ) const
{
    VisualObjectImpl::fillPar( iopar );

    iopar.set( sKeyTopLeftCoord(), pos0_ );
    iopar.set( sKeyBottomRightCoord(), pos1_ );
    iopar.set( sKey::Transparency(), getTransparency() );

    FileSpec fs( filenm_.buf() );
    fs.makePathsRelative();
    iopar.set( sKey::FileName(), fs.fileName() );
}


bool TopBotImage::usePar( const IOPar& iopar )
{
    VisualObjectImpl::usePar( iopar );

    Coord3 ltpos;
    Coord3 brpos;
    float transparency = 0;
    iopar.get( sKeyTopLeftCoord(), ltpos );
    iopar.get( sKeyBottomRightCoord(), brpos );
    iopar.get( sKey::Transparency(), transparency );

    filenm_.setEmpty();
    BufferString relfnm;
    iopar.get( sKey::FileName(), relfnm  );
    if ( !relfnm.isEmpty() )
    {
	const FileSpec fs( relfnm );
	filenm_ = fs.absFileName();
    }

    setPos( ltpos, brpos );
    setRGBImageFromFile( filenm_ );
    setTransparency( transparency );
    return true;
}

} // namespace visBase
