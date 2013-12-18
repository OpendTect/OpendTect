/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vismaterial.h"
#include "visosg.h"
#include "iopar.h"
#include "paralleltask.h"

#include <osg/Material>
#include <osg/Array>
#include <osg/Geometry>

namespace visBase
{


class OsgColorArrayUpdator: public ParallelTask
{
public:
    OsgColorArrayUpdator(Material* p, const od_int64 size );
    od_int64	totalNr() const { return totalnrcolors_; }

protected:
    bool	doWork(od_int64 start, od_int64 stop, int);
    bool	doPrepare(int);
    od_int64	nrIterations() const { return totalnrcolors_; }

private:
    Material* material_;
    Threads::Atomic<od_int64>	totalnrcolors_;

};


OsgColorArrayUpdator::OsgColorArrayUpdator( Material* p, const od_int64 size )
    : material_( p )
    , totalnrcolors_( size )
{
}

bool OsgColorArrayUpdator::doPrepare(int)
{
    if ( material_->osgcolorarray_->getNumElements()<totalnrcolors_ )
	return false;

    return true;
}


bool OsgColorArrayUpdator::doWork(od_int64 start,od_int64 stop,int)
{
    for ( int idx = mCast(int,start); idx<=mCast(int,stop); idx++ )
	material_->updateOsgColor( idx );

    return true;
}


const char* Material::sKeyColor()		{ return "Color"; }
const char* Material::sKeyAmbience()		{ return "Ambient Intensity"; }
const char* Material::sKeyDiffIntensity()	{ return "Diffuse Intensity"; }
const char* Material::sKeySpectralIntensity()	{ return "Specular Intensity"; }
const char* Material::sKeyEmmissiveIntensity()	{ return "Emmissive Intensity";}
const char* Material::sKeyShininess()		{ return "Shininess"; }
const char* Material::sKeyTransparency()	{ return "Transparency"; }


#define mGetWriteLock(nm) \
    Threads::Locker nm( lock_, Threads::Locker::WriteLock )

#define mGetReadLock(nm) \
    Threads::Locker nm( lock_, Threads::Locker::ReadLock )


Material::Material()
    : material_( addAttribute(new osg::Material) )
    , osgcolorarray_( 0 )
    , ambience_( 0.8 )
    , specularintensity_( 0 )
    , emmissiveintensity_( 0 )
    , shininess_( 0 )
    , change( this )
    , colorbindtype_( 1 )
{
    material_->ref();
    setColorMode( Off );
    setMinNrOfMaterials(0);
}


Material::~Material()
{
    material_->unref();
    if ( osgcolorarray_ ) osgcolorarray_->unref();
    for ( int idx = 0; idx< attachedgeoms_.size(); idx++ )
	attachedgeoms_.removeSingle( idx );
}


#define mSetProp( prop ) prop = mat.prop
void Material::setFrom( const Material& mat, bool trigger )
{
    setColors( mat.colors_, false );
    setPropertiesFrom( mat, trigger );
}


void Material::setPropertiesFrom( const Material& mat,bool trigger )
{
    mGetWriteLock( lckr );
    mSetProp( diffuseintensity_ );
    mSetProp( ambience_ );
    mSetProp( specularintensity_ );
    mSetProp( emmissiveintensity_ );
    mSetProp( shininess_ );
    mSetProp( transparency_ );
    setMinNrOfMaterials( colors_.size()- 1, true, trigger );
}


void Material::setColors( const TypeSet<Color>& colors, bool synchronizing )
{
    mGetWriteLock( lckr );
    colors_.erase();
    colors_ = colors;
    setMinNrOfMaterials( colors_.size()-1, synchronizing );

}


void Material::synchronizingOsgColorArray( bool trigger )
{
    if ( !colors_.size() ) return;

    OsgColorArrayUpdator updator( this, colors_.size() );

    if ( updator.execute() && trigger )
    {
	change.trigger();
    }
}


void Material::setColor( const Color& n, int idx )
{
    mGetWriteLock( lckr );
    setMinNrOfMaterials( idx,true,false );
    if ( colors_[idx]==n )
	return;
    colors_[idx] = n;

    updateOsgColor( idx );

    lckr.unlockNow();
    change.trigger();

}


Color Material::getColor( int idx ) const
{
    mGetReadLock( lckr );
    if ( idx >= colors_.size() )
	return Color( 0, 0, 0 );

    if ( colors_.validIdx(idx) )
	return Color( colors_[idx] );

    return Color( colors_[0] );
}


void Material::removeColor( int idx )
{
    mGetWriteLock( lckr );

    if ( colors_.validIdx(idx) && colors_.size()>1 )
     {
	 colors_.removeSingle( idx );
	 transparency_.removeSingle( idx );
	 diffuseintensity_.removeSingle( idx );
	 removeOsgColor( idx );
     }
     else
     {
	 pErrMsg("Removing invalid index or last color.");
     }
}


void Material::removeOsgColor( int idx )
{
    osg::Vec4Array* colarr = mGetOsgVec4Arr( osgcolorarray_ );

    if ( !colarr || colarr->size()<=idx )
	return;
    colarr->erase( colarr->begin() + idx );
}


void Material::setDiffIntensity( float n, int idx )
{
    mGetWriteLock( lckr );

    setMinNrOfMaterials(idx);
    diffuseintensity_[idx] = n;
    updateOsgColor( idx );
    lckr.unlockNow();
    change.trigger();
}


float Material::getDiffIntensity( int idx ) const
{
    float diffuseintensity( 0 );

    mGetReadLock( lckr );
    if ( idx< diffuseintensity_.size() )
    {
	if ( idx>=0 && idx<diffuseintensity_.size() )
	    diffuseintensity = diffuseintensity_[idx];
	else
	    diffuseintensity = diffuseintensity_[ 0 ];

    }

    return diffuseintensity;
}


void Material::setTransparency( float n, int idx )
{
    mGetWriteLock( lckr );
    setMinNrOfMaterials(idx);

    transparency_[idx] = n;
    updateOsgColor( idx );
    lckr.unlockNow();
    change.trigger();
}

void Material::setAllTransparencies( float n )
{
    mGetWriteLock( lckr );
    transparency_.setAll( n );
    setMinNrOfMaterials( colors_.size() -1 );
}


void Material::setTransparencies( float n, const Interval<int>& range )
{
    mGetWriteLock( lckr );
    setMinNrOfMaterials(range.stop-1, false );

    for ( int idx = range.start; idx<range.stop; idx++ )
	transparency_[idx] = n;

    synchronizingOsgColorArray();
}


float Material::getTransparency( int idx ) const
{
    float transparency( 0 );

    mGetReadLock( lckr );
    if ( idx < transparency_.size() )
    {
	if ( idx>=0 && idx<transparency_.size() )
	    transparency = transparency_[idx];
	else
	    transparency = transparency_[0];

    }

    return transparency;
}


const TypeSet<Color> Material::getColors()
{
    mGetReadLock( lckr );
    return colors_;
}


#define mSetGetProperty( Type, func, var ) \
void Material::set##func( Type n ) \
{ \
    mGetReadLock( lck ); \
    var = n; \
    updateOsgColor( 0 ); \
    lck.unlockNow();\
    change.trigger();\
} \
Type Material::get##func() const \
{ \
    return var; \
}


mSetGetProperty( float, Ambience, ambience_ );
mSetGetProperty( float, SpecIntensity, specularintensity_ );
mSetGetProperty( float, EmmIntensity, emmissiveintensity_ );
mSetGetProperty( float, Shininess, shininess_ );


#define mGetOsgCol( col, fac, transp ) \
    osg::Vec4( col.r()*fac/255, col.g()*fac/255, col.b()*fac/255, 1.0-transp )


void Material::updateOsgColor( int idx )
{
    if ( !osgcolorarray_ || idx > (*mGetOsgVec4Arr(osgcolorarray_)).size() )
	return;

    const osg::Vec4 diffuse =
	mGetOsgCol( colors_[idx], diffuseintensity_[idx], transparency_[idx] );

    if ( !idx )
    {

	material_->setAmbient( osg::Material::FRONT_AND_BACK,
		mGetOsgCol(colors_[0],ambience_,transparency_[0]) );
	material_->setSpecular( osg::Material::FRONT_AND_BACK,
		mGetOsgCol(colors_[0],specularintensity_,transparency_[0]) );
	material_->setEmission( osg::Material::FRONT_AND_BACK,
		mGetOsgCol(colors_[0],emmissiveintensity_,transparency_[0]) );

	material_->setShininess(osg::Material::FRONT_AND_BACK, shininess_ );
	material_->setDiffuse(osg::Material::FRONT_AND_BACK, diffuse );

	osg::Vec4Array& colarr = *mGetOsgVec4Arr(osgcolorarray_);
	if ( colarr.size() )
	    colarr[0] = diffuse;
	 else
	    colarr.push_back( diffuse );
    }
    else
    {
	 osg::Vec4Array& colarr = *mGetOsgVec4Arr(osgcolorarray_);
	 colarr[idx] = diffuse;
    }

}


int Material::nrOfMaterial() const
{
    mGetReadLock( lckr );
    return colors_.size();
}


void Material::setMinNrOfMaterials(int minnr, bool synchronize, bool trigger)
{
    while ( colors_.size()<=minnr )
	colors_ += Color(179,179,179);

    float inidiffuseintensity( 0.8 );
    if ( diffuseintensity_.size() )
	inidiffuseintensity = diffuseintensity_[0];

    while ( diffuseintensity_.size() <= minnr )
	diffuseintensity_ += inidiffuseintensity;

    float initransparency( 0 );
    if ( transparency_.size() )
	initransparency = transparency_[0];

    while ( transparency_.size() <= minnr )
	transparency_ += initransparency;

    if ( !synchronize )
	return;

    if ( !osgcolorarray_ ||
	 ( *mGetOsgVec4Arr(osgcolorarray_) ).size() != colors_.size() )
	createOsgColorArray( colors_.size() );

    for ( int idx=0; idx<attachedgeoms_.size(); idx++ )
	attachGeometry( attachedgeoms_[idx] );

    synchronizingOsgColorArray( trigger );

}


void Material::attachGeometry( osg::Geometry* geom )
{
    if ( !geom || !osgcolorarray_ || osgcolorarray_->getNumElements() == 0 )
	return;

    const int pidx = attachedgeoms_.indexOf( geom );
    if ( pidx == -1 )
	attachedgeoms_ += geom;

    if ( osgcolorarray_ )
    {
	mGetWriteLock( lckr );
	if ( colorbindtype_ == osg::Geometry::BIND_OVERALL )
	{
	    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array(1);
	    (*colors)[0] = (*mGetOsgVec4Arr( osgcolorarray_))[0];
	    geom->setColorArray( colors );
	}
	else
	    geom->setColorArray( osgcolorarray_ );

	geom->setColorBinding((osg::Geometry::AttributeBinding)colorbindtype_ );
	geom->dirtyBound();

    }

}


void Material::detachGeometry( osg::Geometry* geom )
{
    mGetWriteLock( lckr );
    const int pidx = attachedgeoms_.indexOf( geom );
    if ( pidx == -1 ) return;
    attachedgeoms_[pidx]->setColorArray( 0 );
}


void Material::setColorBindType(unsigned int type)
{
    colorbindtype_ = type;
}


void Material::setNrOfMaterials( int nr )
{
    mGetWriteLock( lckr );
    if ( nr < colors_.size() )
    {
	for (int idx = colors_.size()-1; idx>nr; idx-- )
	    removeColor( idx );
    }
    if ( nr > colors_.size() )
    {
	setMinNrOfMaterials( nr, false );
	updateOsgColor( nr );
    }
}


int Material::usePar( const IOPar& iopar )
{
    int r,g,b;

    if ( iopar.get( sKeyColor(), r, g, b ) )
	setColor( Color( r,g,b ));

    float val;
    if ( iopar.get( sKeyAmbience(), val ))
	setAmbience( val );

    if ( iopar.get( sKeyDiffIntensity(), val ))
	setDiffIntensity( val );

    if ( iopar.get( sKeySpectralIntensity(), val ))
	setSpecIntensity( val );

    if ( iopar.get( sKeyEmmissiveIntensity(), val ))
	setEmmIntensity( val );

    if ( iopar.get( sKeyShininess(), val ))
	setShininess( val );

    if ( iopar.get( sKeyTransparency(), val ))
	setTransparency( val );

    return 1;
}


void Material::fillPar( IOPar& iopar ) const
{
    Color tmpcolor = getColor();
    iopar.set( sKeyColor(), tmpcolor.r(), tmpcolor.g(), tmpcolor.b() ) ;
    iopar.set( sKeyAmbience(), getAmbience() );
    iopar.set( sKeyDiffIntensity(), getDiffIntensity() );
    iopar.set( sKeySpectralIntensity(), getSpecIntensity() );
    iopar.set( sKeyEmmissiveIntensity(), getEmmIntensity() );
    iopar.set( sKeyShininess(), getShininess() );
    iopar.set( sKeyTransparency(), getTransparency() );
}


void Material::createOsgColorArray( int size )
{
    if ( osgcolorarray_ )
	osgcolorarray_->unref();
    osgcolorarray_ = new osg::Vec4Array( size );
    osgcolorarray_->ref();
}


void Material::setColorMode( ColorMode mode )
{
    if ( mode == Ambient )
	material_->setColorMode( osg::Material::AMBIENT );
    else if ( mode == Diffuse )
	material_->setColorMode( osg::Material::DIFFUSE );
    else if ( mode == Specular )
	material_->setColorMode( osg::Material::SPECULAR );
    else if ( mode == Emission )
	material_->setColorMode( osg::Material::EMISSION );
    else if ( mode == AmbientAndDiffuse )
	material_->setColorMode( osg::Material::AMBIENT_AND_DIFFUSE );
    else
	material_->setColorMode( osg::Material::OFF );
}


Material::ColorMode Material::getColorMode() const
{
    if ( material_->getColorMode() == osg::Material::AMBIENT )
	return Ambient;
    if ( material_->getColorMode() == osg::Material::DIFFUSE )
	return Diffuse;
    if ( material_->getColorMode() == osg::Material::SPECULAR )
	return Specular;
    if ( material_->getColorMode() == osg::Material::EMISSION )
	return Emission;
    if ( material_->getColorMode() == osg::Material::AMBIENT_AND_DIFFUSE )
	return AmbientAndDiffuse;

    return Off;
}


void	Material::clear()
{
    mGetWriteLock( lckr );
    if ( colors_.size() )
        colors_.erase();
    diffuseintensity_.erase();
    transparency_.erase();
    if ( osgcolorarray_ )
        mGetOsgVec4Arr( osgcolorarray_ )->clear();
}


}; // namespace visBase
