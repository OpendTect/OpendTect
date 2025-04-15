#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"

#include "uimainwin.h"
#include "uistrings.h"

#include "helpview.h"
#include "od_helpids.h"

class uiButton;

/*!\brief Stand-alone dialog window with optional 'OK', 'Cancel' and
'Save defaults' button.

It is meant to be the base class for 'normal' dialog windows. The Setup class
allows specification of several properties, like window title and the title
text on the dialog itself. The help ID is linked to a WindowLinkTable.txt.
If you don't want to use the help system, simply pass null ('0').

*/


#define mNoDlgTitle	uiString::empty()
#define mTODOHelpKey	HelpKey( nullptr, ::toString(-1) )
#define mNoHelpKey	HelpKey::emptyHelpKey()


mExpClass(uiBase) uiDialog : public uiMainWin
{
    friend class	uiDialogBody;

public:

    /*!\brief description of properties of dialog. */

    mExpClass(uiBase) Setup
    {
    public:
			Setup(const uiString& window_title,
			      const uiString& dialog_title,
			      const HelpKey& help_key);
			Setup(const uiString& window_title,
			      const HelpKey& help_key);
			~Setup();

	mDefSetupMemb(uiString,wintitle)
	mDefSetupMembInit(uiString,dlgtitle,mNoDlgTitle)
	mDefSetupMemb(HelpKey,helpkey)
	mDefSetupMembInit(HelpKey,videokey,HelpKey::emptyHelpKey())
	mDefSetupMembInit(uiString,savetext,uiStrings::sSaveAsDefault())
	mDefSetupMembInit(uiString,oktext,uiStrings::sOk())
	mDefSetupMembInit(uiString,canceltext,uiStrings::sCancel())
	mDefSetupMembInit(uiString,applytext,uiStrings::sApply())
	mDefSetupMembInit(bool,modal,true) //!< if no parent, always non-modal
	mDefSetupMembInit(bool,applybutton,false)
	mDefSetupMembInit(bool,savebutton,false)
	mDefSetupMembInit(bool,savebutispush,false)
	mDefSetupMembInit(bool,separator,true)
	mDefSetupMembInit(bool,savechecked,false)
	mDefSetupMembInit(bool,menubar,false)
	mDefSetupMembInit(bool,mainwidgcentered,false)
	mDefSetupMembInit(bool,fixedsize,false)
	mDefSetupMembInit(bool,okcancelrev,false) //!< used in wizards
	mDefSetupMembInit(int,nrstatusflds,0)
	    //!< nrstatusflds == -1: Make a statusbar, but don't add msg fields.

	private:
		    Setup(const char*,const char*,int)	    = delete;
			//!< Makes sure you cannot use '0' for help ID.
			//!< Use mTODOHelpKey or mNoHelpKey instead

    };

    enum		Button { OK, CANCEL, APPLY, HELP, CREDITS, SAVE };

			uiDialog(uiParent*,const Setup&);
			~uiDialog();
			mOD_DisableCopy(uiDialog)

    const Setup&	setup() const;

    int			go();
    int			goMinimized();

    void		reject(CallBacker* cb=0);
    void		accept(CallBacker* cb=0);
    enum DoneResult	{ Rejected=0, Accepted=1 };
    void		done(DoneResult);

    void		setHSpacing(int);
    void		setVSpacing(int);
    void		setBorder(int);

    void		setModal(bool yn);
    bool		isModal() const;

    uiButton*		button(Button);
    const uiButton*	button(Button) const;
    void		setButtonText( Button but, const uiString& txt );

    enum CtrlStyle	{ OkAndCancel, RunAndClose, CloseOnly };
			//! On construction, it's (of course) OkAndCancel
    void		setCtrlStyle(CtrlStyle);
			//! OK button disabled when set to CloseOnly
    CtrlStyle		getCtrlStyle() const		{ return ctrlstyle_; }
    void		setOkCancelText(const uiString& ok,const uiString& cnl);
    void		setOkText(const uiString&);
			//! Cancel button disabled when set to empty
    void		setCancelText(const uiString&);
			//! Save button enabled when set to non-empty
    void		enableSaveButton(
			    const uiString& txt=uiStrings::sSaveAsDefault());
    int			uiResult() const;
				//!< -1 running, otherwise (int)DoneResult

    void		setButtonSensitive(Button,bool);
    void		setSaveButtonChecked(bool);
    void		setTitleText(const uiString& txt);
    bool		isButtonSensitive(Button) const;
    bool		hasSaveButton() const;
    bool		saveButtonChecked() const;

    void		setSeparator(bool yn);
			//!< Separator between central dialog and OK/Cancel bar?
    bool		separator() const;
    void		setHelpKey(const HelpKey&);
    virtual HelpKey	helpKey() const;
    void		setVideoKey(const HelpKey&);
    void		setVideoKey(const HelpKey&,int idx);
    HelpKey		videoKey(int idx=0) const;
    int			nrVideos() const;
    void		removeVideo(int);

    enum TitlePos	{ LeftSide, CenterWin, RightSide };
    static TitlePos	titlePos();
    static void		setTitlePos(TitlePos);

    uiGroup*		getDlgGroup();

    Notifier<uiDialog>	applyPushed;

protected:

    virtual bool	rejectOK(CallBacker*){ return true;}//!< confirm reject
    virtual bool	acceptOK(CallBacker*){ return true;}//!< confirm accept
    virtual bool	doneOK(int)	     { return true; } //!< confirm exit

    bool		cancelpushed_;
    CtrlStyle		ctrlstyle_;
    static TitlePos	titlepos_;

};
