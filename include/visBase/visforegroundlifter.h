#ifndef visforegroundlifter_h
#define visforegroundlifter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		Feb 2007
 RCS:		$Id: visforegroundlifter.h,v 1.1 2007-02-09 20:19:47 cvskris Exp $
________________________________________________________________________


-*/

#include "visdata.h"

class SoForegroundTranslation;

namespace visBase
{

/*!Moves the following objects towards the camera. */

class ForegroundLifter : public DataObject
{
public:
    static ForegroundLifter*	create()
				mCreateDataObj( ForegroundLifter );

    void			setLift(float);
    float			getLift() const;
    SoNode*			getInventorNode();

protected:
    				~ForegroundLifter();
    SoForegroundTranslation*	lifter_;
};

};

#endif

