#ifndef uiattrsrchprocfiles_h
#define uiattrsrchprocfiles_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Feb 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uisrchprocfiles.h"


mClass(uiAttributes) uiAttrSrchProcFiles : public uiSrchProcFiles
{
public:
			uiAttrSrchProcFiles(uiParent*,bool is2d);
			~uiAttrSrchProcFiles();

protected:

    CtxtIOObj*		ctioptr_;
    CtxtIOObj&		mkCtio(bool);

};

#endif

