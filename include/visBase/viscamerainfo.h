#ifndef viscamerainfo_h
#define viscamerainfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscamerainfo.h,v 1.1 2003-09-02 12:39:59 kristofer Exp $
________________________________________________________________________


-*/

#include "vissceneobj.h"

class SoCameraInfo;

namespace visBase
{


/*!\brief

*/

class CameraInfo : public SceneObject
{
public:
    static CameraInfo*	create()
			mCreateDataObj(CameraInfo);

    void		setInteractive(bool yn);
    bool		isInteractive() const;

    void		setMoving(bool yn);
    bool		isMoving() const;

    SoNode*		getData();

protected:
    			~CameraInfo();

    SoCameraInfo*	camerainfo;
};

};

#endif

