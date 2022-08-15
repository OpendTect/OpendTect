#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visdatagroup.h"
#include "position.h"

namespace osg { class MatrixTransform; class Vec3d; class Vec3f; class Quat; }


namespace visBase
{
#define mDefTransType( func, tp ) \
    void func( tp& ) const; \
    void func( const tp& f, tp& t ) const; \
    static void func( const Transformation* tr, tp& v ) \
    { if ( tr ) tr->func( v ); } \
    static void func( const Transformation* tr, const tp& f, tp& t );

#define mDefTrans( tp ) \
mDefTransType( transform, tp ); \
mDefTransType( transformBack, tp ); \
mDefTransType( transformDir, tp ); \
mDefTransType( transformBackDir, tp ); \
mDefTransType( transformSize, tp ); \
mDefTransType( transformBackSize, tp ); \
mDefTransType( transformNormal, tp ); \
mDefTransType( transformBackNormal, tp );


#define mDefConvTransType( func, frtp, totp ) \
void func( const frtp&, totp& ) const; \
static void func( const Transformation* tr, const frtp& f, totp& t);

#define mDefConvTrans( frtp, totp ) \
mDefConvTransType( transform, frtp, totp ); \
mDefConvTransType( transformBack, frtp, totp ); \
mDefConvTransType( transformDir, frtp, totp ); \
mDefConvTransType( transformBackDir, frtp, totp ); \
mDefConvTransType( transformSize, frtp, totp ); \
mDefConvTransType( transformBackSize, frtp, totp ); \
mDefConvTransType( transformNormal, frtp, totp ); \
mDefConvTransType( transformBackNormal, frtp, totp );


/*! \brief
The Transformation is an object that transforms everything following the
node.

The transformation is denoted:

Aq=b

Where A is the transformation matrix, q is a column vector with { x, y, z, 1 }
and b is the transformed column vector { x'', y'', z'', m }. The output coords
can be calculated by:

x' = x''/m; y' = y''/m; z'=z''/m;

*/


mExpClass(visBase) Transformation : public DataObjectGroup
{
public:
    static Transformation*	create()
				mCreateDataObj(Transformation);

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
    const osg::MatrixTransform* getTransformNode() const { return node_; };

			mDefTrans( Coord3 );
			mDefTrans( osg::Vec3d );
			mDefTrans( osg::Vec3f );
    			mDefConvTrans( Coord3, osg::Vec3d );
    			mDefConvTrans( Coord3, osg::Vec3f );
    			mDefConvTrans( osg::Vec3d, Coord3 );
    			mDefConvTrans( osg::Vec3f, Coord3 );

    Transformation&     operator*=(const Transformation&);
    
private:

    virtual		~Transformation();

    void		updateMatrix();
    void		updateNormalizationMode();

    osg::MatrixTransform* node_;

    osg::Vec3d&		curscale_;
    osg::Vec3d&		curtrans_;
    osg::Quat&		currot_;
    osg::Quat&		curso_;
};

} // namespace visBase

