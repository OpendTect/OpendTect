#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uidialog.h"
#include "uistring.h"

namespace Well { class Data; }

class ElasticPropSelection;

class uiIOObjSel;
class uiLabeledComboBox;
class uiCheckBox;
class uiGenInput;
class uiSeisSel;
class uiWellPropSel;
class uiSeis2DLineNameSel;
class uiMultiSynthSeisSel;


namespace WellTie
{

class Setup;
class uiTieWin;

mExpClass(uiWellAttrib) uiTieWinMGRDlg : public uiDialog
{ mODTextTranslationClass(uiTieWinMGRDlg);

public:
			uiTieWinMGRDlg(uiParent*,const WellTie::Setup&);
			~uiTieWinMGRDlg();

    void		delWins();
    const MultiID&	getWellId() const;

protected:

    WellTie::Setup&	wtsetup_;
    ElasticPropSelection& elpropsel_;
    bool		savedefaut_;
    ObjectSet<uiTieWin> welltiedlgset_;
    uiWellPropSel*	logsfld_;

    RefMan<Well::Data>	wd_;

    uiIOObjSel*         wellfld_;
    uiGenInput*		typefld_ = nullptr;
    uiGenInput*		seisextractfld_ = nullptr;
    uiSeisSel*		seis2dfld_ = nullptr;
    uiSeisSel*		seis3dfld_ = nullptr;
    uiSeis2DLineNameSel* seislinefld_ = nullptr;
    uiCheckBox*		used2tmbox_;
    uiLabeledComboBox*	cscorrfld_;
    uiMultiSynthSeisSel* wvltfld_;

    void		getSetup( const char* wllnm );
    bool		getSeismicInSetup();
    bool		getVelLogInSetup() const;
    bool		getDenLogInSetup() const;
    bool		initSetup();
    void		saveWellTieSetup(const MultiID&,
					 const WellTie::Setup&) const;

    void		initDlg(CallBacker*);
    bool		acceptOK(CallBacker*) override;
    void		typeSelChg(CallBacker*);
    void		seisSelChg(CallBacker*);
    void		seis2DCheckChg(CallBacker*);
    void		d2TSelChg(CallBacker*);
    void		wellSelChg(CallBacker*);
    void		wellTieDlgClosed(CallBacker*);
    bool		selIs2D() const;
    bool		seisIDIs3D(MultiID) const;
};

} // namespace WellTie
