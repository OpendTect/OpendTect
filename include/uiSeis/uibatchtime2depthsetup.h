#ifndef uibatchtime2depthsetup_h
#define uibatchtime2depthsetup_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		Jan 2008
 RCS:		$Id: uibatchtime2depthsetup.h,v 1.3 2009-04-05 14:52:53 cvskris Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"


class CtxtIOObj;
class uiVelSel;
class uiSeisSel;
class uiPosSubSel;

/*!Dialog to setup a time->depth conversion for volumes on disk. */

class uiBatchTime2DepthSetup : public uiFullBatchDialog
{
public:
    		uiBatchTime2DepthSetup(uiParent*);

protected:

    bool		fillPar(IOPar&);
    bool		prepareProcessing();
    void		dirChangeCB(CallBacker*);

    uiGenInput*		directionsel_;

    uiVelSel*		velsel_;

    uiSeisSel*		inputtimesel_;
    uiSeisSel*		inputdepthsel_;

    uiPosSubSel*	possubsel_;
    uiSeisSel*		outputsel_;
};


#endif
