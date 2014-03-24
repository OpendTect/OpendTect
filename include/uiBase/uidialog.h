#ifndef uidialog_h
#define uidialog_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uimainwin.h"
#include "bufstring.h"
#include "uistrings.h"
#include "helpview.h"

class uiButton;

/*!\brief Stand-alone dialog window with optional 'Ok', 'Cancel' and
'Save defaults' button.

It is meant to be the base class for 'normal' dialog windows. The Setup class
allows specification of several properties, like window title and the title
text on the dialog itself. The help ID is linked to a WindowLinkTable.txt.
If you don't want to use the help system, simply pass null ('0').


*/

#define mNoDlgTitle	""
#define mTODOHelpKey	HelpKey( 0, toString(-1) )
#define mNoHelpKey	HelpKey( "", 0 )


mExpClass(uiBase) uiDialog : public uiMainWin
{
    // impl: uimainwin.cc
    friend class	uiDialogBody;

public:

    /*!\brief description of properties of dialog. */

    mExpClass(uiBase) Setup
    {
    public:
			Setup( const uiString& window_title,
			       const uiString& dialog_title,
			       const HelpKey& help_key )
			: wintitle_(window_title)
			, dlgtitle_(dialog_title)
			, helpkey_(help_key), savetext_("Save defaults")
			, oktext_( sOk() ), canceltext_( sCancel() )
			, modal_(true) // if no parent given, always non-modal
			, applybutton_(false)
			, savebutton_(false), savebutispush_(false)
			, separator_(true), menubar_(false), nrstatusflds_(0)
			, mainwidgcentered_(false), savechecked_(false)
			, fixedsize_(false), okcancelrev_(false)
			{}

	mDefSetupMemb(uiString,wintitle)
	mDefSetupMemb(uiString,dlgtitle)
	mDefSetupMemb(HelpKey,helpkey)
	mDefSetupMemb(uiString,savetext)
	mDefSetupMemb(uiString,oktext)
	mDefSetupMemb(uiString,canceltext)
	mDefSetupMemb(bool,modal)
	mDefSetupMemb(bool,applybutton)
	mDefSetupMemb(bool,savebutton)
	mDefSetupMemb(bool,savebutispush)
	mDefSetupMemb(bool,separator)
	mDefSetupMemb(bool,savechecked)
	mDefSetupMemb(bool,menubar)
	mDefSetupMemb(bool,mainwidgcentered)
	mDefSetupMemb(bool,fixedsize)
	mDefSetupMemb(bool,okcancelrev) //!< used in wizards
	mDefSetupMemb(int,nrstatusflds)
	    //! nrstatusflds == -1: Make a statusbar, but don't add msg fields.

	private:
			Setup( const char* window_title,
			       const char* dialog_title,
			       int help_id )	{}
			//!< Makes sure you cannot use '0' for help ID.
			//!< Use mTODOHelpKey or mNoHelpKey instead

    };

    enum		Button { OK, CANCEL, APPLY, HELP, CREDITS, SAVE };

			uiDialog(uiParent*,const Setup&);
    const Setup&	setup() const;

    int			go();
    int			goMinimized();

    void		reject( CallBacker* cb =0);
    void		accept( CallBacker* cb =0);
    void		done(int ret=0);
			//!< 0=Cancel, 1=OK, other=user defined

    void		setHSpacing( int );
    void		setVSpacing( int );
    void		setBorder( int );

    void		setModal(bool yn);
    bool		isModal() const;

    uiButton*		button( Button but );
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
    void		enableSaveButton( const uiString& txt="Save defaults" );
			//! 0: cancel; 1: OK
    int			uiResult() const;

    void		setButtonSensitive(Button,bool);
    void		setSaveButtonChecked(bool);
    void		setTitleText(const uiString& txt);
    bool		hasSaveButton() const;
    bool		saveButtonChecked() const;

    void		setSeparator(bool yn);
			//!< Separator between central dialog and Ok/Cancel bar?
    bool		separator() const;
    void		setHelpKey(const HelpKey&);
    virtual HelpKey	helpKey() const;

    void		showMinMaxButtons();
    void		showAlwaysOnTop();
    static int		titlePos();
    static void		setTitlePos(int pos);
			// pos: -1 = left, 0 = center, 1 = right

    Notifier<uiDialog>	applyPushed;

protected:

    virtual bool        rejectOK(CallBacker*){ return true;}//!< confirm reject
    virtual bool        acceptOK(CallBacker*){ return true;}//!< confirm accept
    virtual bool        doneOK(int)	     { return true; } //!< confirm exit

    bool		cancelpushed_;
    CtrlStyle		ctrlstyle_;
    static int		titlepos_;

};

#endif

