#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidialog.h"

class IOObjContext;
class uiIOObjSel;
class CtxtIOObj;

namespace VolProc
{

class VolumeReader;
class ProcessingStep;

mClass(VolProcTest) uiReader : public uiDialog
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
