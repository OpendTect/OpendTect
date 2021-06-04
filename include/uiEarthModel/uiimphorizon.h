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
class BinIDValueSet;
class BufferStringSet;
class CtxtIOObj;
class HorizonScanner;

class uiCheckBox;
class uiColorInput;
class uiFileInput;
class uiGenInput;
class uiIOObjSel;
class uiListBox;
class uiPosSubSel;
class uiPushButton;
class uiScaler;
class uiStratLevelSel;
class uiTableImpDataSel;
class uiT2DConvSel;
class uiUnitSel;
namespace Coords { class uiCoordSystemSel; }
namespace EM { class Horizon3D; }
namespace Table { class FormatDesc; }

/*! \brief Dialog for Horizon Import */

mExpClass(uiEarthModel) uiImportHorizon : public uiDialog
{ mODTextTranslationClass(uiImportHorizon);
public:
    static void		initClass();
			uiImportHorizon(uiParent*,bool);
			~uiImportHorizon();

    MultiID		getSelID() const;

    Notifier<uiImportHorizon>	importReady;

protected:

    uiFileInput*	inpfld_;
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

    uiCheckBox*		tdsel_;
    uiT2DConvSel*	transfld_;

    virtual bool	acceptOK(CallBacker*);
    void		descChg(CallBacker*);
    void		inputChgd(CallBacker*);
    void		addAttribCB(CallBacker*);
    void		rmAttribCB(CallBacker*);
    void		clearListCB(CallBacker*);
    void		scanPush(CallBacker*);
    void		fillUdfSel(CallBacker*);
    void		stratLvlChg(CallBacker*);
    void		interpolSettingsCB(CallBacker*);
    void		zDomSel(CallBacker*);

    bool		getFileNames(BufferStringSet&) const;
    bool		checkInpFlds();
    bool		doScan();
    bool		doImport();
    bool		fillUdfs(ObjectSet<BinIDValueSet>&);
    EM::Horizon3D*	createHor() const;
    EM::Horizon3D*	loadHor();

    CtxtIOObj&		ctio_;
    Table::FormatDesc&	fd_;
    HorizonScanner*	scanner_;
    bool		isgeom_;

private:
    uiString		goOnMsg();
};


mExpClass(uiEarthModel) uiImpHorFromZMap final : public uiDialog
{ mODTextTranslationClass(uiImpHorFromZMap);
public:
				uiImpHorFromZMap(uiParent*);
				~uiImpHorFromZMap();

    MultiID			getSelID() const;

    Notifier<uiImpHorFromZMap>	importReady;

protected:

    uiFileInput*		inpfld_;
    Coords::uiCoordSystemSel*	crsfld_;
    uiPosSubSel*		subselfld_;
    uiUnitSel*			unitfld_;
    uiIOObjSel*			outputfld_;

    virtual bool		acceptOK(CallBacker*);
    void			inputChgd(CallBacker*);
    EM::Horizon3D*		createHor() const;
};

