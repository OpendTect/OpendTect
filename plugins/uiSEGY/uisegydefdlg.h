#ifndef uisegydefdlg_h
#define uisegydefdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id$
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

mExpClass(uiSEGY) uiSEGYDefDlg : public uiVarWizardDlg
{ mODTextTranslationClass(uiSEGYDefDlg);
public:

    mStruct(uiSEGY) Setup : public uiDialog::Setup
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
    bool		acceptOK(CallBacker*);

    void		useSpecificPars(const IOPar&);

};


/*!\brief UI for manipulating fille names/paths for a SEGYDirect data-store */

mExpClass(uiSEGY) uiEditSEGYFileDataDlg : public uiDialog
{ mODTextTranslationClass(uiEditSEGYFileDataDlg)
public:
			uiEditSEGYFileDataDlg(uiParent* p,const IOObj&);
			~uiEditSEGYFileDataDlg()	{}

protected:

    const IOObj&	ioobj_;
    IOPar&		filepars_;
    od_int64		fileparsoffset_;

    uiFileInput*	dirsel_;
    uiTable*		filetable_;

    void		fillFileTable();
    void		updateFileTable(int);
    void		editCB(CallBacker*);
    void		dirSelCB(CallBacker*);
    void		fileSelCB(CallBacker*);
    bool		acceptOK(CallBacker*);

    int			nrfiles_;
    bool		isusable_;

};

#endif
