#ifndef uisegydefdlg_h
#define uisegydefdlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegydefdlg.h,v 1.4 2008-09-24 14:01:56 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "seistype.h"
class uiSEGYFileSpec;
class uiSEGYFilePars;
class uiComboBox;
class uiGenInput;
class IOObj;


/*!\brief Initial dialog for SEG-Y I/O. */

class uiSEGYDefDlg : public uiDialog
{
public:

    struct Setup : public uiDialog::Setup
    {
			Setup();

	TypeSet<Seis::GeomType> geoms_; // empty=get uiSEGYRead default
    };

			uiSEGYDefDlg(uiParent*,const Setup&,IOPar&);
			~uiSEGYDefDlg();

    void		use(const IOObj*,bool force);
    void		usePar(const IOPar&);

    Seis::GeomType	geomType() const;
    int			nrTrcExamine() const;
    void		fillPar(IOPar&) const;

protected:

    Setup		setup_;
    Seis::GeomType	geomtype_;
    IOPar&		pars_;

    uiSEGYFileSpec*	filespecfld_;
    uiSEGYFilePars*	fileparsfld_;
    uiGenInput*		nrtrcexfld_;
    uiComboBox*		geomfld_;

    void		initFlds(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif
