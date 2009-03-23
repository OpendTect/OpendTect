#ifndef uivolprocvolreader_h
#define uivolprocvolreader_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		November 2008
 RCS:		$Id: uivolprocvolreader.h,v 1.3 2009-03-23 11:02:00 cvsbert Exp $
________________________________________________________________________

-*/

#include "uivolprocchain.h"

class uiSeisSel;
class CtxtIOObj;

namespace VolProc
{

class Step;
class VolumeReader;

mClass uiVolumeReader : public uiStepDialog
{
public:
   static void			initClass();
   				~uiVolumeReader();
				
				uiVolumeReader(uiParent*,VolumeReader*);

protected:

    static uiStepDialog*	create(uiParent*, Step*);
    void			volSel(CallBacker*);
    void			updateFlds(CallBacker*);
    bool			acceptOK(CallBacker*);

    VolumeReader*		volumereader_;

    uiSeisSel*			seissel_;
    CtxtIOObj*			ctio_;

};

}; //namespace

#endif
