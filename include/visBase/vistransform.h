#ifndef vistransform_h
#define vistransform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id$
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visdatagroup.h"
#include "position.h"

namespace osg { class MatrixTransform; class Vec3d; }

class SoMatrixTransform;
class SbMatrix;
class SoRotation;
class SbVec3f;
class Quaternion;

namespace visBase
{
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


mClass(visBase) Transformation : public DataObjectGroup
{
public:
    static Transformation*	create()
				mCreateDataObj(Transformation);

    void		setRotation(const Coord3& vec,double angle);
    void		setTranslation( const Coord3& );
    Coord3		getTranslation() const;

    void		setScale( const Coord3& );
    Coord3		getScale() const;

    void		reset();

    void		setA( double a11, double a12, double a13, double a14,
	    		      double a21, double a22, double a23, double a24,
			      double a31, double a32, double a33, double a34,
			      double a41, double a42, double a43, double a44 );

    Coord3		transform( const Coord3& ) const;
    Coord3		transformBack(  const Coord3& ) const;
    void		transform( SbVec3f& ) const;
    void		transformBack( SbVec3f& ) const;
    void		transform(osg::Vec3d&) const;
    void		transformBack(osg::Vec3d&) const;

    Coord3		transformDir(const Coord3&) const;
    Coord3		transformDirBack(const Coord3&) const;

    virtual void	fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int		usePar( const IOPar& );
    
private:

    virtual		~Transformation();
    void		ensureGroup();

    SoGroup*		transformgroup_;
    SoMatrixTransform*	transform_;

    osg::MatrixTransform* node_;

    static const char*	matrixstr();

    virtual SoNode*	gtInvntrNode();
};


/*!Rotation of following objects in 3d.*/



mClass(visBase) Rotation : public DataObject
{
public:
    static Rotation*	create()
			mCreateDataObj(Rotation);

    void		set(const Coord3& vec,double angle);
    void		set(const Quaternion&);
    void		get(Quaternion&) const;

    Coord3		transform(const Coord3&) const;
    Coord3		transformBack(const Coord3&) const;

private:
    virtual		~Rotation();

    SoRotation*		rotation_;

    virtual SoNode*	gtInvntrNode();

};

}

#endif

