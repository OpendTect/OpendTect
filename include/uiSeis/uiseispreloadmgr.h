#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2009
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "dbkey.h"
#include "seistype.h"

class BufferStringSet;
class DataCharacteristics;
class IOObj;
class Scaler;
class TrcKeyZSampling;
class uiGenInput;
class uiListBox;
class uiMapperRangeEditor;
class uiScaler;
class uiSeisSel;
class uiSeisSubSel;
class uiTextEdit;


/*!\brief Manager for pre-loading Seismic Data */

mExpClass(uiSeis) uiSeisPreLoadMgr : public uiDialog
{ mODTextTranslationClass(uiSeisPreLoadMgr)
public:
			uiSeisPreLoadMgr(uiParent*);

protected:

    uiListBox*		listfld_;
    uiTextEdit*		infofld_;

    void		fillList();
    void		fullUpd(CallBacker*);
    void		selChg(CallBacker*);
    void		cubeLoadPush(CallBacker*);
    void		linesLoadPush(CallBacker*);
    void		ps3DPush(CallBacker*);
    void		ps2DPush(CallBacker*);
    void		unloadPush(CallBacker*);
    void		openPush(CallBacker*);
    void		savePush(CallBacker*);

    DBKey		initmid_;
};


mExpClass(uiSeis) uiSeisPreLoadSel : public uiDialog
{ mODTextTranslationClass(uiSeisPreLoadSel)
public:
			uiSeisPreLoadSel(uiParent*,Seis::GeomType,
					 const DBKey& input);
			~uiSeisPreLoadSel();

    const IOObj*	getIOObj() const;
    void		getSampling(TrcKeyZSampling&) const;
    void		getSampling(TrcKeyZSampling&,Pos::GeomID) const;
    void		selectedGeomIDs(TypeSet<Pos::GeomID>&) const;
    Scaler*		getScaler() const;
    void		getDataChar(DataCharacteristics&) const;

protected:
    void		fillHist(CallBacker*);
    void		seisSel(CallBacker*);
    void		selChangeCB(CallBacker*);
    void		histChangeCB(CallBacker*);
    void		doScaleCB(CallBacker*);
    void		finalizeDoneCB(CallBacker*);
    void		updateScaleFld();
    void		updateEstUsage();
    bool		acceptOK();

    Scaler*		scaler_;

    uiSeisSel*			seissel_;
    uiSeisSubSel*		subselfld_;
    uiGenInput*			typefld_;
    uiMapperRangeEditor*	histfld_;
    uiGenInput*			nrtrcsfld_;
    uiGenInput*			doscalefld_;
    uiGenInput*			fromrgfld_;
    uiGenInput*			torgfld_;
};

