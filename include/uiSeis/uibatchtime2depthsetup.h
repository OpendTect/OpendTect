#ifndef uibatchtime2depthsetup_h
#define uibatchtime2depthsetup_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2008
 RCS:		$Id: uibatchtime2depthsetup.h,v 1.7 2010/10/25 18:52:54 cvskris Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"

class CtxtIOObj;
class uiTimeDepthBase;
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
    void		updateZRangeCB(CallBacker*) {}

    uiGenInput*		directionsel_;

    uiTimeDepthBase*	t2dfld_;
    uiTimeDepthBase*	d2tfld_;

    uiSeisSel*		inputtimesel_;
    uiSeisSel*		inputdepthsel_;

    uiPosSubSel*	possubsel_;

    uiSeisSel*		outputtimesel_;
    uiSeisSel*		outputdepthsel_;
};


#endif
