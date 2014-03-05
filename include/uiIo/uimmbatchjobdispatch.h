#ifndef uiseismmproc_h
#define uiseismmproc_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          April 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

class Timer;
class uiLabel;
class uiSlider;
class uiListBox;
class uiTextEdit;
class uiGenInput;
class uiComboBox;
class HostDataList;
class uiProgressBar;
class uiTextFileDlg;
namespace Batch { class JobSpec; }


mExpClass(uiIo) uiMMBatchJobDispatcher : public uiDialog
{
public:
                        uiMMBatchJobDispatcher(uiParent*,const Batch::JobSpec&);
			~uiMMBatchJobDispatcher();

protected:

    Batch::JobSpec&	jobspec_;
    HostDataList&	hdl_;
    Timer*		timer_;
    int			nrcyclesdone_;
    BufferString	basecaption_;

    uiListBox*		avmachfld_;
    uiListBox*		usedmachfld_;
    uiTextEdit*		progrfld_;
    uiTextFileDlg*	logvwer_;
    uiSlider*		nicefld_;
    uiProgressBar*	progbar_;
    uiComboBox*		jrppolselfld_;
    uiGenInput*		jrpstartfld_;
    uiGenInput*		jrpstopfld_;
    uiLabel*		jrpworklbl_;

    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);

    void		initWin(CallBacker*);
    void		doCycle(CallBacker*);
    void		addPush(CallBacker*);
    void		stopPush(CallBacker*);
    void		vwLogPush(CallBacker*);
    void		jrpSel(CallBacker*);

    void		startWork(CallBacker*);
    void		updateAliveDisp();
    bool		isPaused() const;
    const char*		curUsedMachName();

};

#endif

