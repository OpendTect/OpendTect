#ifndef viscamera_h
#define viscamera_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscamera.h,v 1.2 2002-03-11 10:46:12 kristofer Exp $
________________________________________________________________________


-*/

#include "vissceneobj.h"

class SoPerspectiveCamera;

namespace visBase
{

/*!\brief


*/

class Camera : public SceneObject
{
public:
    static Camera*	create()
			mCreateDataObj0arg( Camera );

    void		setPosition(float,float,float);
    float		position(int dim) const;

    void		pointAt(float,float,float);

    void		setAspectRatio( float );
    float		aspectRatio() const;

    void		setHeightAngle( float );
    float		heightAngle() const;

    void		setNearDistance( float );
    float		nearDistance() const;

    void		setFarDistance( float );
    float		farDistance() const;

    void		setFocalDistance( float );
    float		focalDistance() const;

    SoNode*		getData();
protected:

    			Camera();
    virtual		~Camera();


    SoPerspectiveCamera*	camera;
};


};


#endif
