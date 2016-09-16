#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          August 2011
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uiobjfileman.h"

/*! \brief
CrossPlot manager
*/

mExpClass(uiIo) uiDataPointSetMan : public uiObjFileMan
{ mODTextTranslationClass(uiDataPointSetMan);
public:
    				uiDataPointSetMan(uiParent*);
				~uiDataPointSetMan();

protected:

    void			mergePush(CallBacker*);

    void			mkFileInfo();
public:

    static uiString		sSelDataSetEmpty();

};
