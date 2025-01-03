#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uivarwizarddlg.h"

class uiSEGYFileSpec;
class uiSEGYFilePars;
class uiComboBox;
class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiTable;

class IOObj;


/*!\brief Initial dialog for SEG-Y I/O. */

mExpClass(uiSEGYTools) uiSEGYDefDlg : public uiVarWizardDlg
{ mODTextTranslationClass(uiSEGYDefDlg);
public:

    mStruct(uiSEGYTools) Setup : public uiDialog::Setup
    {
					Setup();

	mDefSetupMemb(Seis::GeomType,	defgeom)
	TypeSet<Seis::GeomType>		geoms_;
					//!< empty=get uiSEGYRead default
    };

			uiSEGYDefDlg(uiParent*,const Setup&,IOPar&);

    void		use(const IOObj*,bool force);
    void		usePar(const IOPar&);

    Seis::GeomType	geomType() const;
    int			nrTrcExamine() const;
    void		fillPar(IOPar&) const;

    Notifier<uiSEGYDefDlg> readParsReq;
    Notifier<uiSEGYDefDlg> writeParsReq;

protected:

    Setup		setup_;
    Seis::GeomType	geomtype_;

    uiSEGYFileSpec*	filespecfld_;
    uiSEGYFilePars*	fileparsfld_;
    uiGenInput*		nrtrcexfld_;
    uiComboBox*		geomfld_;
    uiCheckBox*		savenrtrcsbox_;

    void		initFlds(CallBacker*);
    void		readParsCB(CallBacker*);
    void		writeParsCB(CallBacker*);
    void		fileSel(CallBacker*);
    void		geomChg(CallBacker*);
    bool		acceptOK(CallBacker*) override;

    void		useSpecificPars(const IOPar&);

};

