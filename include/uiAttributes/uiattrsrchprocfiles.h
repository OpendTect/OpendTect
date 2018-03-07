#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2006
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uisrchprocfiles.h"


mExpClass(uiAttributes) uiAttrSrchProcFiles : public uiSrchProcFiles
{
public:
			uiAttrSrchProcFiles(uiParent*,bool is2d);
			~uiAttrSrchProcFiles();

protected:

    CtxtIOObj*		ctioptr_;
    CtxtIOObj&		mkCtio(bool);

};
