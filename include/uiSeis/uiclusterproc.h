#ifndef uiclusterproc_h
#define uiclusterproc_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Raman Singh
 Date:          April 2009
 RCS:           $Id: uiclusterproc.h,v 1.1 2009-05-22 05:11:54 cvsraman Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class DirList;
class IOPar;
class Timer;
class uiLabel;
class uiProgressBar;


mClass uiClusterProc : public uiDialog
{
public:
                        uiClusterProc(uiParent*,const IOPar& iop);
			~uiClusterProc();

    static const char*	sKeyScriptDir()		{ return "Script dir"; }
protected:

    int			totalnr_;
    const IOPar&	pars_;
    DirList*		dirlist_;
    Timer*		timer_;
    BufferString	scriptdirnm_;

    uiLabel*		progfld_;
    uiProgressBar*	progbar_;

    void		doPostProcessing();
    void		progressCB(CallBacker*);
};

#endif
