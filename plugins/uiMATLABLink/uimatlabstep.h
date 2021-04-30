#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		February 2013
________________________________________________________________________

-*/

#include "uimatlablinkmod.h"
#include "uivolprocstepdlg.h"
#include "filepath.h"
#include "matlabstep.h"

class uiFileInput;
class uiPushButton;
class uiTable;

namespace VolProc
{

mExpClass(uiMATLABLink) uiMatlabStep : public uiStepDialog
{ mODTextTranslationClass(uiMatlabStep);
public:
        mDefaultFactoryInstanciationBase(
		VolProc::MatlabStep::sFactoryKeyword(),
		VolProc::MatlabStep::sFactoryDisplayName())
		mDefaultFactoryInitClassImpl( uiStepDialog, createInstance );

protected:

			uiMatlabStep(uiParent*,MatlabStep*,bool is2d);
    static uiStepDialog* createInstance(uiParent*,Step*,bool is2d);

    void		fileSelCB(CallBacker*);
    void		loadCB(CallBacker*);
    bool		acceptOK(CallBacker*);

    void		fillParTable(const BufferStringSet&,
				     const BufferStringSet&);
    bool		readTable(BufferStringSet&,BufferStringSet&) const;
    static FilePath	getSODefaultDir();

    uiFileInput*	filefld_;
    uiPushButton*	loadbut_;
    uiTable*		partable_;

    bool		fileloaded_;
};

} // namespace VolProc

