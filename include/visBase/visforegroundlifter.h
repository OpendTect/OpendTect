#ifndef visforegroundlifter_h
#define visforegroundlifter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		Feb 2007
 RCS:		$Id: visforegroundlifter.h,v 1.2 2009-01-08 10:15:41 cvsranojay Exp $
________________________________________________________________________


-*/

#include "visdata.h"

class SoForegroundTranslation;

namespace visBase
{

/*!Moves the following objects towards the camera. */

mClass ForegroundLifter : public DataObject
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

