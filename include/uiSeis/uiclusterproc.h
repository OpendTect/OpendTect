#ifndef uiclusterproc_h
#define uiclusterproc_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          April 2009
 RCS:           $Id: uiclusterproc.h,v 1.2 2009-07-22 16:01:22 cvsbert Exp $
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
