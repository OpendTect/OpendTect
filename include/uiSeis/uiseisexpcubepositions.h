#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"

class MultiID;
class uiGenInput;
class uiFileInput;
class uiSeisPosProvGroup;
namespace PosInfo { class CubeData; }


mExpClass(uiSeis) uiSeisExpCubePositionsDlg : public uiDialog
{ mODTextTranslationClass(uiSeisExpCubePositionsDlg)
public:

			uiSeisExpCubePositionsDlg(uiParent*);
			~uiSeisExpCubePositionsDlg();

    void		setInput(const MultiID&);
    void		show() override;

private:
    bool		acceptOK(CallBacker*) override;
    bool		getPositions(const MultiID&,PosInfo::CubeData&) const;
    bool		doExport(const PosInfo::CubeData&,
				 const char* filenm) const;

    uiSeisPosProvGroup* inpfld_;
    uiGenInput*		isascfld_;
    uiFileInput*	outfld_;
    bool		dontask_ = true;
};
