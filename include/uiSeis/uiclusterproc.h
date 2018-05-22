#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          April 2009
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "uistring.h"

class DirList;
class TaskRunner;
class Timer;
class uiLabel;
class uiProgressBar;
class uiTextEdit;

class ClusterProc;

mExpClass(uiSeis) uiClusterProc : public uiDialog
{ mODTextTranslationClass(uiClusterProc);
public:
                        uiClusterProc(uiParent*,const IOPar& iop);
			~uiClusterProc();

    static bool		mergeOutput(const IOPar&,TaskRunner*,uiString&,
				    bool withdel=true);
    static const char*	sKeyScriptDir()		{ return "Script dir"; }
    uiString		sNrDoneText(const uiString& nrdone,
				    const uiString& totnr,
				    const uiString& nrerror);
protected:

    int			totalnr_;
    const IOPar&	pars_;
    Timer*		timer_;
    BufferString	scriptdirnm_;
    ClusterProc&	proc_;

    uiLabel*		label_;
    uiTextEdit*		msgfld_;
    uiProgressBar*	progbar_;

    void		progressCB(CallBacker*);
    bool		submitJobs();
};
