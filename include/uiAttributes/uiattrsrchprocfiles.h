#ifndef uiattrsrchprocfiles_h
#define uiattrsrchprocfiles_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Feb 2006
 RCS:           $Id: uiattrsrchprocfiles.h,v 1.3 2006-02-28 16:33:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisrchprocfiles.h"


class uiAttrSrchProcFiles : public uiSrchProcFiles
{
public:
			uiAttrSrchProcFiles(uiParent*);
			~uiAttrSrchProcFiles();

protected:

    CtxtIOObj*		ctioptr_;
    CtxtIOObj&		mkCtio();

};

#endif
