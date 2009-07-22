#ifndef uivolprocvolreader_h
#define uivolprocvolreader_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		November 2008
 RCS:		$Id: uivolprocvolreader.h,v 1.4 2009-07-22 16:01:24 cvsbert Exp $
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
