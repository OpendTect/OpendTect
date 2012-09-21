#ifndef viscamerainfo_h
#define viscamerainfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visdata.h"

class SoCameraInfo;

namespace visBase
{


/*!\brief

*/

mClass(visBase) CameraInfo : public DataObject
{
public:

    static CameraInfo*	create()
			mCreateDataObj(CameraInfo);

    void		setInteractive(bool yn);
    bool		isInteractive() const;

    void		setMoving(bool yn);
    bool		isMoving() const;

protected:

    			~CameraInfo();

    SoCameraInfo*	camerainfo;

    virtual SoNode*	gtInvntrNode();

};

};

#endif


