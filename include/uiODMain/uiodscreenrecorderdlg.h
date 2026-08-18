/** @file include/uiODMain/uiodscreenrecorderdlg.h */

#pragma once

/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "bufstring.h"
#include "uidialog.h"

class uiFileInput;
class uiLabeledComboBox;
class uiODMain;
class uiPushButton;
class uiStringSet;
class uiTextEdit;


/*!\brief Screen recorder controls and runtime diagnostics. */

mExpClass(uiODMain) uiODScreenRecorderDlg : public uiDialog
{ mODTextTranslationClass(uiODScreenRecorderDlg);
public:

	enum class SourceType
	{
	    Screen,
	    Window,
		Selection, // Future work, possibly, using uiRubberBand, 
				   //even though it's not a global, multi-monitor overlay.
		None		
	};

	uiODScreenRecorderDlg(uiODMain&);
	~uiODScreenRecorderDlg();
	mOD_DisableCopy(uiODScreenRecorderDlg)

	SourceType sourceType() const;
	int	sourceIndex() const;
	int	codecIndex() const;
	int	targetFrameRate() const;
	bool hasSource() const;
	BufferString outputFileName() const;

	void setOutputFileName(const char*);
	void setSources(const uiStringSet&);
	void setCodecOptions(const uiStringSet&);
	void setRecorderAvailability(bool,const uiString&);
	void setIdle(const uiString&);
	void setStarting(const uiString&);
	void setRecording(const uiString&);
	void setFinalizing(const uiString&);

	Notifier<uiODScreenRecorderDlg> startRequested;
	Notifier<uiODScreenRecorderDlg> stopRequested;
	Notifier<uiODScreenRecorderDlg> refreshSourcesRequested;
	Notifier<uiODScreenRecorderDlg> sourceTypeChanged;

protected:

	void sourceTypeCB(CallBacker*);
	void startStopCB(CallBacker*);
	void refreshSourcesCB(CallBacker*);
	void refreshDiagnosticsCB(CallBacker*);
	void copyCB(CallBacker*);
	void setActivity(bool,bool,bool,const uiString&,
				     const char*,const uiString&);
	void updateActionSensitivity();

	uiLabeledComboBox* sourcetypefld_ {nullptr};
	uiLabeledComboBox*	sourcefld_ {nullptr};
	uiLabeledComboBox*	codecfld_ {nullptr};
	uiLabeledComboBox*	frameratefld_ {nullptr};
	uiFileInput* outputfld_	{nullptr};
	uiPushButton* refreshsourcesbut_ {nullptr};
	uiPushButton* startStopButton_	{nullptr};
	uiTextEdit* diagnosticstxt_	{nullptr};
	bool stopmode_ {false};
	bool actionenabled_	{true};
	bool recorderavailable_ {false};
};
