#ifndef uiattrsrchprocfiles_h
#define uiattrsrchprocfiles_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Feb 2006
 RCS:           $Id: uiattrsrchprocfiles.h,v 1.5 2009-02-24 14:08:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisrchprocfiles.h"


mClass uiAttrSrchProcFiles : public uiSrchProcFiles
{
public:
			uiAttrSrchProcFiles(uiParent*,bool is2d);
			~uiAttrSrchProcFiles();

protected:

    CtxtIOObj*		ctioptr_;
    CtxtIOObj&		mkCtio(bool);

};

#endif
