#ifndef visforegroundlifter_h
#define visforegroundlifter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		Feb 2007
 RCS:		$Id: visforegroundlifter.h,v 1.3 2009-07-22 16:01:24 cvsbert Exp $
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

