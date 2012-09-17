#ifndef uibatchprestackproc_h
#define uibatchprestackproc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		March 2009
 RCS:		$Id: uibatchprestackproc.h,v 1.3 2011/10/25 09:19:26 cvskris Exp $
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
    void		outputNameChangeCB(CallBacker*);

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
