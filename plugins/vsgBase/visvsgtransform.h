#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgdatagroup.h"

#include "position.h"

#include <vsg/maths/mat4.h>
#include <vsg/maths/quat.h>
#include <vsg/maths/vec3.h>

namespace vsg { class AbsoluteTransform; class MatrixTransform; }

namespace dGBVis
{

#define mDGBVisDefTransType( func, tp ) \
    void func( tp& ) const; \
    void func( const tp& f, tp& t ) const; \
    static void func( const Transformation* tr, tp& v ) \
    { if ( tr ) tr->func( v ); } \
    static void func( const Transformation* tr, const tp& f, tp& t );

#define mDGBVisDefTrans( tp ) \
mDGBVisDefTransType( transform, tp ) \
mDGBVisDefTransType( transformBack, tp ) \
mDGBVisDefTransType( transformDir, tp ) \
mDGBVisDefTransType( transformBackDir, tp ) \
mDGBVisDefTransType( transformSize, tp ) \
mDGBVisDefTransType( transformBackSize, tp ) \
mDGBVisDefTransType( transformNormal, tp ) \
mDGBVisDefTransType( transformBackNormal, tp )

#define mDGBVisDefConvTransType( func, frtp, totp ) \
    void func( const frtp&, totp& ) const; \
    static void func( const Transformation* tr, const frtp& f, totp& t );

#define mDGBVisDefConvTrans( frtp, totp ) \
mDGBVisDefConvTransType( transform, frtp, totp ) \
mDGBVisDefConvTransType( transformBack, frtp, totp ) \
mDGBVisDefConvTransType( transformDir, frtp, totp ) \
mDGBVisDefConvTransType( transformBackDir, frtp, totp ) \
mDGBVisDefConvTransType( transformSize, frtp, totp ) \
mDGBVisDefConvTransType( transformBackSize, frtp, totp ) \
mDGBVisDefConvTransType( transformNormal, frtp, totp ) \
mDGBVisDefConvTransType( transformBackNormal, frtp, totp )


/*!\brief Counterpart of visBase::Transformation.

  Used in two ways: as a group node (a vsg::MatrixTransform) that
  transforms everything below it on the GPU, and as a CPU transform for
  display transformations.

  The transformation is denoted Aq=b, where A is the matrix, q the column
  vector { x, y, z, 1 } and b the transformed vector { x'', y'', z'', m }.
  The output coordinates are x' = x''/m; y' = y''/m; z' = z''/m.
*/

mExpClass(vsgBase) Transformation : public DataObjectGroup
{
public:
    static RefMan<Transformation> create();
			mCreateDGBVisObj(Transformation);

    void		reset();

    void		setA(double a11,double a12,double a13,double a14,
			     double a21,double a22,double a23,double a24,
			     double a31,double a32,double a33,double a34,
			     double a41,double a42,double a43,double a44 );

    void		setMatrix(const Coord3& trans,
				  const Coord3& rotvec,double rotangle,
				  const Coord3& scale);

    void		setTranslation(const Coord3&);
    void		setRotation(const Coord3& vec,double angle);
    void		setScale(const Coord3&);
    void		setScaleOrientation(const Coord3& vec,double angle);

    Coord3		getTranslation() const;
    Coord3		getScale() const;
    void		getRotation(Coord3& vec,double& angle) const;

    void		setAbsoluteReferenceFrame();
    const vsg::dmat4&	getMatrix() const	{ return matrix_; }
    const vsg::dmat4&	getInverseMatrix() const { return invmatrix_; }

			mDGBVisDefTrans( Coord3 )
			mDGBVisDefTrans( vsg::dvec3 )
			mDGBVisDefTrans( vsg::vec3 )
			mDGBVisDefConvTrans( Coord3, vsg::dvec3 )
			mDGBVisDefConvTrans( Coord3, vsg::vec3 )
			mDGBVisDefConvTrans( vsg::dvec3, Coord3 )
			mDGBVisDefConvTrans( vsg::vec3, Coord3 )

    Transformation&	operator*=(const Transformation&);

    Notifier<Transformation> transformchange;
			/*!<Objects using this as display transformation
			    must recompute their vertices. */

private:
			~Transformation();

    enum class Kind	{ Point, Dir, Size, Normal };
    vsg::dvec3		doTransform(const vsg::dvec3&,bool back,Kind) const;

    void		updateMatrix();
    void		setMatrixInternal(const vsg::dmat4&);

    vsg::MatrixTransform*	mtnode_ = nullptr;
    vsg::AbsoluteTransform*	absnode_ = nullptr;

    vsg::dmat4		matrix_;
    vsg::dmat4		invmatrix_;
    vsg::dvec3		curtrans_;
    vsg::dvec3		curscale_;
    vsg::dquat		currot_;
    vsg::dquat		curso_;
};

} // namespace dGBVis

#undef mDGBVisDefTransType
#undef mDGBVisDefTrans
#undef mDGBVisDefConvTransType
#undef mDGBVisDefConvTrans
