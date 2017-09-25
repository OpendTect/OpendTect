#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2003
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uicompoundparsel.h"
#include "dbkey.h"


/*! \brief Dialog for surface aux data selection. */

mExpClass(uiEarthModel) uiEMAuxDataSel : public uiCompoundParSel
{ mODTextTranslationClass(uiEMAuxDataSel);
public:
			uiEMAuxDataSel(uiParent*,const uiString& label,
				       const DBKey* =0,
				       const char* auxdata=0 );
    const DBKey&	getSurfaceID() const;
    const char*		getAuxDataSel() const;

protected:

    virtual uiString		getSummary() const;
    void			butPushCB(CallBacker*);

    DBKey			hormid_;
    const char*			auxdatanm_;
};
