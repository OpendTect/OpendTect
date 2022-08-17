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

class uiIOObjSel;
class uiSeisSubSel;
class uiFileInput;

namespace PreStack
{

mExpClass(uiPreStackProcessing) uiEventExport : public uiDialog
{ mODTextTranslationClass(uiEventExport);
public:
    			uiEventExport(uiParent*, const MultiID*);

protected:
    bool		acceptOK(CallBacker*) override;

    uiIOObjSel*		eventsel_;
    uiSeisSubSel*	subsel_;
    uiFileInput*	outputfile_;
};


} // namespace PreStack
