#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          August 2011
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uiobjfileman.h"

/*! \brief
Cross-plot data manager
*/

mExpClass(uiIo) uiDataPointSetMan : public uiObjFileMan
{ mODTextTranslationClass(uiDataPointSetMan);
public:
				uiDataPointSetMan(uiParent*);
				~uiDataPointSetMan();

protected:

    void			mergePush(CallBacker*);

    virtual bool		gtItemInfo(const IOObj&,uiPhraseSet&) const;

public:

    static uiString		sSelDataSetEmpty();

};
