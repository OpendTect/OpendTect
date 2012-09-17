#ifndef uiattrsrchprocfiles_h
#define uiattrsrchprocfiles_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Feb 2006
 RCS:           $Id: uiattrsrchprocfiles.h,v 1.6 2009/07/22 16:01:20 cvsbert Exp $
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
