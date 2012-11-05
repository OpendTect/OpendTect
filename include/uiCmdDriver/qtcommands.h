#ifndef qtcommands_h
#define qtcommands_h

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

namespace CmdDrive
{

mStartDeclCmdClass( uiCmdDriver, ColorOk, Command )		mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, FileOk, Command )		mEndDeclCmdClass


mStartDeclCmdClass( uiCmdDriver, Snapshot, StealthCmd )	mEndDeclCmdClass

mClass(uiCmdDriver) SnapshotActivator: public Activator
{
public:
		SnapshotActivator(const uiMainWin&,const char* filenm,int zoom);
    void	actCB(CallBacker*);
protected:
    uiMainWin&	 actgrabwin_;
    BufferString actfilenm_;
    int		 actzoom_;
};


mStartDeclComposerClass( uiCmdDriver, QColorDlg,CmdComposer,uiMainWin )
    mEndDeclComposerClass

mClass(uiCmdDriver) SetColorActivator: public Activator
{
public:
		SetColorActivator(const Color&);
    void	actCB(CallBacker*);

protected:
    Color	color_;
};

mStartDeclComposerClass( uiCmdDriver, QFileDlg,CmdComposer,uiMainWin )
    mEndDeclComposerClass


}; // namespace CmdDrive

#endif

