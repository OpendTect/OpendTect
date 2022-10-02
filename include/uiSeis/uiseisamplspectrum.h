#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiamplspectrum.h"
#include "datapack.h"


mExpClass(uiSeis) uiSeisAmplSpectrum : public uiAmplSpectrum
{ mODTextTranslationClass(uiSeisAmplSpectrum);
public:
				uiSeisAmplSpectrum(uiParent* p,
					const uiAmplSpectrum::Setup& =
					uiAmplSpectrum::Setup());
				~uiSeisAmplSpectrum();

    void			setDataPackID(DataPackID,DataPackMgr::MgrID,
					      int version=0);
};
