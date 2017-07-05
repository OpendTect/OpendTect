#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		June 2008
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"

class uiFileSel;
class CtxtIOObj;
class uiIOObjSel;
class uiGenInput;
class uiTableImpDataSel;

namespace Table { class FormatDesc; }

namespace PreStack
{
mExpClass(uiPreStackProcessing) uiImportMute : public uiDialog
{ mODTextTranslationClass(uiImportMute);
public:
			uiImportMute(uiParent*);
			~uiImportMute();

protected:

    uiFileSel*		inpfld_;
    uiGenInput*		inpfilehaveposfld_;
    uiGenInput*		posdatainfld_;
    uiGenInput*		inlcrlfld_;
    uiIOObjSel*		outfld_;

    CtxtIOObj&		ctio_;

    Table::FormatDesc&	fd_;
    uiTableImpDataSel*	dataselfld_;

    bool		haveInpPosData() const;

    void		formatSel(CallBacker*);
    void		changePrefPosInfo(CallBacker*);

    virtual bool	acceptOK();
};


}; //namespace Prestack
