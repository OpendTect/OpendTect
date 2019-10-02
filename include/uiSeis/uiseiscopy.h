#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2014
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "seistype.h"

class IOObj;
class uiGenInput;
class uiSeisProvider;
class uiSeisStorer;
class uiBatchJobDispatcherSel;


/*!\brief UI for copying seismic data */

mExpClass(uiSeis) uiSeisCopy : public uiDialog
{ mODTextTranslationClass(uiSeisCopy);
public:

    mUseType( Seis,	GeomType );

			uiSeisCopy(uiParent*,const IOObj* startobj=nullptr,
				   const char* allowtransls_fms=nullptr);
			uiSeisCopy(uiParent*,GeomType,
				   const char* allowtransls_fms=nullptr);

    DBKey		copiedID() const;

protected:

    uiSeisProvider*	provfld_;
    uiGenInput*		nullhndlfld_;
    uiSeisStorer*	storfld_;
    uiBatchJobDispatcherSel* batchfld_;

    bool		acceptOK();

private:

    void		init(const IOObj*,const char*,GeomType gt=Seis::Vol);

};
