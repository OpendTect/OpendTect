#ifndef uiclusterproc_h
#define uiclusterproc_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          April 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uidialog.h"

class DirList;
class IOPar;
class TaskRunner;
class Timer;
class uiLabel;
class uiProgressBar;
class uiTextEdit;

class ClusterProc;

mClass uiClusterProc : public uiDialog
{
public:
                        uiClusterProc(uiParent*,const IOPar& iop);
			~uiClusterProc();

    static bool		mergeOutput(const IOPar&,TaskRunner*,BufferString&,
	    			    bool withdel=true);
    static const char*	sKeyScriptDir()		{ return "Script dir"; }
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

#endif
