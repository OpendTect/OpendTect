#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2017
________________________________________________________________________

-*/

#include "uihdf5mod.h"
#include "uisettings.h"

class uiGenInput;


mExpClass(uiHDF5) uiHDF5Settings : public uiSettingsSubGroup
{ mODTextTranslationClass(uiHDF5Settings);
public:

			uiHDF5Settings(uiSettingsGroup&);

    virtual bool	commit(uiRetVal&);

    uiGenInput*		usehdffld_;
    const bool		initialenabled_;

};
