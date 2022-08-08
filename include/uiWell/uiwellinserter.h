#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2020
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uiioobjinserter.h"

class uiToolButtonSetup;

mExpClass(uiWell) uiWellInserter : public uiIOObjInserter
{ mODTextTranslationClass(uiWellInserter);
public:

				uiWellInserter();
				~uiWellInserter();

    uiToolButtonSetup*		getButtonSetup() const override;
    static uiIOObjInserter*	create()
				{ return new uiWellInserter; }

    static void			initClass();

protected:
    void			startRead(CallBacker*);
};
