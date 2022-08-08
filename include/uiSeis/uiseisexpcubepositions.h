#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck
 Date:		Sep 2018
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

