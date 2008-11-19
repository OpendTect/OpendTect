#ifndef uivolprocvolreader_h
#define uivolprocvolreader_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		November 2008
 RCS:		$Id: uivolprocvolreader.h,v 1.1 2008-11-19 15:01:57 cvskris Exp $
________________________________________________________________________

-*/

#include "uivolprocchain.h"

class uiSeisSel;
class CtxtIOObj;

namespace VolProc
{

class Step;
class VolumeReader;

class uiVolumeReader : public uiStepDialog
{
public:
   static void			initClass();
   				~uiVolumeReader();
				
				uiVolumeReader(uiParent*,VolumeReader*);

protected:
    static uiStepDialog*	create(uiParent*, Step*);
    bool			acceptOK(CallBacker*);
    void			updateFlds(CallBacker*);

    VolumeReader*		volumereader_;

    uiSeisSel*			seissel_;
    CtxtIOObj*			seisctxt_;
};

}; //namespace

#endif
