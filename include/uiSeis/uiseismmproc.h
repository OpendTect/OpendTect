#ifndef uimergeseis_h
#define uimergeseis_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          April 2002
 RCS:           $Id: uiseismmproc.h,v 1.16 2003-05-27 13:17:43 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class IOPar;
class IOObj;
class Timer;
class uiGroup;
class Executor;
class uiButton;
class uiSlider;
class IOParList;
class uiTextEdit;
class SeisMMJobMan;
class uiProgressBar;
class uiFileBrowser;
class uiIOFileSelect;
class uiLabeledListBox;


class uiSeisMMProc : public uiDialog
{
public:
                        uiSeisMMProc(uiParent*,const char* prognm,
				     const IOParList&);
			~uiSeisMMProc();

protected:

    SeisMMJobMan*	jm;
    Executor*		task;
    IOObj*		targetioobj;
    BufferString	rshcomm;
    BufferString	tmpstordir;
    Timer&		tim;
    IOParList&		iopl;
    int			delay;
    int			nrcyclesdone;
    int			estmbs;

    uiLabeledListBox*	avmachfld;
    uiLabeledListBox*	usedmachfld;
    uiButton*		addbut;
    uiButton*		stopbut;
    uiButton*		vwlogbut;
    uiIOFileSelect*	tmpstordirfld;
    uiTextEdit*		progrfld;
    uiFileBrowser*	logvwer;
    uiGroup*		machgrp;
    uiSlider*		nicefld;
    uiProgressBar*	progbar;

    bool		running;
    bool		finished;
    bool		jmfinished;

    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);

    Executor&		getFirstJM(const char*,const IOPar&);
    void		newJM();
    void		updateCurMachs();
    void		prepareNextCycle(int);
    void		setDataTransferrer(SeisMMJobMan*);
    bool		getCurMach(BufferString&) const;
    void		stopRunningJobs();
    void		execFinished(bool);
    void		setNiceNess();

    void		doCycle(CallBacker*);
    void		postStep(CallBacker*);
    void		addPush(CallBacker*);
    void		stopPush(CallBacker*);
    void		vwLogPush(CallBacker*);

};

#endif
