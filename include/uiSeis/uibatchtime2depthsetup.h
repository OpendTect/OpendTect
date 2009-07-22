#ifndef uibatchtime2depthsetup_h
#define uibatchtime2depthsetup_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2008
 RCS:		$Id: uibatchtime2depthsetup.h,v 1.4 2009-07-22 16:01:22 cvsbert Exp $
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
