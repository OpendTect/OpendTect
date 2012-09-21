#ifndef cmddriverdlg_h
#define cmddriverdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          October 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "uidialog.h"

class uiCheckBox;
class uiFileInput;
class uiTextEdit;
class uiLabel;
class uiLabeledComboBox;
class uiPushButton;
class BufferString;


namespace CmdDrive
{
   
class CmdDriver;
class CmdRecorder;
class InteractSpec;


mClass(uiCmdDriver) uiCmdInteractDlg : public uiDialog
{
public:
				uiCmdInteractDlg(uiParent*,const InteractSpec&);

    bool			unHide() const		{ return unhide_; }

protected:
    bool			rejectOK(CallBacker*);
    uiTextEdit*			infofld_;
    uiLabel*			resumelbl_;
    bool			unhide_;
};


mClass(uiCmdDriver) uiCmdDriverDlg : public uiDialog
{
public:
				uiCmdDriverDlg(uiParent*,CmdDriver&,
					       CmdRecorder&);
				~uiCmdDriverDlg();

    void			popUp();
    void			autoStartGo( const char* fnm);
    void			executeFinished();
    void			beforeSurveyChg();
    void			afterSurveyChg();

protected:

    void			selChgCB(CallBacker*);
    bool			selectGoCB(CallBacker*);
    void			selectAbortCB(CallBacker*);
    void			selectPauseCB(CallBacker*);
    bool			selectStartRecordCB(CallBacker*);
    void			selectStopRecordCB(CallBacker*);
    void			interactCB(CallBacker*);
    bool			rejectOK(CallBacker*);
    void			interactClosedCB(CallBacker*);
    void			toolTipChangeCB(CallBacker*);

    void			refreshDisplay(bool runmode,bool idle);
    void			setDefaultSelDirs();
    void			setDefaultLogFile();

    uiFileInput*                inpfld_;
    uiFileInput*                outfld_;
    uiFileInput*                logfld_;

    bool			inpfldsurveycheck_;
    bool			outfldsurveycheck_;
    bool			logfldsurveycheck_;
    BufferString		logproposal_;

    uiLabeledComboBox*          cmdoptionfld_;
    uiPushButton*		gobut_;
    uiPushButton*		abortbut_;
    uiPushButton*               pausebut_;
    uiPushButton*               startbut_;
    uiPushButton*               stopbut_;
    uiCheckBox*			tooltipfld_;
    CmdDriver&			drv_;
    CmdRecorder&		rec_;

    uiCmdInteractDlg*		interactdlg_;
};


}; // namespace CmdDrive


#endif

