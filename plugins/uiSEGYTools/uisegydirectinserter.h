#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uiioobjinserter.h"
#include "uistring.h"


mExpClass(uiSEGYTools) uiSEGYDirectVolInserter : public uiIOObjInserter
{ mODTextTranslationClass(uiSEGYDirectVolInserter);
public:

				uiSEGYDirectVolInserter();
				~uiSEGYDirectVolInserter();

    virtual uiToolButtonSetup*	getButtonSetup() const;
    static uiIOObjInserter*	create()
				{ return new uiSEGYDirectVolInserter; }

    static void			initClass();

protected:

    void		startScan(CallBacker*);
};


mExpClass(uiSEGYTools) uiSEGYDirect2DInserter : public uiIOObjInserter
{ mODTextTranslationClass(uiSEGYDirect2DInserter);
public:

				uiSEGYDirect2DInserter();
				~uiSEGYDirect2DInserter();

    virtual uiToolButtonSetup*	getButtonSetup() const;
    static uiIOObjInserter*	create()
				{ return new uiSEGYDirect2DInserter; }

    static void			initClass();

protected:

    void		startScan(CallBacker*);
};


mExpClass(uiSEGYTools) uiSEGYDirectPS3DInserter : public uiIOObjInserter
{ mODTextTranslationClass(uiSEGYDirectPS3DInserter);
public:

				uiSEGYDirectPS3DInserter();
				~uiSEGYDirectPS3DInserter();

    virtual uiToolButtonSetup*	getButtonSetup() const;
    static uiIOObjInserter*	create()
				{ return new uiSEGYDirectPS3DInserter;  }

    static void			initClass();

protected:

    void		startScan(CallBacker*);
};
