#ifndef menubutcommands_h
#define menubutcommands_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          February 2009
 RCS:           $Id$
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
    bool        isLocalEnvCommand() const       	{ return false; }    
mEndDeclCmdClass

mClass(uiCmdDriver) MenuActivator : public Activator
{
public:	
		MenuActivator(const uiMenuItem&);
    void	actCB(CallBacker*);

protected:
    uiMenuItem&	actmnuitm_;
};


mStartDeclCmdClass( uiCmdDriver, Button, UiObjectCmd )
    bool        isOpenQDlgCommand() const       	{ return true; }

protected:
    bool	actQDlgButton(const char* parstr);
mEndDeclCmdClass

mClass(uiCmdDriver) ButtonActivator : public Activator
{
public:	
		ButtonActivator(const uiButton&);
    void	actCB(CallBacker*);

protected:
    uiButton&	actbut_;
};


mStartDeclCmdClass( uiCmdDriver, Close, Command )
protected:
    bool	actCloseCurWin(const char* parstr);
mEndDeclCmdClass

mClass(uiCmdDriver) MdiAreaCloseActivator : public Activator
{
public:	
		MdiAreaCloseActivator(const uiMdiArea&,const char* winname);
    void	actCB(CallBacker*);

protected:
    uiMdiAreaWindow*	actwindow_;
};


mStartDeclCmdClass( uiCmdDriver, Show, Command )
    bool        isOpenQDlgCommand() const       	{ return false; }

protected:
    bool	actShowCurWin(const char* parstr);
mEndDeclCmdClass

mClass(uiCmdDriver) ShowActivator : public Activator
{
public:	
		ShowActivator(const uiMainWin&,int minnormax);
    void	actCB(CallBacker*);

protected:
    uiMainWin&	actmainwin_;
    int		actminnormax_;
};


mClass(uiCmdDriver) MdiAreaShowActivator : public Activator
{
public:	
		MdiAreaShowActivator(const uiMdiArea&,const char* winname,
				     int minnormax);
    void	actCB(CallBacker*);

protected:
    uiMdiAreaWindow*	actwindow_;
    int			actminnormax_;
};


mStartDeclCmdClass( uiCmdDriver, Tab, UiObjectCmd )		mEndDeclCmdClass

mClass(uiCmdDriver) TabActivator : public Activator
{
public:
		TabActivator(const uiTabBar&,int tabidx);
    void	actCB(CallBacker*);

protected:
    uiTabBar&	acttabbar_;
    int		acttabidx_;
};


mStartDeclCmdClass( uiCmdDriver, ButtonMenu, UiObjectCmd )	mEndDeclCmdClass
mStartDeclCmdClassNoEntry( uiCmdDriver,OkCancel, Command )	mEndDeclCmdClass
mStartDeclCmdClassNoAct( uiCmdDriver,Ok, OkCancelCmd )		mEndDeclCmdClass
mStartDeclCmdClassNoAct( uiCmdDriver,Cancel, OkCancelCmd )	mEndDeclCmdClass



mStartDeclCmdClass( uiCmdDriver, NrMenuItems, UiObjQuestionCmd )
    bool        isLocalEnvCommand() const               { return false; }
mEndDeclCmdClass

mStartDeclCmdClass( uiCmdDriver, IsMenuItemOn, UiObjQuestionCmd )
    bool        isLocalEnvCommand() const               { return false; }
mEndDeclCmdClass

mStartDeclCmdClass( uiCmdDriver, GetMenuItem, UiObjQuestionCmd )
    bool        isLocalEnvCommand() const               { return false; }
mEndDeclCmdClass

mStartDeclCmdClass( uiCmdDriver, IsButtonOn, UiObjQuestionCmd )		
    bool        isOpenQDlgCommand() const       	{ return true; }
protected:
    bool	actQDlgButton(const char* parstr);
mEndDeclCmdClass

mStartDeclCmdClass( uiCmdDriver, GetButton, UiObjQuestionCmd )		
    bool        isOpenQDlgCommand() const       	{ return true; }
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
    bool        isLocalEnvCommand() const               { return false; }
protected:
    bool	actIsCurWinShown(const char* parstr);
mEndDeclCmdClass


mStartDeclComposerClass( uiCmdDriver, Menu, CmdComposer, uiMenuItem )
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


}; // namespace CmdDrive

#endif

