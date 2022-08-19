#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
