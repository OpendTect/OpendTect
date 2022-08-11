#pragma once

/*
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        A. Huck
Date:	       May 2015
RCS:	       $Id $
______________________________________________________________________

*/

#include "uiseismod.h"
#include "uiamplspectrum.h"
#include "datapack.h"


mExpClass(uiSeis) uiSeisAmplSpectrum : public uiAmplSpectrum
{ mODTextTranslationClass(uiSeisAmplSpectrum);
public:
				uiSeisAmplSpectrum( uiParent* p,
					const uiAmplSpectrum::Setup& =
					uiAmplSpectrum::Setup() )
				    : uiAmplSpectrum(p)
				{}

    void			setDataPackID(DataPackID,DataPackMgr::MgrID,
					      int version=0);
};

