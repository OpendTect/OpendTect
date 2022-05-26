#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2014
 RCS:		$Id$
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
