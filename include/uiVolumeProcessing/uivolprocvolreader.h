#ifndef uivolprocvolreader_h
#define uivolprocvolreader_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		November 2008
 RCS:		$Id: uivolprocvolreader.h,v 1.6 2012-08-03 13:01:20 cvskris Exp $
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uivolprocchain.h"

#include "volprocvolreader.h"

class uiSeisSel;
class CtxtIOObj;

namespace VolProc
{

mClass(uiVolumeProcessing) uiVolumeReader : public uiStepDialog
{
public:
    mDefaultFactoryInstanciationBase(
	    VolProc::VolumeReader::sFactoryKeyword(),
	    VolProc::VolumeReader::sFactoryDisplayName())
    mDefaultFactoryInitClassImpl( uiStepDialog, createInstance );

				

protected:
				uiVolumeReader(uiParent*,VolumeReader*);
   				~uiVolumeReader();
    static uiStepDialog*	createInstance(uiParent*, Step*);

    void			volSel(CallBacker*);
    void			updateFlds(CallBacker*);
    bool			acceptOK(CallBacker*);

    VolumeReader*		volumereader_;

    uiSeisSel*			seissel_;
    CtxtIOObj*			ctio_;

};

}; //namespace

#endif

