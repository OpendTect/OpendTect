#ifndef uimergeseis_h
#define uimergeseis_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          April 2002
 RCS:           $Id: uiseismmproc.h,v 1.1 2002-04-21 15:06:56 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class SeisMMJobMan;
class IOPar;


class uiSeisMMProc : public uiDialog
{
public:
                        uiSeisMMProc(uiParent*,const char* prognm, const IOPar&,
				     const char* seisoutkey,const char* ickey);
			~uiSeisMMProc();

protected:

    SeisMMJobMan*	jm;

    bool		rejectOK(CallBacker*);

};

#endif
