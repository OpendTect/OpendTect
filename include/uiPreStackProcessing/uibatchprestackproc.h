#ifndef uibatchprestackproc_h
#define uibatchprestackproc_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		March 2009
 RCS:		$Id: uibatchprestackproc.h,v 1.1 2009-03-16 16:31:14 cvskris Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"


class CtxtIOObj;
class uiSeisSel;
class uiPosSubSel;

namespace PreStack
{
class uiProcSel;

/*!Dialog to setup a time->depth conversion for volumes on disk. */

mClass uiBatchProcSetup : public uiFullBatchDialog
{
public:
    		uiBatchProcSetup(uiParent*,bool is2d );
		~uiBatchProcSetup();

protected:

    bool		fillPar(IOPar&);
    bool		prepareProcessing();

    uiProcSel*		chainsel_;

    CtxtIOObj&		inputctxt_;
    uiSeisSel*		inputsel_;

    uiPosSubSel*	possubsel_;

    CtxtIOObj&		outputctxt_;
    uiSeisSel*		outputsel_;
    bool		is2d_;
};

}; //namespace


#endif
