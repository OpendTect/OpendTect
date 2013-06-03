#ifndef uimatlabstep_h
#define uimatlabstep_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		February 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uimatlablinkmod.h"
#include "uivolprocstepdlg.h"
#include "matlabstep.h"

class uiFileInput;
class uiPushButton;
class uiTable;

namespace VolProc
{

mExpClass(uiMatlabLink) uiMatlabStep : public uiStepDialog
{
public:
        mDefaultFactoryInstanciationBase(
		VolProc::MatlabStep::sFactoryKeyword(),
		VolProc::MatlabStep::sFactoryDisplayName())
		mDefaultFactoryInitClassImpl( uiStepDialog, createInstance );

protected:

    			uiMatlabStep(uiParent*,MatlabStep*);
    static uiStepDialog* createInstance(uiParent*,Step*);

    void		fileSelCB(CallBacker*);
    void		loadCB(CallBacker*);
    bool		acceptOK(CallBacker*);

    uiFileInput*	filefld_;
    uiPushButton*	loadbut_;
    uiTable*		partable_;
};

} // namespace VolProc

#endif
