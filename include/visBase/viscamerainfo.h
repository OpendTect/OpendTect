#ifndef viscamerainfo_h
#define viscamerainfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscamerainfo.h,v 1.3 2004-01-05 09:43:47 kristofer Exp $
________________________________________________________________________


-*/

#include "visdata.h"

class SoCameraInfo;

namespace visBase
{


/*!\brief

*/

class CameraInfo : public DataObject
{
public:
    static CameraInfo*	create()
			mCreateDataObj(CameraInfo);

    void		setInteractive(bool yn);
    bool		isInteractive() const;

    void		setMoving(bool yn);
    bool		isMoving() const;

    SoNode*		getInventorNode();

protected:
    			~CameraInfo();

    SoCameraInfo*	camerainfo;
};

};

#endif

