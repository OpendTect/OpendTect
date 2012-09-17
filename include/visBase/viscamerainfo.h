#ifndef viscamerainfo_h
#define viscamerainfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viscamerainfo.h,v 1.6 2011/04/28 07:00:12 cvsbert Exp $
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

protected:

    			~CameraInfo();

    SoCameraInfo*	camerainfo;

    virtual SoNode*	gtInvntrNode();

};

};

#endif

