#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"

class Array2DInterpol;
class BinIDValueSet;
class BufferStringSet;
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
			uiImportHorizon(uiParent*,bool);
			~uiImportHorizon();

    MultiID		getSelID() const;

    Notifier<uiImportHorizon>	importReady;

protected:

    uiFileInput*	inpfld_;
    uiPushButton*	scanbut_;
    uiListBox*		attrlistfld_;
    uiPosSubSel*	subselfld_;
    uiGenInput*		filludffld_			= nullptr;
    uiPushButton*	interpolparbut_;
    Array2DInterpol*	interpol_			= nullptr;
    uiTableImpDataSel*	dataselfld_;
    uiColorInput*	colbut_				= nullptr;
    uiStratLevelSel*	stratlvlfld_			= nullptr;
    uiIOObjSel*		outputfld_;

    uiGenInput*		zdomainfld_				= nullptr;

    bool		acceptOK(CallBacker*) override;
    void		descChg(CallBacker*);
    void		inputChgd(CallBacker*);
    void		addAttribCB(CallBacker*);
    void		rmAttribCB(CallBacker*);
    void		clearListCB(CallBacker*);
    void		scanPush(CallBacker*);
    void		fillUdfSel(CallBacker*);
    void		stratLvlChg(CallBacker*);
    void		interpolSettingsCB(CallBacker*);
    void		zDomainCB(CallBacker*);

    bool		getFileNames(BufferStringSet&) const;
    bool		checkInpFlds();
    bool		doScan();
    bool		doImport();
    bool		fillUdfs(ObjectSet<BinIDValueSet>&);
    EM::Horizon3D*	createHor() const;
    EM::Horizon3D*	loadHor();

    Table::FormatDesc&	fd_;
    HorizonScanner*	scanner_			= nullptr;
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
    uiUnitSel*			unitfld_;
    uiIOObjSel*			outputfld_;

    bool			acceptOK(CallBacker*) override;
    void			inputChgd(CallBacker*);
    EM::Horizon3D*		createHor() const;
};
