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

mStartDeclCmdClass( Assign, StealthCmd )	mEndDeclCmdClass
mStartDeclCmdClass( Case, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( Comment, StealthCmd )	mEndDeclCmdClass
mStartDeclCmdClass( GreyOuts, StealthCmd )	mEndDeclCmdClass
mStartDeclCmdClass( IsMatch, StealthCmd )	mEndDeclCmdClass
mStartDeclCmdClass( IsWindow, StealthCmd )	mEndDeclCmdClass
mStartDeclCmdClass( LogMode, StealthCmd ) 	mEndDeclCmdClass
mStartDeclCmdClass( OnError, StealthCmd )	mEndDeclCmdClass
mStartDeclCmdClass( OnOffCheck, StealthCmd )	mEndDeclCmdClass
mStartDeclCmdClass( Sleep, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( Try, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( Wait, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( WinAssert, StealthCmd )	mEndDeclCmdClass

mStartDeclCmdClass( Pause, Command )
    bool	isOpenQDlgCommand() const 	{ return false; }
mEndDeclCmdClass

mStartDeclCmdClass( Guide, Command )
    bool	isOpenQDlgCommand() const 	{ return false; }
mEndDeclCmdClass

mStartDeclCmdClass( Window, StealthCmd )
    bool	isOpenQDlgCommand() const 	{ return false; }
mEndDeclCmdClass

mStartDeclCmdClass( If, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( ElseIf, StealthCmd )	mEndDeclCmdClass
mStartDeclCmdClass( Else, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( Fi, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( Do, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( OdUntil, StealthCmd )	mEndDeclCmdClass
mStartDeclCmdClass( DoWhile, StealthCmd )	mEndDeclCmdClass
mStartDeclCmdClass( Od, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( For, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( Rof, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( Break, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( Continue, StealthCmd )	mEndDeclCmdClass
mStartDeclCmdClass( Def, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( Fed, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( Return, StealthCmd )	mEndDeclCmdClass
mStartDeclCmdClass( Call, StealthCmd )		mEndDeclCmdClass
mStartDeclCmdClass( End, StealthCmd )		mEndDeclCmdClass

}; // namespace CmdDrive

#endif
