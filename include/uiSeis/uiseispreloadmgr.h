#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "datapack.h"
#include "multiid.h"
#include "seistype.h"

class BufferStringSet;
class DataCharacteristics;
class IOObj;
class Scaler;
class TrcKeyZSampling;
class uiComboBox;
class uiGenInput;
class uiListBox;
class uiMapperRangeEditor;
class uiMultiZSeisSubSel;
class uiScaler;
class uiSeisSel;
class uiTextEdit;
namespace Stats { class RandGen; }


/*!\brief Manager for pre-loading Seismic Data */

mExpClass(uiSeis) uiSeisPreLoadMgr : public uiDialog
{ mODTextTranslationClass(uiSeisPreLoadMgr)
public:
			uiSeisPreLoadMgr(uiParent*);
			~uiSeisPreLoadMgr();

protected:

    uiListBox*		listfld_;
    uiTextEdit*		infofld_;

    void		fillList();
    void		initDlgCB(CallBacker*);
    void		fullUpd(CallBacker*);
    void		selChg(CallBacker*);
    void		cubeLoadPush(CallBacker*);
    void		linesLoadPush(CallBacker*);
    void		ps3DPush(CallBacker*);
    void		ps2DPush(CallBacker*);
    void		unloadPush(CallBacker*);
    void		openPush(CallBacker*);
    void		savePush(CallBacker*);
};


mExpClass(uiSeis) uiSeisPreLoadSel : public uiDialog
{ mODTextTranslationClass(uiSeisPreLoadSel)
public:
			uiSeisPreLoadSel(uiParent*,Seis::GeomType,
					 const MultiID& input);
			~uiSeisPreLoadSel();

    const IOObj*	getIOObj() const;
    void		getSampling(TrcKeyZSampling&) const;
    void		getSampling(TrcKeyZSampling&,Pos::GeomID) const;
    void		selectedGeomIDs(TypeSet<Pos::GeomID>&) const;
    Scaler*		getScaler() const;
    void		getDataChar(DataCharacteristics&) const;

protected:

    void		initDlgCB(CallBacker*);
    void		fillHist(CallBacker*);
    void		seisSel(CallBacker*);
    void		selChangeCB(CallBacker*);
    void		histChangeCB(CallBacker*);
    void		doScaleCB(CallBacker*);
    void		updateScaleFld();
    void		updateEstUsage();
    bool		acceptOK(CallBacker*) override;

    Scaler*			scaler_;
    Stats::RandGen&		gen_;

    uiSeisSel*			seissel_;
    uiMultiZSeisSubSel*		subselfld_;
    uiGenInput*			formatdiskfld_;
    uiGenInput*			sizediskfld_;
    uiGenInput*			typefld_;
    uiGenInput*			memusagefld_;
    uiMapperRangeEditor*	histfld_;
    uiGenInput*			nrtrcsfld_;
    uiGenInput*			doscalefld_;
    uiGenInput*			fromrgfld_;
    uiGenInput*			torgfld_;
};



mExpClass(uiSeis) uiSeisPreLoadedDataSel : public uiGroup
{ mODTextTranslationClass(uiSeisPreLoadedDataSel)
public:
				uiSeisPreLoadedDataSel(uiParent*,Seis::GeomType,
					const uiString& txt=uiString::empty());
				~uiSeisPreLoadedDataSel();

    const MultiID&		selectedKey() const;
    const char*			selectedName() const;
    DataPackID			selectedDPID() const;
    int				selectedCompNr() const;
    const char*			selectedCompName() const;

    void			setInput(const MultiID&,int compnr=0);

    Notifier<uiSeisPreLoadedDataSel>	selectionChanged;

protected:

    void			initGrpCB(CallBacker*);
    void			selCB(CallBacker*);
    void			selPushCB(CallBacker*);
    void			preloadCB(CallBacker*);
    void			updateCB(CallBacker*);

    Seis::GeomType		geomtype_;
    TypeSet<MultiID>		keys_;
    BufferStringSet		names_;
    MultiID			selkey_;

    int				compnr_		= 0;

    uiComboBox*			nmfld_;
    uiButton*			selbut_;
    uiButton*			preloadbut_;
};
