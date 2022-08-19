#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    static bool		mergeOutput(const IOPar&,TaskRunner*,BufferString&,
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
