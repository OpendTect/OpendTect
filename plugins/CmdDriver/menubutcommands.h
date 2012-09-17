#ifndef menubutcommands_h
#define menubutcommands_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          February 2009
 RCS:           $Id: menubutcommands.h,v 1.16 2011/08/03 09:30:32 cvsjaap Exp $
 ________________________________________________________________________

-*/

#include "command.h"
#include "cmdcomposer.h"

class uiButton;
class uiTabBar;
class uiMdiArea;
class uiMdiAreaWindow;


namespace CmdDrive
{

class CmdDriver;


mStartDeclCmdClass( Menu, UiObjectCmd )
    bool        isLocalEnvCommand() const       	{ return false; }    
mEndDeclCmdClass

mClass MenuActivator : public Activator
{
public:	
		MenuActivator(const uiMenuItem&);
    void	actCB(CallBacker*);

protected:
    uiMenuItem&	actmnuitm_;
};


mStartDeclCmdClass( Button, UiObjectCmd )
    bool        isOpenQDlgCommand() const       	{ return true; }

protected:
    bool	actQDlgButton(const char* parstr);
mEndDeclCmdClass

mClass ButtonActivator : public Activator
{
public:	
		ButtonActivator(const uiButton&);
    void	actCB(CallBacker*);

protected:
    uiButton&	actbut_;
};


mStartDeclCmdClass( Close, Command )
protected:
    bool	actCloseCurWin(const char* parstr);
mEndDeclCmdClass

mClass MdiAreaCloseActivator : public Activator
{
public:	
		MdiAreaCloseActivator(const uiMdiArea&,const char* winname);
    void	actCB(CallBacker*);

protected:
    uiMdiAreaWindow*	actwindow_;
};


mStartDeclCmdClass( Show, Command )
    bool        isOpenQDlgCommand() const       	{ return false; }

protected:
    bool	actShowCurWin(const char* parstr);
mEndDeclCmdClass

mClass ShowActivator : public Activator
{
public:	
		ShowActivator(const uiMainWin&,int minnormax);
    void	actCB(CallBacker*);

protected:
    uiMainWin&	actmainwin_;
    int		actminnormax_;
};


mClass MdiAreaShowActivator : public Activator
{
public:	
		MdiAreaShowActivator(const uiMdiArea&,const char* winname,
				     int minnormax);
    void	actCB(CallBacker*);

protected:
    uiMdiAreaWindow*	actwindow_;
    int			actminnormax_;
};


mStartDeclCmdClass( Tab, UiObjectCmd )			mEndDeclCmdClass

mClass TabActivator : public Activator
{
public:
		TabActivator(const uiTabBar&,int tabidx);
    void	actCB(CallBacker*);

protected:
    uiTabBar&	acttabbar_;
    int		acttabidx_;
};


mStartDeclCmdClass( ButtonMenu, UiObjectCmd )		mEndDeclCmdClass
mStartDeclCmdClass( OkCancel, Command )			mEndDeclCmdClass
mStartDeclCmdClassNoAct( Ok, OkCancelCmd )		mEndDeclCmdClass
mStartDeclCmdClassNoAct( Cancel, OkCancelCmd )		mEndDeclCmdClass



mStartDeclCmdClass( NrMenuItems, UiObjQuestionCmd )
    bool        isLocalEnvCommand() const               { return false; }
mEndDeclCmdClass

mStartDeclCmdClass( IsMenuItemOn, UiObjQuestionCmd )
    bool        isLocalEnvCommand() const               { return false; }
mEndDeclCmdClass

mStartDeclCmdClass( GetMenuItem, UiObjQuestionCmd )
    bool        isLocalEnvCommand() const               { return false; }
mEndDeclCmdClass

mStartDeclCmdClass( IsButtonOn, UiObjQuestionCmd )		
    bool        isOpenQDlgCommand() const       	{ return true; }
protected:
    bool	actQDlgButton(const char* parstr);
mEndDeclCmdClass

mStartDeclCmdClass( GetButton, UiObjQuestionCmd )		
    bool        isOpenQDlgCommand() const       	{ return true; }
protected:
    bool	actQDlgButton(const char* parstr);
mEndDeclCmdClass

mStartDeclCmdClass( NrButtonMenuItems, UiObjQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( IsButtonMenuItemOn, UiObjQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( GetButtonMenuItem, UiObjQuestionCmd )	mEndDeclCmdClass

mStartDeclCmdClass( NrTabs, UiObjQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( CurTab, UiObjQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( IsTabOn, UiObjQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( GetTab, UiObjQuestionCmd )	mEndDeclCmdClass


mStartDeclCmdClass( IsShown, UiObjQuestionCmd )
    bool        isLocalEnvCommand() const               { return false; }
protected:
    bool	actIsCurWinShown(const char* parstr);
mEndDeclCmdClass


mStartDeclComposerClass( Menu, CmdComposer )		mEndDeclComposerClass
mStartDeclComposerClass( Button, CmdComposer )		mEndDeclComposerClass
mStartDeclComposerClass( Close, CmdComposer )		mEndDeclComposerClass
mStartDeclComposerClass( MdiArea, CmdComposer )		mEndDeclComposerClass
mStartDeclComposerClass( QMsgBoxBut, CmdComposer )	mEndDeclComposerClass
mStartDeclComposerClass( Tab, CmdComposer )		mEndDeclComposerClass


}; // namespace CmdDrive

#endif
