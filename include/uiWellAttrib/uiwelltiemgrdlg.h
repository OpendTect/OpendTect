#ifndef uiwelltiemgrdlg_h
#define uiwelltiemgrdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2009
 RCS:           $Id$
________________________________________________________________________

-*/


#include "uiwellattribmod.h"
#include "uidialog.h"

namespace Well { class Data; }

class CtxtIOObj;
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
{

public:
			uiTieWinMGRDlg(uiParent*,WellTie::Setup&);
			~uiTieWinMGRDlg();

    void		delWins();

protected:

    WellTie::Setup&	wtsetup_;
    CtxtIOObj&		wllctio_; //will be removed
    CtxtIOObj&		wvltctio_; //will be removed
    CtxtIOObj&		seisctio2d_; //will be removed
    CtxtIOObj&		seisctio3d_; //will be removed
    ElasticPropSelection& elpropsel_;
    bool		savedefaut_;
    bool		is2d_; //will be removed
    ObjectSet<uiTieWin> welltiedlgset_;
    ObjectSet<uiTieWin> welltiedlgsetcpy_;
    uiWellPropSel*	logsfld_;

    Well::Data*		wd_;

    uiIOObjSel*         wellfld_;
    uiSeisWaveletSel*	wvltfld_;
    uiGenInput*		typefld_;
    uiGenInput*		seisextractfld_;
    uiSeisSel*		seis2dfld_;
    uiSeisSel*		seis3dfld_;
    uiSeis2DLineNameSel* seislinefld_;
    uiCheckBox*		used2tmbox_;
    uiLabeledComboBox*	cscorrfld_;
    uiWaveletExtraction* extractwvltdlg_;

    void		getSetup( const char* wllnm );
    bool		getSeismicInSetup();
    bool		getVelLogInSetup() const;
    bool		getDenLogInSetup() const;
    bool		initSetup();
    void		saveWellTieSetup(const MultiID&,
					 const WellTie::Setup&) const;

    void		onFinalise(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		extrWvlt(CallBacker*);
    void		extractWvltDone(CallBacker*);
    void		typeSelChg(CallBacker*);
    void		seisSelChg(CallBacker*);
    void		seis2DCheckChg(CallBacker*);
    void		d2TSelChg(CallBacker*);
    void		wellSelChg(CallBacker*);
    void		wellTieDlgClosed(CallBacker*);
	void		wellToBeDeleted(CallBacker*);
    void		set3DSeis() const {}; //will be removed
    void		set2DSeis() const {}; //will be removed
    void		setLine() const {}; //will be removed
    void		setTypeFld() {}; //will be removed
    bool		selIs2D() const;
    bool		seisIDIs3D(MultiID) const;
};

}; //namespace WellTie
#endif

