/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgtransform.h"

#include "visvsgdataman.h"
#include "math2.h"

#include <vsg/maths/transform.h>
#include <vsg/nodes/AbsoluteTransform.h>
#include <vsg/nodes/MatrixTransform.h>

mCreateDGBVisFactoryEntry( dGBVis::Transformation );

namespace
{

vsg::dvec3 toD( const Coord3& c )	{ return vsg::dvec3(c.x_,c.y_,c.z_); }
vsg::dvec3 toD( const vsg::dvec3& v )	{ return v; }
vsg::dvec3 toD( const vsg::vec3& v )	{ return vsg::dvec3( v ); }

template <class T> T fromD(const vsg::dvec3&);

template <>
Coord3 fromD<Coord3>( const vsg::dvec3& v )
{
    return Coord3( v.x, v.y, v.z );
}

template <>
vsg::dvec3 fromD<vsg::dvec3>( const vsg::dvec3& v )
{
    return v;
}

template <>
vsg::vec3 fromD<vsg::vec3>( const vsg::dvec3& v )
{
    return vsg::vec3( v );
}

bool isZeroRotation( const vsg::dquat& q )
{
    return q.x==0.0 && q.y==0.0 && q.z==0.0;
}

} // namespace


namespace dGBVis
{

Transformation::Transformation()
    : DataObjectGroup(vsg::MatrixTransform::create().get())
    , transformchange(this)
{
    mtnode_ = static_cast<vsg::MatrixTransform*>( vsggroup_ );
    reset();
}


Transformation::~Transformation()
{
    detachAllNotifiers();
}


void Transformation::reset()
{
    curtrans_ = vsg::dvec3( 0.0, 0.0, 0.0 );
    currot_ = curso_ = vsg::dquat();
    curscale_ = vsg::dvec3( 1.0, 1.0, 1.0 );
    setMatrixInternal( vsg::dmat4() );
}


Transformation& Transformation::operator*=( const Transformation& nt )
{
    setMatrixInternal( nt.matrix_ * matrix_ );
    vsg::decompose( matrix_, curtrans_, currot_, curscale_ );
    curso_ = vsg::dquat();
    return *this;
}


void Transformation::setA( double a11, double a12, double a13, double a14,
			   double a21, double a22, double a23, double a24,
			   double a31, double a32, double a33, double a34,
			   double a41, double a42, double a43, double a44 )
{
    // vsg::dmat4 takes columns
    const vsg::dmat4 mat( a11, a21, a31, a41,
			  a12, a22, a32, a42,
			  a13, a23, a33, a43,
			  a14, a24, a34, a44 );
    setMatrixInternal( mat );

    vsg::decompose( matrix_, curtrans_, currot_, curscale_ );
    curso_ = vsg::dquat();
}


void Transformation::updateMatrix()
{
    vsg::dmat4 mat = vsg::scale( curscale_ );
    if ( !isZeroRotation(curso_) )
	mat = vsg::rotate( curso_ ) * mat * vsg::rotate( vsg::inverse(curso_) );

    mat = vsg::translate( curtrans_ ) * vsg::rotate( currot_ ) * mat;
    setMatrixInternal( mat );
}


void Transformation::setMatrixInternal( const vsg::dmat4& mat )
{
    matrix_ = mat;
    invmatrix_ = vsg::inverse( mat );
    if ( mtnode_ )
	mtnode_->matrix = matrix_;

    if ( absnode_ )
	absnode_->matrix = matrix_;

    transformchange.trigger();
    requestSingleRedraw();
}


void Transformation::setMatrix( const Coord3& trans,
				const Coord3& rotvec, double rotangle,
				const Coord3& scale )
{
    curtrans_ = toD( trans );
    currot_ = vsg::dquat( rotangle, toD(rotvec) );
    curscale_ = toD( scale );
    curso_ = vsg::dquat();
    updateMatrix();
}


void Transformation::setTranslation( const Coord3& vec )
{
    curtrans_ = toD( vec );
    updateMatrix();
}


void Transformation::setRotation( const Coord3& vec, double angle )
{
    currot_ = vsg::dquat( angle, toD(vec) );
    updateMatrix();
}


void Transformation::getRotation( Coord3& vec, double& angle ) const
{
    const double sinhalfangle = Math::Sqrt( currot_.x*currot_.x +
					    currot_.y*currot_.y +
					    currot_.z*currot_.z );
    angle = 2.0 * Math::Atan2( sinhalfangle, currot_.w );
    if ( sinhalfangle > 0.0 )
	vec = Coord3( currot_.x/sinhalfangle, currot_.y/sinhalfangle,
		      currot_.z/sinhalfangle );
    else
	vec = Coord3( 0.0, 0.0, 1.0 );
}


void Transformation::setScale( const Coord3& vec )
{
    curscale_ = toD( vec );
    updateMatrix();
}


void Transformation::setScaleOrientation( const Coord3& vec, double angle )
{
    curso_ = vsg::dquat( angle, toD(vec) );
    updateMatrix();
}


Coord3 Transformation::getTranslation() const
{
    return fromD<Coord3>( curtrans_ );
}


Coord3 Transformation::getScale() const
{
    return fromD<Coord3>( curscale_ );
}


void Transformation::setAbsoluteReferenceFrame()
{
    if ( absnode_ )
	return;

    vsg::ref_ptr<vsg::AbsoluteTransform> absnode =
				vsg::AbsoluteTransform::create( matrix_ );
    for ( auto& child : vsggroup_->children )
	absnode->addChild( child );

    absnode_ = absnode.get();
    mtnode_ = nullptr;
    vsggroup_ = setVsgNode( absnode_ );
}


vsg::dvec3 Transformation::doTransform( const vsg::dvec3& inp, bool back,
					Kind kind ) const
{
    const bool useinverse = kind==Kind::Normal ? !back : back;
    const vsg::dmat4& mat = useinverse ? invmatrix_ : matrix_;
    vsg::dvec3 res = mat * inp;
    if ( kind != Kind::Point )
	res -= vsg::dvec3( mat[3][0], mat[3][1], mat[3][2] );

    if ( kind == Kind::Size )
	res.set( Math::Abs(res.x), Math::Abs(res.y), Math::Abs(res.z) );

    return res;
}


#define mDGBVisDeclTransType( func, tp, back, kind ) \
void Transformation::func( tp& to ) const \
{ to = fromD<tp>( doTransform(toD(to),back,Kind::kind) ); } \
void Transformation::func( const tp& inp, tp& to ) const \
{ to = fromD<tp>( doTransform(toD(inp),back,Kind::kind) ); } \
void Transformation::func( const Transformation* tr, const tp& inp, tp& to ) \
{ \
    if ( tr ) tr->func( inp, to ); \
    else to = inp; \
}

#define mDGBVisDeclTrans( tp ) \
mDGBVisDeclTransType( transform, tp, false, Point ) \
mDGBVisDeclTransType( transformBack, tp, true, Point ) \
mDGBVisDeclTransType( transformDir, tp, false, Dir ) \
mDGBVisDeclTransType( transformBackDir, tp, true, Dir ) \
mDGBVisDeclTransType( transformSize, tp, false, Size ) \
mDGBVisDeclTransType( transformBackSize, tp, true, Size ) \
mDGBVisDeclTransType( transformNormal, tp, false, Normal ) \
mDGBVisDeclTransType( transformBackNormal, tp, true, Normal )

mDGBVisDeclTrans( Coord3 )
mDGBVisDeclTrans( vsg::dvec3 )
mDGBVisDeclTrans( vsg::vec3 )


#define mDGBVisDeclConvTransType( func, frtp, totp, back, kind ) \
void Transformation::func( const frtp& inp, totp& to ) const \
{ to = fromD<totp>( doTransform(toD(inp),back,Kind::kind) ); } \
void Transformation::func( const Transformation* tr, const frtp& inp, \
			   totp& to ) \
{ \
    if ( tr ) tr->func( inp, to ); \
    else to = fromD<totp>( toD(inp) ); \
}

#define mDGBVisDeclConvTrans( frtp, totp ) \
mDGBVisDeclConvTransType( transform, frtp, totp, false, Point ) \
mDGBVisDeclConvTransType( transformBack, frtp, totp, true, Point ) \
mDGBVisDeclConvTransType( transformDir, frtp, totp, false, Dir ) \
mDGBVisDeclConvTransType( transformBackDir, frtp, totp, true, Dir ) \
mDGBVisDeclConvTransType( transformSize, frtp, totp, false, Size ) \
mDGBVisDeclConvTransType( transformBackSize, frtp, totp, true, Size ) \
mDGBVisDeclConvTransType( transformNormal, frtp, totp, false, Normal ) \
mDGBVisDeclConvTransType( transformBackNormal, frtp, totp, true, Normal )

mDGBVisDeclConvTrans( Coord3, vsg::dvec3 )
mDGBVisDeclConvTrans( Coord3, vsg::vec3 )
mDGBVisDeclConvTrans( vsg::dvec3, Coord3 )
mDGBVisDeclConvTrans( vsg::vec3, Coord3 )

} // namespace dGBVis
