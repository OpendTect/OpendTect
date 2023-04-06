#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiparentbody.h"
#include "uidialog.h"
#include "uimainwin.h"

#include "thread.h"
#include "timer.h"

#include <QEventLoop>
#include <QMainWindow>

class uiButton;
class uiCheckBox;
class uiDockWin;
class uiLabel;
class uiMenu;
class uiMenuBar;
class uiStatusBar;
class uiToolBar;
class uiToolButton;
class QWidget;


class uiMainWinBody : public uiCentralWidgetBody, public QMainWindow
{
mODTextTranslationClass(uiMainWinBody)
friend class		uiMainWin;
public:
			uiMainWinBody(uiMainWin& handle,uiParent* parnt,
				      const char* nm,bool modal);
    virtual		~uiMainWinBody();
			mOD_DisableCopy(uiMainWinBody)

    static void		getTopLevelWindows(ObjectSet<uiMainWin>&,
					   bool visibleonly);

    void		construct(int nrstatusflds,bool wantmenubar);

    uiStatusBar*	uistatusbar();
    uiMenuBar*		uimenubar();

    virtual void	polish();
    void		reDraw(bool deep);
    void		go(bool showminimized=false);
    virtual void	show()				{ doShow(); }
    virtual void	display(bool yn)		{ doDisplay(yn); }
    void		setInTray(bool yn)		{ intray_ = yn; }
    void		doSetWindowFlags(Qt::WindowFlags,bool yn);

    void		move(uiMainWin::PopupArea);
    void		move(int,int);

    void		close();
    bool		poppedUp() const		{ return poppedup_; }
    bool		touch();

    void		removeDockWin(uiDockWin*);
    void		addDockWin(uiDockWin&,uiMainWin::Dock);

    QMenu*		createPopupMenu() override;
    void		addToolBar(uiToolBar*);
    uiToolBar*		findToolBar(const char*);
    uiToolBar*		removeToolBar(uiToolBar*);
    uiMenu&		getToolbarsMenu()		{ return *toolbarsmnu_;}
    void		updateToolbarsMenu();

    const ObjectSet<uiToolBar>& toolBars() const	{ return toolbars_; }
    const ObjectSet<uiDockWin>& dockWins() const	{ return dockwins_; }

    void		setModal(bool yn);
    bool		isModal() const			{ return modal_; }

    void		activateInGUIThread(const CallBack&,bool busywait);

    bool		force_finalize_;

    static QScreen*	primaryScreen();
    QScreen*		screen(bool usetoplevel=false) const;

protected:

    const QWidget*	qwidget_() const override { return this; }
    void		finalize() override	{ finalize(false); }
    virtual void	finalize(bool trigger_finalize_start_stop);
    void		closeEvent(QCloseEvent*) override;
    bool		event(QEvent*) override;

    void		keyPressEvent(QKeyEvent*) override;
    void		resizeEvent(QResizeEvent*) override;

    void		doShow(bool minimized=false);
    void		doDisplay(bool yn);
    void		managePopupPos();


    void		renewToolbarsMenu();
    void		toggleToolbar(CallBacker*);

    void		saveSettings();
    void		readSettings();
    void		restoreDefaultState();

    bool		exitapponclose_;

    Threads::Mutex	activatemutex_;
    ObjectSet<CallBack> activatecbs_;
    int			nractivated_;

    int			eventrefnr_ = -1;

    uiStatusBar*	statusbar_;
    uiMenuBar*		menubar_;
    uiMenu*		toolbarsmnu_;

    ObjectSet<uiToolBar> toolbars_;
    ObjectSet<uiDockWin> dockwins_;
    uiString		windowtitle_;
    uiMainWin&		handle_;

private:

    QEventLoop		eventloop_;

    int			iconsz_;
    bool		modal_;

    void		popTimTick(CallBacker*);
    void		getPosForScreenMiddle(int& x,int& y);
    void		getPosForParentMiddle(int& x,int& y);
    Timer		poptimer_;
    bool		poppedup_;
    uiSize		prefsz_;
    uiPoint		prefpos_;
    bool		moved_;
    bool		createtbmenu_;
    bool		intray_ = false;

