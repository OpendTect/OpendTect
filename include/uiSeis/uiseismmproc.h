#ifndef uimergeseis_h
#define uimergeseis_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          April 2002
 RCS:           $Id: uiseismmproc.h,v 1.2 2002-04-22 14:40:36 bert Exp $
________________________________________________________________________

-*/

#include "uiexecutor.h"

class SeisMMJobMan;
class IOPar;
class uiLabeledListBox;
class uiButton;
class uiTextEdit;


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
    uiButton*		vwlogfld;
    uiTextEdit*		progrfld;

    bool		rejectOK(CallBacker*);

    Executor&		getFirstJM(const char*,const IOPar&);
    void		newJM();
    void		dispProgress(CallBacker*);
    void		addPush(CallBacker*);
    void		stopPush(CallBacker*);
    void		vwLogPush(CallBacker*);

};

#endif
