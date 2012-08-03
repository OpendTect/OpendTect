#ifndef uisegydefdlg_h
#define uisegydefdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegydefdlg.h,v 1.15 2012-08-03 13:01:06 cvskris Exp $
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiseismod.h"
#include "uivarwizarddlg.h"
#include "seistype.h"
class uiSEGYFileSpec;
class uiSEGYFilePars;
class uiComboBox;
class uiCheckBox;
class uiGenInput;
class IOObj;


/*!\brief Initial dialog for SEG-Y I/O. */

mClass(uiSeis) uiSEGYDefDlg : public uiVarWizardDlg
{
public:

    mStruct(uiSeis) Setup : public uiDialog::Setup
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
    void		fileSel(CallBacker*);
    void		geomChg(CallBacker*);
    bool		acceptOK(CallBacker*);

    void		useSpecificPars(const IOPar&);

};


#endif


