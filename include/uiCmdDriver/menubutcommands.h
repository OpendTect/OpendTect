#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "command.h"
#include "cmdcomposer.h"

#include "uibutton.h"
#include "uimdiarea.h"
#include "uimenu.h"
#include "uitabbar.h"


namespace CmdDrive
{

mStartDeclCmdClass( uiCmdDriver, Menu, UiObjectCmd )
    bool	isLocalEnvCommand() const override	 { return false; }
mEndDeclCmdClass

mExpClass(uiCmdDriver) MenuActivator : public Activator
{
public:	
		MenuActivator(const uiAction&);
    void	actCB(CallBacker*) override;

protected:
    uiAction&	actmnuitm_;
};


mStartDeclCmdClass( uiCmdDriver, Button, UiObjectCmd )
    bool	isOpenQDlgCommand() const override		{ return true; }

protected:
    bool	actQDlgButton(const char* parstr);
mEndDeclCmdClass

mExpClass(uiCmdDriver) ButtonActivator : public Activator
{
public:	
		ButtonActivator(const uiButton&);
    void	actCB(CallBacker*) override;

protected:
    uiButton&	actbut_;
};


mStartDeclCmdClass( uiCmdDriver, Close, Command )
protected:
    bool	actCloseCurWin(const char* parstr);
mEndDeclCmdClass

mExpClass(uiCmdDriver) MdiAreaCloseActivator : public Activator
{
public:	
		MdiAreaCloseActivator(const uiMdiArea&,const char* winname);
    void	actCB(CallBacker*) override;

protected:
    uiMdiAreaWindow*	actwindow_;
};


mStartDeclCmdClass( uiCmdDriver, Show, Command )
    bool	isOpenQDlgCommand() const override	 { return false; }

protected:
    bool	actShowCurWin(const char* parstr);
mEndDeclCmdClass

mExpClass(uiCmdDriver) ShowActivator : public Activator
{
public:	
		ShowActivator(const uiMainWin&,int minnormax);
    void	actCB(CallBacker*) override;

protected:
    uiMainWin&	actmainwin_;
    int		actminnormax_;
};


mExpClass(uiCmdDriver) MdiAreaShowActivator : public Activator
{
public:	
		MdiAreaShowActivator(const uiMdiArea&,const char* winname,
				     int minnormax);
    void	actCB(CallBacker*) override;

protected:
    uiMdiAreaWindow*	actwindow_;
    int			actminnormax_;
};


mStartDeclCmdClass( uiCmdDriver, Tab, UiObjectCmd )		mEndDeclCmdClass

mExpClass(uiCmdDriver) TabActivator : public Activator
{
public:
		TabActivator(const uiTabBar&,int tabidx);
    void	actCB(CallBacker*) override;

protected:
    uiTabBar&	acttabbar_;
    int		acttabidx_;
};


mStartDeclCmdClass( uiCmdDriver, ButtonMenu, UiObjectCmd )	mEndDeclCmdClass
mStartDeclCmdClassNoEntry( uiCmdDriver,OkCancel, Command )	mEndDeclCmdClass
mStartDeclCmdClassNoAct( uiCmdDriver,Ok, OkCancelCmd )		mEndDeclCmdClass
mStartDeclCmdClassNoAct( uiCmdDriver,Cancel, OkCancelCmd )	mEndDeclCmdClass



mStartDeclCmdClass( uiCmdDriver, NrMenuItems, UiObjQuestionCmd )
    bool	isLocalEnvCommand() const override	 { return false; }
mEndDeclCmdClass

mStartDeclCmdClass( uiCmdDriver, IsMenuItemOn, UiObjQuestionCmd )
    bool	isLocalEnvCommand() const override	 { return false; }
mEndDeclCmdClass

mStartDeclCmdClass( uiCmdDriver, GetMenuItem, UiObjQuestionCmd )
    bool	isLocalEnvCommand() const override	 { return false; }
mEndDeclCmdClass

mStartDeclCmdClass( uiCmdDriver, IsButtonOn, UiObjQuestionCmd )		
    bool	isOpenQDlgCommand() const override	 { return true; }
protected:
    bool	actQDlgButton(const char* parstr);
mEndDeclCmdClass

mStartDeclCmdClass( uiCmdDriver, GetButton, UiObjQuestionCmd )		
    bool	isOpenQDlgCommand() const override	 { return true; }
protected:
    bool	actQDlgButton(const char* parstr);
mEndDeclCmdClass

mStartDeclCmdClass( uiCmdDriver, NrButtonMenuItems, UiObjQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, IsButtonMenuItemOn, UiObjQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetButtonMenuItem, UiObjQuestionCmd )
    mEndDeclCmdClass

mStartDeclCmdClass( uiCmdDriver, NrTabs, UiObjQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, CurTab, UiObjQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, IsTabOn, UiObjQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetTab, UiObjQuestionCmd )	mEndDeclCmdClass


mStartDeclCmdClass( uiCmdDriver, IsShown, UiObjQuestionCmd )
    bool	isLocalEnvCommand() const override	 { return false; }
protected:
    bool	actIsCurWinShown(const char* parstr);
mEndDeclCmdClass


mStartDeclComposerClass( uiCmdDriver, Menu, CmdComposer, uiAction )
    mEndDeclComposerClass
mStartDeclComposerClass( uiCmdDriver, Button, CmdComposer, uiButton )
    mEndDeclComposerClass
mStartDeclComposerClass( uiCmdDriver, Close, CmdComposer, uiMainWin )
    mEndDeclComposerClass
mStartDeclComposerClass( uiCmdDriver, MdiArea,CmdComposer,uiMdiArea )
    mEndDeclComposerClass
mStartDeclComposerClass( uiCmdDriver, Tab, CmdComposer, uiTabBar )
    mEndDeclComposerClass

mStartDeclComposerClass( uiCmdDriver, QMsgBoxBut, CmdComposer, uiMainWin )
mEndDeclComposerClass


} // namespace CmdDrive
