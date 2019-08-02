#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

class Array2DInterpol;
class BinnedValueSet;
class BufferStringSet;
class CtxtIOObj;
class HorizonScanner;

class uiCheckBox;
class uiColorInput;
class uiFileSel;
class uiGenInput;
class uiIOObjSel;
class uiListBox;
class uiPosSubSel;
class uiPushButton;
class uiScaler;
class uiStratLevelSel;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }
namespace EM { class Horizon3D; }

/*! \brief Dialog for Horizon Import */

mExpClass(uiEarthModel) uiImportHorizon : public uiDialog
{ mODTextTranslationClass(uiImportHorizon);
public:
    static void		initClass();
			uiImportHorizon(uiParent*,bool);
			~uiImportHorizon();

    bool		doDisplay() const;
    DBKey		getSelID() const;

    Notifier<uiImportHorizon>	importReady;

protected:

    uiFileSel*		inpfld_;
    uiPushButton*	scanbut_;
    uiListBox*		attrlistfld_;
    uiPosSubSel*	subselfld_;
    uiGenInput*		filludffld_;
    uiPushButton*	interpolparbut_;
    Array2DInterpol*	interpol_;
    uiTableImpDataSel*	dataselfld_;
    uiColorInput*	colbut_;
    uiStratLevelSel*	stratlvlfld_;
    uiIOObjSel*		outputfld_;
    uiCheckBox*		displayfld_;

    virtual bool	acceptOK();
    void                descChg(CallBacker*);
    void		inputChgd(CallBacker*);
    void		addAttribCB(CallBacker*);
    void		rmAttribCB(CallBacker*);
    void		clearListCB(CallBacker*);
    void		scanPush(CallBacker*);
    void		fillUdfSel(CallBacker*);
    void		stratLvlChg(CallBacker*);
    void		interpolSettingsCB(CallBacker*);

    bool		getFileNames(BufferStringSet&) const;
    bool		checkInpFlds();
    bool		doScan();
    bool		doImport();
    bool		fillUdfs(ObjectSet<BinnedValueSet>&);
    EM::Horizon3D*	createHor() const;
    EM::Horizon3D*	loadHor();

    CtxtIOObj&		ctio_;
    Table::FormatDesc&	fd_;
    HorizonScanner*	scanner_;
    bool		isgeom_;

private:
    uiString		goOnMsg();
};
