#ifndef uivolumereader_h
#define uivolumereader_h

/*+

_________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Y.C. Liu 
 Date:		03-28-2007
 RCS:		$Id: uivolumereader.h,v 1.1 2007-03-30 21:00:56 cvsyuancheng Exp $
_________________________________________________________________________

-*/

#include "uidialog.h"

class IOObjContext;
class uiIOObjSel;
class CtxtIOObj;

namespace VolProc
{

class VolumeReader;
class ProcessingStep;

class uiReader : public uiDialog
{

public:
    static void		initClass();
    			
			uiReader(uiParent*,VolumeReader*);
			~uiReader();
protected:
    static uiDialog*	create(uiParent*,ProcessingStep*);
    bool		acceptOK(CallBacker*);

    VolumeReader*	volreader_;

    uiIOObjSel*		uinputselfld_;
    CtxtIOObj*		iocontext_;
};


};
#endif
