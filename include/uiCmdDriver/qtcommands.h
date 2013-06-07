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

#include "command.h"
#include "cmdcomposer.h"

namespace CmdDrive
{

mStartDeclCmdClass( ColorOk, Command )		mEndDeclCmdClass
mStartDeclCmdClass( FileOk, Command )		mEndDeclCmdClass


mStartDeclCmdClass( Snapshot, StealthCmd )	mEndDeclCmdClass

mClass SnapshotActivator: public Activator
{
public:
		SnapshotActivator(const uiMainWin&,const char* filenm,int zoom);
    void	actCB(CallBacker*);
protected:
    uiMainWin&	 actgrabwin_;
    BufferString actfilenm_;
    int		 actzoom_;
};


mStartDeclComposerClass( QColorDlg,CmdComposer,uiMainWin ) mEndDeclComposerClass

mClass SetColorActivator: public Activator
{
public:
		SetColorActivator(const Color&);
    void	actCB(CallBacker*);

protected:
    Color	color_;
};

mStartDeclComposerClass( QFileDlg,CmdComposer,uiMainWin ) mEndDeclComposerClass


}; // namespace CmdDrive

#endif

