#ifndef vistransform_h
#define vistransform_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: vistransform.h,v 1.6 2002-04-30 14:13:28 kristofer Exp $
________________________________________________________________________

-*/

#include "vissceneobj.h"
#include "geompos.h"

class SoMatrixTransform;

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


class Transformation : public SceneObject
{
public:
    static Transformation*	create()
				mCreateDataObj0arg(Transformation);

    void		setA( float a11, float a12, float a13, float a14,
	    		      float a21, float a22, float a23, float a24,
			      float a31, float a32, float a33, float a34,
			      float a41, float a42, float a43, float a44 );

    Geometry::Pos	transform( const Geometry::Pos& ) const;
    Geometry::Pos	transformBack(  const Geometry::Pos& ) const;

    virtual void	fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int		usePar( const IOPar& );
    
    SoNode*		getData();
private:
    virtual		~Transformation();


    SoMatrixTransform*	transform_;

    static const char*	matrixstr;
};

};


#endif
