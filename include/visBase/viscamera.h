#ifndef viscamera_h
#define viscamera_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscamera.h,v 1.11 2002-11-15 08:14:32 kristofer Exp $
________________________________________________________________________


-*/

#include "vissceneobj.h"
#include "position.h"

class SoPerspectiveCamera;

namespace visBase
{

/*!\brief



*/

class Camera : public SceneObject
{
public:
    static Camera*	create()
			mCreateDataObj( Camera );

    void		setPosition(const Coord3&);
    Coord3		position() const;

    void		pointAt(const Coord3&);
    void		pointAt(const Coord3& pos,
	    			const Coord3& upvector );
    void		setOrientation( const Coord3& dirvector,
					float angle );
    void		getOrientation( Coord3& dirvector,
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

    void		setStereoAdjustment(float);
    float		getStereoAdjustment() const;

    void		setBalanceAdjustment(float);
    float		getBalanceAdjustment() const;

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
