#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
________________________________________________________________________


-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"

class uiFileSel;
class uiIOObjSel;
class uiSeisSubSel;

namespace PreStack
{

mExpClass(uiPreStackProcessing) uiEventExport : public uiDialog
{ mODTextTranslationClass(uiEventExport);
public:
			uiEventExport(uiParent*,const DBKey*);

protected:
    bool		acceptOK();

    uiIOObjSel*		eventsel_;
    uiSeisSubSel*	subsel_;
    uiFileSel*		outfld_;
};

} // namespace PreStack
