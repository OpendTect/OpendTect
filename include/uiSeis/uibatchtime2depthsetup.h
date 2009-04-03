#ifndef uibatchtime2depthsetup_h
#define uibatchtime2depthsetup_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		Jan 2008
 RCS:		$Id: uibatchtime2depthsetup.h,v 1.2 2009-04-03 17:19:23 cvskris Exp $
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
		~uiBatchTime2DepthSetup();

protected:

    bool		fillPar(IOPar&);
    bool		prepareProcessing();

    uiVelSel*		velsel_;

    CtxtIOObj&		inputctxt_;
    uiSeisSel*		inputsel_;

    uiPosSubSel*	possubsel_;

    CtxtIOObj&		outputctxt_;
    uiSeisSel*		outputsel_;
};


#endif
