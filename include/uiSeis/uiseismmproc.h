#ifndef uimergeseis_h
#define uimergeseis_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          April 2002
 RCS:           $Id: uiseismmproc.h,v 1.5 2002-04-24 22:19:06 bert Exp $
________________________________________________________________________

-*/

#include "uiexecutor.h"

class SeisMMJobMan;
class IOPar;
class uiLabeledListBox;
class uiGroup;
class uiButton;
class uiIOFileSelect;
class uiTextEdit;
class uiFileBrowser;


class uiSeisMMProc : public uiExecutor
{
public:
                        uiSeisMMProc(uiParent*,const char* prognm,const IOPar&);
			~uiSeisMMProc();

protected:

    SeisMMJobMan*	jm;
    uiLabeledListBox*	avmachfld;
    uiLabeledListBox*	usedmachfld;
    uiButton*		addbut;
    uiButton*		stopbut;
    uiButton*		vwlogbut;
    uiIOFileSelect*	tmpstordirfld;
    uiTextEdit*		progrfld;
    uiFileBrowser*	logvwer;
    uiGroup*		machgrp;
    bool		running;
    bool		finished;
    bool		jmfinished;

    bool		rejectOK(CallBacker*);
    void		doFinalise();
    void		execFinished();

    Executor&		getFirstJM(const char*,const IOPar&);
    void		newJM();
    void		updateCurMachs();
    int			getCurMach(BufferString&) const;

    void		postStep(CallBacker*);
    void		addPush(CallBacker*);
    void		stopPush(CallBacker*);
    void		vwLogPush(CallBacker*);

};

#endif
