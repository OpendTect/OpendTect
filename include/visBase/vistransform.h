#ifndef vistransform_h
#define vistransform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: vistransform.h,v 1.20 2011-04-28 07:00:12 cvsbert Exp $
________________________________________________________________________

-*/

#include "visdata.h"
#include "position.h"

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


mClass Transformation : public DataObject
{
public:
    static Transformation*	create()
				mCreateDataObj(Transformation);

    void		setRotation(const Coord3& vec,float angle);
    void		setTranslation( const Coord3& );
    Coord3		getTranslation() const;

    void		setScale( const Coord3& );
    Coord3		getScale() const;

    void		reset();

    void		setA( float a11, float a12, float a13, float a14,
	    		      float a21, float a22, float a23, float a24,
			      float a31, float a32, float a33, float a34,
			      float a41, float a42, float a43, float a44 );
    void		setA( const SbMatrix& );

    Coord3		transform( const Coord3& ) const;
    Coord3		transformBack(  const Coord3& ) const;
    void		transform( SbVec3f& ) const;
    void		transformBack( SbVec3f& ) const;

    virtual void	fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int		usePar( const IOPar& );
    
private:

    virtual		~Transformation();

    SoMatrixTransform*	transform_;

    static const char*	matrixstr();

    virtual SoNode*	gtInvntrNode();

};


/*!Rotation of following objects in 3d.*/



mClass Rotation : public DataObject
{
public:
    static Rotation*	create()
			mCreateDataObj(Rotation);

    void		set(const Coord3& vec,float angle);
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
