/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          April 2013
________________________________________________________________________

-*/

#include "vistexturepanelstrip.h"
#include "vistexturechannels.h"
#include "odversion.h"

#include <osgGeo/LayeredTexture>
#include <osgGeo/TexturePanelStrip>
#include <osg/Geometry>

mCreateFactoryEntry( visBase::TexturePanelStrip );

namespace visBase
{

TexturePanelStrip::TexturePanelStrip()
    : VisualObjectImpl( false )
    , osgpanelstrip_( new osgGeo::TexturePanelStripNode )
    , pathcoords_( new TypeSet<Coord> )
    , pathtexoffsets_( new TypeSet<float> )
{
    osgpanelstrip_->ref();
    addChild( osgpanelstrip_ );
    // osgpanelstrip_->toggleTilingMode();	// for testing purposes only
}


TexturePanelStrip::~TexturePanelStrip()
{
    osgpanelstrip_->unref();
}


void TexturePanelStrip::setTextureChannels( visBase::TextureChannels* channels )
{
    channels_ = channels;
    osgpanelstrip_->setTexture( channels_->getOsgTexture() );

    const int maxtexsize = channels_->getOsgTexture()->maxTextureSize();
    osgpanelstrip_->setTextureBrickSize( maxtexsize, false );
}


visBase::TextureChannels* TexturePanelStrip::getTextureChannels()
{ return channels_; }


void TexturePanelStrip::freezeDisplay( bool yn )
{ osgpanelstrip_->freezeDisplay( yn ); }


bool TexturePanelStrip::isDisplayFrozen() const
{ return osgpanelstrip_->isDisplayFrozen(); }


void TexturePanelStrip::updatePath()
{
    if ( !pathcoords_ )
	return;

    osg::ref_ptr<osg::Vec2Array> osgpath =
				    new osg::Vec2Array( pathcoords_->size() );

    for ( int idx=0; idx<osgpath->size(); idx++ )
    {
	Coord3 dummy( (*pathcoords_)[idx], 0.0 );
	mVisTrans::transform( displaytrans_, dummy );
	(*osgpath)[idx] = osg::Vec2( dummy.x, dummy.y );
    }

    osgpanelstrip_->setPath( *osgpath );
}


void TexturePanelStrip::setPath( const TypeSet<Coord>& coords )
{
    *pathcoords_ = coords;
    updatePath();
}


const TypeSet<Coord>& TexturePanelStrip::getPath() const
{ return *pathcoords_; }


void TexturePanelStrip::setPath2TextureMapping( const TypeSet<float>& offsets )
{
    *pathtexoffsets_ = offsets;

    osg::ref_ptr<osg::FloatArray> osgmapping =
					new osg::FloatArray( offsets.size() );

    for ( int idx=0; idx<osgmapping->size(); idx++ )
	(*osgmapping)[idx] = offsets[idx];

    osgpanelstrip_->setPath2TextureMapping( *osgmapping );
}


const TypeSet<float>& TexturePanelStrip::getPath2TextureMapping() const
{ return *pathtexoffsets_; }


void TexturePanelStrip::setPathTextureShift( float shift, int startidx )
{ osgpanelstrip_->setPathTextureShift( shift, startidx ); }


float TexturePanelStrip::getPathTextureShift() const
{ return osgpanelstrip_->getPathTextureShift(); }


float TexturePanelStrip::getPathTextureShiftStartIdx() const
{ return osgpanelstrip_->getPathTextureShiftStartIdx(); }


void TexturePanelStrip::setZRange( const Interval<float>& zrange )
{
    Coord3 topdummy( Coord(), zrange.start );
    mVisTrans::transform( displaytrans_, topdummy );
    Coord3 bottomdummy( Coord(), zrange.stop );
    mVisTrans::transform( displaytrans_, bottomdummy );

    osgpanelstrip_->setZRange( topdummy.z, bottomdummy.z );
}


Interval<float> TexturePanelStrip::getZRange() const
{
    Coord3 topdummy( Coord(), osgpanelstrip_->getTop() );
    mVisTrans::transformBack( displaytrans_, topdummy );
    Coord3 bottomdummy( Coord(), osgpanelstrip_->getBottom() );
    mVisTrans::transformBack( displaytrans_, bottomdummy );

    return Interval<float>(topdummy.z,bottomdummy.z);
}


void TexturePanelStrip::setZRange2TextureMapping(
					    const Interval<float>& offsets )
{
    osgpanelstrip_->setZRange2TextureMapping(true, offsets.start, offsets.stop);
}


void TexturePanelStrip::unsetZRange2TextureMapping()
{ osgpanelstrip_->setZRange2TextureMapping( false ); }


bool TexturePanelStrip::isZRange2TextureMappingSet() const
{ return osgpanelstrip_->isZRange2TextureMappingSet(); }


Interval<float> TexturePanelStrip::getZRange2TextureMapping() const
{
    return Interval<float>( osgpanelstrip_->getTopTextureMapping(),
			    osgpanelstrip_->getBottomTextureMapping() );
}


void TexturePanelStrip::setZTextureShift( float shift )
{ osgpanelstrip_->setZTextureShift( shift ); }


float TexturePanelStrip::getZTextureShift() const
{ return osgpanelstrip_->getZTextureShift(); }


void TexturePanelStrip::swapTextureAxes( bool yn )
{ osgpanelstrip_->swapTextureAxes( yn ); }


bool TexturePanelStrip::areTextureAxesSwapped() const
{ return osgpanelstrip_->areTextureAxesSwapped(); }


void TexturePanelStrip::smoothNormals( bool yn )
{ osgpanelstrip_->smoothNormals( yn ); }


bool TexturePanelStrip::areNormalsSmoothed() const
{ return osgpanelstrip_->areNormalsSmoothed(); }


void TexturePanelStrip::setDisplayTransformation( const mVisTrans* tr )
{
    Coord3 dummy( 0.0, 0.0, 1.0 );
    mVisTrans::transformDir( displaytrans_, dummy );
    if ( fabs(dummy.normalize().z) < 1.0 )
    {
	pErrMsg( "Display transformation violates assumed orthogonality "
		 "between xy and z." );
    }

    Interval<float> zrange = getZRange();
    displaytrans_ = tr;
    setZRange( zrange );
    updatePath();
}


const mVisTrans* TexturePanelStrip::getDisplayTransformation() const
{ return displaytrans_; }


int TexturePanelStrip::getNrTextures() const
{
    const std::vector<osg::Geometry*>& geometries =
	osgpanelstrip_->getGeometries();
    return geometries.size();

}


bool TexturePanelStrip::getTextureDataInfo( int tidx,
	TextureDataInfo& texinfo ) const
{
    texinfo.setEmpty();
    const std::vector<osg::Geometry*>& geometries =
	osgpanelstrip_->getGeometries();

    if ( tidx>=geometries.size() )
	return false;

    const osg::Array* coords = geometries[tidx]->getVertexArray();
    const osg::Vec3Array* vtxcoords =
	dynamic_cast<const osg::Vec3Array*>(coords);

    const osg::PrimitiveSet* ps = geometries[tidx]->getPrimitiveSet(0);

    if ( !vtxcoords || !ps ) return false;

    for ( int idx=0; idx<vtxcoords->size(); idx++ )
    {
	texinfo.coords_ += Coord3( vtxcoords->at(idx)[0], vtxcoords->at(idx)[1],
	    vtxcoords->at(idx)[2] );
    }

    for ( int idx=0; idx<ps->getNumIndices(); idx++ )
	texinfo.ps_ += ps->index( idx );

       return calcTextureCoordinates( tidx, texinfo.texcoords_ );

}


bool TexturePanelStrip::getTextureInfo( int& width, int& height, int& pixsize )
{
    const osg::Image* image = osgpanelstrip_->getCompositeTextureImage();
    if ( !image ) return false;

    width = image->s();
    height = image->t();
    pixsize = image->getPixelSizeInBits() / 8;

    return true;
}


bool TexturePanelStrip::calcTextureCoordinates( int geomidx,
						TypeSet<Coord>& coords ) const
{
    coords.setEmpty();
    osg::ref_ptr<const osg::Vec2Array> osgcoords =
			osgpanelstrip_->getCompositeTextureCoords( geomidx );
    if ( !osgcoords.valid() )
	return false;

    for ( int idx=0; idx<osgcoords->size(); idx++ )
	coords += Conv::to<Coord>( osgcoords->at(idx) );

    return true;
}


const unsigned char* TexturePanelStrip::getTextureData() const
{
    const osg::Image* image = osgpanelstrip_->getCompositeTextureImage();
    return image ? image->data() : 0;
}


bool TexturePanelStrip::calcTextureCoordinates( TypeSet<Coord3>& coordin,
	TypeSet<Coord>& coordout ) const
{
    return true;
}


Coord TexturePanelStrip::getImageTextureCoord( const Coord3 dispcrd,
    double minx, double xlen, double minz, double zlen ) const
{
    return Coord( 0, 0 );

}


void TexturePanelStrip::getMinAndLengthInfo( double& minval, double& len,
    bool bz ) const
{
}

} // namespace visBase
