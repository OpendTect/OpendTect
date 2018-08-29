#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2017
________________________________________________________________________

-*/

#include "uiodhdf5mod.h"
#include "uisettings.h"

class uiGenInput;


mExpClass(uiODHDF5) uiHDF5Settings : public uiSettingsSubGroup
{ mODTextTranslationClass(uiHDF5Settings);
public:

			uiHDF5Settings(uiSettingsGroup&);

    virtual bool	commit(uiRetVal&);

protected:

    BufferStringSet	typenms_;
    uiGenInput*		usehdffld_;
    ObjectSet<uiGenInput> useflds_;
    const bool		initialglobenabled_;
    BoolTypeSet		initialenabled_;

    static uiString	envBlockStr(const uiString&,const char*);
    BufferString	getSubKey(int) const;
    void		removeWithKey(const char*);

    void		useChgCB(CallBacker*);

};
