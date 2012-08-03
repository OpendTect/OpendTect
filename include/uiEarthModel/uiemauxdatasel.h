#ifndef uiemauxdatasel_h
#define uiemauxdatasel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiemauxdatasel.h,v 1.3 2012-08-03 13:00:55 cvskris Exp $
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uicompoundparsel.h"
#include "multiid.h"


/*! \brief Dialog for surface aux data selection. */

mClass(uiEarthModel) uiEMAuxDataSel : public uiCompoundParSel
{
public:
			uiEMAuxDataSel(uiParent*,const char* label,
				       const MultiID* =0,
				       const char* auxdata=0 );
    const MultiID&	getSurfaceID() const;
    const char*		getAuxDataSel() const;

protected:

    virtual BufferString	getSummary() const;
    bool			butPushCB(CallBacker*);

    MultiID			hormid_;
    const char*			auxdatanm_;
};

#endif