    bool		deletefrombody_;
    bool		deletefromod_;

    bool		hasguisettings_;
};



class uiDialogBody : public uiMainWinBody
{ mODTextTranslationClass(uiDialogBody)
public:
			uiDialogBody(uiDialog&,uiParent*,
				     const uiDialog::Setup&);
			~uiDialogBody();

    int			exec(bool showminimized);

    void		reject(CallBacker*);
			//!< to be called by a 'cancel' button
    void		accept(CallBacker*);
			//!< to be called by a 'ok' button
    void		done(int i);

    void		uiSetResult( int v )	{ result_ = v; }
    int			uiResult()		{ return result_; }

    void		setTitleText(const uiString& txt);
    void		setOkCancelText(const uiString&,const uiString&);
    void		setOkText(const uiString&);
			//!< OK button disabled when set to empty
    void		setCancelText(const uiString&);
			//!< Cancel button disabled when set to empty
    void		setApplyText(const uiString&);
    void		enableSaveButton(const uiString& txt);
    void		setSaveButtonChecked(bool yn);
    void		setButtonSensitive(uiDialog::Button,bool yn);
    bool		isButtonSensitive(uiDialog::Button) const;
    bool		saveButtonChecked() const;
    bool		hasSaveButton() const;
    const uiButton*	button(uiDialog::Button) const;
    uiButton*		button(uiDialog::Button);

			//! Separator between central dialog and Ok/Cancel bar?
    void		setSeparator( bool yn ) { setup_.separator_ = yn; }
    bool		separator() const	{ return setup_.separator_; }
    void		setHelpKey(const HelpKey& key) { setup_.helpkey_ = key;}
    HelpKey		helpKey() const { return setup_.helpkey_; }
    void		setVideoKey(const HelpKey&,int idx=-1);
    HelpKey		videoKey(int idx) const;
    int			nrVideos() const;
    void		removeVideo(int);

    void		setDlgGrp( uiGroup* cw )	{ dlggrp_=cw; }
    uiGroup*		getDlgGrp()			{ return dlggrp_; }

    void		setHSpacing( int spc )	{ dlggrp_->setHSpacing(spc); }
    void		setVSpacing( int spc )	{ dlggrp_->setVSpacing(spc); }
    void		setBorder( int b )	{ dlggrp_->setBorder( b ); }

    void		addChild(uiBaseObject& child) override;
    void		attachChild(ConstraintType,uiObject* child,
				    uiObject* other,int margin,
				    bool reciprocal) override;
    void		provideHelp(CallBacker*);
    void		showVideo(CallBacker*);

    const uiDialog::Setup& getSetup() const	{ return setup_; }

protected:

    void		manageChld_(uiBaseObject&,uiObjectBody&) override;
    const QWidget*	managewidg_() const override
			{
			    if ( !initing_ )
				return dlggrp_->pbody()->managewidg();
			    return uiMainWinBody::managewidg_();
			}

    int			result_;
    bool		initchildrendone_;

    uiGroup*		dlggrp_;
    uiDialog::Setup	setup_;

    uiButton*		okbut_;
    uiButton*		cnclbut_;
    uiButton*		applybut_;
    uiButton*		helpbut_;
    uiButton*		videobut_;
    uiToolButton*	creditsbut_;

    uiCheckBox*		savebutcb_;
    uiToolButton*	savebuttb_;

    uiLabel*		titlelbl_;

    TypeSet<HelpKey>	videokeys_;

    void		_done(int);

    void		finalize() override	{ finalize(false); }
    void		finalize(bool) override;
    void		closeEvent(QCloseEvent*) override;
    void		applyCB(CallBacker*);

private:

    void		initChildren();
    uiObject*		createChildren();
    void		layoutChildren(uiObject*);

    uiDialog&		dlghandle_;
};
