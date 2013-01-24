#ifndef uivolumereader_h
#define uivolumereader_h

/*+

_________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu 
 Date:		03-28-2007
 RCS:		$Id$
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
#endif
