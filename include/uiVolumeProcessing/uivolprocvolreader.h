#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uivolprocchain.h"

#include "volprocvolreader.h"

class uiSeisSel;


namespace VolProc
{

mExpClass(uiVolumeProcessing) uiVolumeReader : public uiStepDialog
{ mODTextTranslationClass(uiVolumeReader);
public:
    mDefaultFactoryInstanciationBase(
	    VolProc::VolumeReader::sFactoryKeyword(),
	    VolProc::VolumeReader::sFactoryDisplayName())
    mDefaultFactoryInitClassImpl( uiStepDialog, createInstance );



protected:
				uiVolumeReader(uiParent*,VolumeReader*,
						bool is2d);
				~uiVolumeReader();
    static uiStepDialog*	createInstance(uiParent*, Step*,bool is2d);

    void			volSel(CallBacker*);
    void			updateFlds(CallBacker*);
    bool			acceptOK(CallBacker*) override;

    VolumeReader*		volumereader_;

    uiSeisSel*			seissel_;

};

} // namespace VolProc
