#ifndef viscamera_h
#define viscamera_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscamera.h,v 1.7 2002-05-08 08:33:55 kristofer Exp $
________________________________________________________________________


-*/

#include "vissceneobj.h"

class SoPerspectiveCamera;
namespace Geometry { class Pos; };

namespace visBase
{

/*!\brief



*/

class Camera : public SceneObject
{
public:
    static Camera*	create()
			mCreateDataObj0arg( Camera );

    void		setPosition(const Geometry::Pos&);
    Geometry::Pos	position() const;

    void		pointAt(const Geometry::Pos&);
    void		pointAt(const Geometry::Pos& pos,
	    			const Geometry::Pos& upvector );
    void		setOrientation( const Geometry::Pos& dirvector,
					float angle );
    void		getOrientation( Geometry::Pos& dirvector,
					float& angle );

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
    int			usePar( const IOPar& );
    void		fillPar( IOPar&, TypeSet<int>& ) const;
protected:

    virtual		~Camera();


    SoPerspectiveCamera*	camera;

    static const char*	posstr;
    static const char*	orientationstr;
    static const char*	aspectratiostr;
    static const char*	heightanglestr;
    static const char*	neardistancestr;
    static const char*	fardistancestr;
    static const char*	focaldistancestr;
};


};


#endif
