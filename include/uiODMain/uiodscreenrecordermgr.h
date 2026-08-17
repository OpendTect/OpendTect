/** @file include/uiODMain/uiodscreenrecordermgr.h */

#pragma once

/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "callback.h"
#include "uistring.h"

class uiODMain;
class uiODScreenRecorderDlg;


/*!\brief Owns the OD screen recorder user interface and recording state. */

mExpClass(uiODMain) uiODScreenRecorderMgr : public CallBacker
{ mODTextTranslationClass(uiODScreenRecorderMgr);
public:

    enum class RecorderState
    {
        Idle,
        Starting,
        Recording,
        Finalizing
    };

    uiODScreenRecorderMgr(uiODMain&);
    ~uiODScreenRecorderMgr();

    mOD_DisableCopy(uiODScreenRecorderMgr)

    void toggleRecording();
    void finishBeforeExit();

    inline RecorderState getRecorderState() const { return state_; }

    inline bool isRecording() const
    {
	return state_ == RecorderState::Recording;
    }

    inline const BufferString& lastOutputFile() const
    {
         return lastoutputfile_; 
        }

    Notifier<uiODScreenRecorderMgr> stateChanged;
    Notifier<uiODScreenRecorderMgr> recordingFinished;

protected:

    class RecorderEngine;

    void showDialog();
    void setState(RecorderState,const uiString&);

    void startCB(CallBacker*);
    void stopCB(CallBacker*);
    void refreshSourcesCB(CallBacker*);
    void sourceTypeChangedCB(CallBacker*);
    void beforeExitCB(CallBacker*);

    void recordingStarted();
    void recordingDone(const BufferString&);
    void recordingFailed(const uiString&);
    void recordingCancelled();
    void durationChanged(od_int64);

////////////////////////////////////DATA SECTION///////////////////////////////

    uiODMain& appl_;
    uiODScreenRecorderDlg* dialog_ {nullptr};
    RecorderEngine* engine_ {nullptr};
    RecorderState state_ {RecorderState::Idle};
    BufferString lastoutputfile_;
    bool exiting_ {false};
};
