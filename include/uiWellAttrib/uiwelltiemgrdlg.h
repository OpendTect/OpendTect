#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2009
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
class uiSeisWaveletSel;
class uiWaveletExtraction;


namespace WellTie
{

class Setup;
class uiTieWin;

mExpClass(uiWellAttrib) uiTieWinMGRDlg : public uiDialog
{ mODTextTranslationClass(uiTieWinMGRDlg);

public:
			uiTieWinMGRDlg(uiParent*,WellTie::Setup&);
			~uiTieWinMGRDlg();

    void		delWins();
    const MultiID&	getWellId() const;

protected:

    WellTie::Setup&	wtsetup_;
    ElasticPropSelection& elpropsel_;
    bool		savedefaut_;
    ObjectSet<uiTieWin> welltiedlgset_;
    uiWellPropSel*	logsfld_;

    Well::Data*		wd_ = nullptr;

    uiIOObjSel*         wellfld_;
    uiSeisWaveletSel*	wvltfld_;
    uiGenInput*		typefld_ = nullptr;
    uiGenInput*		seisextractfld_ = nullptr;
    uiSeisSel*		seis2dfld_ = nullptr;
    uiSeisSel*		seis3dfld_ = nullptr;
    uiSeis2DLineNameSel* seislinefld_ = nullptr;
    uiCheckBox*		used2tmbox_;
    uiLabeledComboBox*	cscorrfld_;
    uiWaveletExtraction* extractwvltdlg_ = nullptr;

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

