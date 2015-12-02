#ifndef drivercommands_h
#define drivercommands_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          February 2009
 RCS:           $Id$
 ________________________________________________________________________

-*/

#include "command.h"

namespace CmdDrive
{

mStartDeclCmdClass( uiCmdDriver, Assign, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, Case, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, Comment, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GreyOuts, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, IsMatch, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, IsWindow, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, LogMode, StealthCmd ) 		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, OnError, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, OnOffCheck, StealthCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, Sleep, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, Try, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, Wait, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, WinAssert, StealthCmd )	mEndDeclCmdClass

mStartDeclCmdClass( uiCmdDriver, Pause, Command )
    bool	isOpenQDlgCommand() const 	{ return false; }
mEndDeclCmdClass

mStartDeclCmdClass( uiCmdDriver, Guide, Command )
    bool	isOpenQDlgCommand() const 	{ return false; }
mEndDeclCmdClass

mStartDeclCmdClass( uiCmdDriver, Window, StealthCmd )
    bool	isOpenQDlgCommand() const 	{ return false; }
mEndDeclCmdClass

mStartDeclCmdClass( uiCmdDriver, If, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, ElseIf, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, Else, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, Fi, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, Do, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, OdUntil, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, DoWhile, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, Od, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, For, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, Rof, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, Break, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, Continue, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, Def, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, Fed, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, Return, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, Call, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, End, StealthCmd )		mEndDeclCmdClass

}; // namespace CmdDrive

#endif
