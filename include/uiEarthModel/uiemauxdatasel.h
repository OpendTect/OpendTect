#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
