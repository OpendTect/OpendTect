#ifndef viscamerainfo_h
#define viscamerainfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscamerainfo.h,v 1.5 2009-07-22 16:01:24 cvsbert Exp $
________________________________________________________________________


-*/

#include "visdata.h"

class SoCameraInfo;

namespace visBase
{


/*!\brief

*/

mClass CameraInfo : public DataObject
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

