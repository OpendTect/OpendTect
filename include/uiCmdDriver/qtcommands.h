#ifndef qtcommands_h
#define qtcommands_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          February 2009
 RCS:           $Id: qtcommands.h,v 1.1 2012-09-17 12:38:33 cvsjaap Exp $
 ________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "command.h"
#include "cmdcomposer.h"

namespace CmdDrive
{

class CmdDriver;


mStartDeclCmdClass( ColorOk, Command )		mEndDeclCmdClass
mStartDeclCmdClass( FileOk, Command )		mEndDeclCmdClass


mStartDeclCmdClass( Snapshot, StealthCmd )	mEndDeclCmdClass

mClass(CmdDriver) SnapshotActivator: public Activator
{
public:
		SnapshotActivator(const uiMainWin&,const char* filenm,int zoom);
    void	actCB(CallBacker*);
protected:
    uiMainWin&	 actgrabwin_;
    BufferString actfilenm_;
    int		 actzoom_;
};


mStartDeclComposerClass( QColorDlg, CmdComposer )	mEndDeclComposerClass

mClass(CmdDriver) SetColorActivator: public Activator
{
public:
		SetColorActivator(const Color&);
    void	actCB(CallBacker*);

protected:
    Color	color_;
};

mStartDeclComposerClass( QFileDlg, CmdComposer )	mEndDeclComposerClass


}; // namespace CmdDrive

#endif

