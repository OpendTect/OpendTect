/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/

#include "vismaterial.h"
#include "visosg.h"
#include "iopar.h"
#include "paralleltask.h"
#include "visdata.h"

#include <osg/Material>
#include <osg/Array>
#include <osg/Geometry>

namespace visBase
{

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
    , diffuseintensity_( 0.8 )
    , change( this )
    , colorbindtype_( 1 )
    , transparencybendpower_ ( 1.0 )
{
    material_->ref();
    setColorMode( Off );
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
    if ( mat.osgcolorarray_ )
    {
	setColorArray(
	mGetOsgVec4Arr(mat.osgcolorarray_->clone(osg::CopyOp::DEEP_COPY_ALL)) );
    }

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
    mSetProp( color_ );
    updateOsgMaterial();

    lckr.unlockNow();

    if ( trigger ) change.trigger();
}


void Material::setColors( const TypeSet<Color>& colors, bool trigger )
{
    //Loop backwards to allocate the memory at first call
    for ( int idx=colors.size()-1; idx>=0; idx-- )
        setColor( colors[idx], idx, trigger );
}


void Material::setColor( const Color& n, int idx, bool trigger )
{
    mGetWriteLock( lckr );

    if ( idx < 0 )
    {
	color_ = n;
	updateOsgMaterial();
	lckr.unlockNow();
	if ( trigger ) change.trigger();
	return;
    }

    if ( !osgcolorarray_ )
	setColorArray( new osg::Vec4Array(idx+1) );

    osg::Vec4Array* colarr = mGetOsgVec4Arr(osgcolorarray_);

    if ( colarr )
    {
	if ( colarr->size()<=idx )
	    colarr->resizeArray( idx+1 );

	(*colarr)[idx] = osg::Vec4f( n.rF(), n.gF(), n.bF(), 1.0f-n.tF() );
    }

    lckr.unlockNow();
    if ( trigger ) change.trigger();
}


void Material::setColorArray( osg::Array* ptr )
{
    unRefOsgPtr( osgcolorarray_ );
    osgcolorarray_ = ptr;
    refOsgPtr( osgcolorarray_ );

    for ( int idx=0; idx<attachedgeoms_.size(); idx++ )
	attachGeometry( attachedgeoms_[idx] );
}


Color Material::getColor( int idx ) const
{
    mGetReadLock( lckr );

    if ( !osgcolorarray_ )
	return color_;

    const osg::Vec4Array* colarr = mGetOsgVec4Arr(osgcolorarray_);

    if ( !colarr || idx >= colarr->size() )
	return Color( 0, 0, 0 );

    return Color( Conv::to<Color>( (*colarr)[idx] )  );
}


void Material::removeColor( int idx )
{
    mGetWriteLock( lckr );
    removeOsgColor( idx );
}


void Material::removeOsgColor( int idx )
{
    osg::Vec4Array* colarr = mGetOsgVec4Arr( osgcolorarray_ );
    if ( !colarr || colarr->size()<=idx )
    {
	pErrMsg("Removing invalid index or last color.");
	return;
    }

    colarr->erase( colarr->begin() + idx );
}


void Material::setTransparency( float n, int idx, bool update )
{
    mGetWriteLock( lckr );
    if ( !osgcolorarray_ )
    {
	color_.setTransparencyF( n );
	updateOsgMaterial();
    }

    if ( osgcolorarray_ )
    {
	osg::Vec4Array* colarr = mGetOsgVec4Arr(osgcolorarray_);
	if ( idx >= colarr->size() )
	    return;

	(*colarr)[idx].a() = 1.0f-n;
	colarr->dirty();

	if ( update )
	{
	    for ( int idy=0; idy<attachedgeoms_.size(); idy++ )
		attachedgeoms_[idy]->dirtyDisplayList();
	}
    }

    lckr.unlockNow();
    change.trigger();
}


void Material::setAllTransparencies( float n )
{
    if ( !osgcolorarray_ )
    {
	setTransparency( n );
	return;
    }

    for ( int idx=0; idx<osgcolorarray_->getNumElements(); idx++ )
	setTransparency( n, idx, true );

    visBase::DataObject::requestSingleRedraw();
}


void Material::setTransparencies( float n, const Interval<int>& range )
{
    for ( int idx=range.start; idx<=range.stop; idx++ )
	setTransparency( n, idx, true );

    visBase::DataObject::requestSingleRedraw();
}


float Material::getTransparency( int idx ) const
{
   if ( !osgcolorarray_ )
       return color_.tF();

   osg::Vec4Array* colarr = mGetOsgVec4Arr( osgcolorarray_ );
   return idx < colarr->size() ? (1.0f-(*colarr)[idx].a()) : 0.0f;
}


const TypeSet<Color> Material::getColors()
{
    mGetReadLock( lckr );

    TypeSet<Color> colors;
    if ( osgcolorarray_ )
    {
	mDefParallelCalc2Pars( ColorUpdator, tr("Updating colors"),
			       osg::Vec4Array&, osgclrs,
			       TypeSet<Color>&, colors )
	mDefParallelCalcBody
	(
	    ,
		colors_[idx] = Conv::to<Color>( osgclrs_[idx] );
	    ,
	)

	osg::Vec4Array* colarr = mGetOsgVec4Arr( osgcolorarray_ );
	const int sz = colarr->size();
        colors.setSize( sz );
	ColorUpdator updator( sz, *colarr, colors );
	updator.executeParallel( true );
    }
    else
	colors += color_;

    return colors;
}


#define mSetGetProperty( Type, func, var ) \
void Material::set##func( Type n ) \
{ \
    mGetReadLock( lck ); \
    var = n; \
    updateOsgMaterial(); \
    lck.unlockNow();\
    change.trigger();\
} \
Type Material::get##func() const \
{ \
    return var; \
}


mSetGetProperty( float, Ambience, ambience_ );
mSetGetProperty( float, DiffIntensity, diffuseintensity_ );
mSetGetProperty( float, SpecIntensity, specularintensity_ );
mSetGetProperty( float, EmmIntensity, emmissiveintensity_ );
mSetGetProperty( float, Shininess, shininess_ );


void Material::rescaleTransparency( float bendpower )
{
    transparencybendpower_ = fabs( bendpower );
}


float Material::getRescaledTransparency() const
{
   float res = getTransparency( 0 );
   if ( transparencybendpower_ == 1.0 )
       return res;

   return pow( res, transparencybendpower_ );
}


#define mGetOsgCol( col, fac, transp ) \
    osg::Vec4( col.r()*fac/255, col.g()*fac/255, col.b()*fac/255, 1.0-transp )

void Material::updateOsgMaterial()
{
    if ( !osgcolorarray_  )
    {
	const osg::Vec4 diffuse = mGetOsgCol( color_ , diffuseintensity_,
					      getRescaledTransparency() );

	const float transparency0 = getRescaledTransparency();

	material_->setAmbient( osg::Material::FRONT_AND_BACK,
		mGetOsgCol(color_,ambience_,transparency0) );
	material_->setSpecular( osg::Material::FRONT_AND_BACK,
		mGetOsgCol(color_,specularintensity_,transparency0) );
	material_->setEmission( osg::Material::FRONT_AND_BACK,
		mGetOsgCol(color_,emmissiveintensity_,transparency0) );

	material_->setShininess(osg::Material::FRONT_AND_BACK, shininess_ );
	material_->setDiffuse(osg::Material::FRONT_AND_BACK, diffuse );
    }

    visBase::DataObject::requestSingleRedraw();
}


int Material::nrOfMaterial() const
{
    mGetReadLock( lckr );
    if ( osgcolorarray_ )
	return osgcolorarray_->getNumElements();

    return 1;
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


void Material::clear()
{
    mGetWriteLock( lckr );
    if ( osgcolorarray_ )
        mGetOsgVec4Arr( osgcolorarray_ )->clear();
}


}; // namespace visBase
