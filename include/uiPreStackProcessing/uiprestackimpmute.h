#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "uidialog.h"

class uiFileInput;
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

    uiFileInput*	inpfld_;
    uiGenInput*		inpfilehaveposfld_;
    uiGenInput*		posdatainfld_;
    uiGenInput*		inlcrlfld_;
    uiIOObjSel*		outfld_;

    Table::FormatDesc&	fd_;
    uiTableImpDataSel*	dataselfld_;

    bool		haveInpPosData() const;

    void 		formatSel(CallBacker*);
    void		changePrefPosInfo(CallBacker*);

    bool		acceptOK(CallBacker*) override;
};

} // namespace PreStack
