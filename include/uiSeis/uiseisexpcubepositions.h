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

class uiGenInput;
class uiFileSel;
class uiSeisPosProvGroup;
namespace PosInfo { class CubeData; }


mExpClass(uiSeis) uiSeisExpCubePositionsDlg : public uiDialog
{ mODTextTranslationClass(uiSeisExpCubePositionsDlg)
public:

			uiSeisExpCubePositionsDlg(uiParent*);
			~uiSeisExpCubePositionsDlg();

    void		setInput(const DBKey&);
    virtual void	show();

private:
    bool		acceptOK();
    bool		getPositions(const DBKey&,PosInfo::CubeData&) const;
    bool		doExport(const PosInfo::CubeData&,
				 const char* filenm) const;

    uiSeisPosProvGroup* inpfld_;
    uiGenInput*		isascfld_;
    uiFileSel*		outfld_;
    bool		dontask_ = true;
};
