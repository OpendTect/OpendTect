#ifndef uimergeseis_h
#define uimergeseis_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          April 2002
 RCS:           $Id: uiseismmproc.h,v 1.21 2004-03-03 11:07:41 nanne Exp $
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
class uiCheckBox;
class uiGenInput;
class SeisMMJobMan;
class uiProgressBar;
class uiFileBrowser;
class uiIOFileSelect;
class uiLabeledListBox;
class uiLabeledComboBox;


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
    BufferString	caption;	
    Timer&		tim;
    IOParList&		iopl;
    int			delay;
    int			nrcyclesdone;
    int			estmbs;

    uiLabeledListBox*	avmachfld;
    uiLabeledListBox*	usedmachfld;
    uiLabeledComboBox*	jrppolselfld;
    uiButton*		addbut;
    uiButton*		stopbut;
    uiButton*		vwlogbut;
    uiCheckBox*		autorembut;
    uiCheckBox*		detectbut;
    uiIOFileSelect*	tmpstordirfld;
    uiTextEdit*		progrfld;
    uiFileBrowser*	logvwer;
    uiGroup*		machgrp;
    uiGenInput*		jrpstartfld;
    uiGenInput*		jrpstopfld;
    BufferString	jrpstarttime;
    BufferString	jrpstoptime;
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
    void		jrpSel(CallBacker*);
    void		startStopUpd(CallBacker*);

    bool		autoRemaining() const;
    bool		pauseJobs() const;
};

#endif
