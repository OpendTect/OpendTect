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
#include "multiid.h"


/*! \brief Dialog for surface aux data selection. */

mExpClass(uiEarthModel) uiEMAuxDataSel : public uiCompoundParSel
{ mODTextTranslationClass(uiEMAuxDataSel);
public:
			uiEMAuxDataSel(uiParent*,const uiString& label,
				       const MultiID* =0,
				       const char* auxdata=0 );
    const MultiID&	getSurfaceID() const;
    const char*		getAuxDataSel() const;

protected:

    BufferString		getSummary() const override;
    void			butPushCB(CallBacker*);

    MultiID			hormid_;
    const char*			auxdatanm_;
};

