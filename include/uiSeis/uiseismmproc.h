#ifndef uimergeseis_h
#define uimergeseis_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          April 2002
 RCS:           $Id: uiseismmproc.h,v 1.23 2004-10-27 14:56:39 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class IOPar;
class IOObj;
class Timer;
class Executor;
class uiSlider;
class JobRunner;
class IOParList;
class uiTextEdit;
class uiCheckBox;
class uiGenInput;
class HostDataList;
class uiProgressBar;
class uiFileBrowser;
class uiIOFileSelect;
class SeisJobExecProv;
class uiLabeledListBox;
class uiLabeledComboBox;


class uiSeisMMProc : public uiDialog
{
public:
                        uiSeisMMProc(uiParent*,const char* prognm,
				     const IOParList&);
			~uiSeisMMProc();

protected:

    HostDataList&	hdl;
    SeisJobExecProv*	jobprov;
    JobRunner*		jobrunner;
    Executor*		task;
    Timer*		timer;
    IOParList&		iopl;
    IOObj*		targetioobj;
    BufferString	progname;
    int			nrcyclesdone;

    uiLabeledListBox*	avmachfld;
    uiLabeledListBox*	usedmachfld;
    uiLabeledComboBox*	jrppolselfld;
    uiCheckBox*		autofillfld;
    uiIOFileSelect*	tmpstordirfld;
    uiTextEdit*		progrfld;
    uiFileBrowser*	logvwer;
    uiGenInput*		jrpstartfld;
    uiGenInput*		jrpstopfld;
    uiSlider*		nicefld;
    uiProgressBar*	progbar;

    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);

    void		doCycle(CallBacker*);
    void		addPush(CallBacker*);
    void		stopPush(CallBacker*);
    void		vwLogPush(CallBacker*);
    void		jrpSel(CallBacker*);

    void		startWork(CallBacker*);
    void		setNiceNess();
    void		updateAliveDisp();
};

#endif
