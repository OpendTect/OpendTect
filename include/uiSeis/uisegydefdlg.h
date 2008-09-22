#ifndef uisegydefdlg_h
#define uisegydefdlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegydefdlg.h,v 1.2 2008-09-22 15:09:01 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegyread.h"
#include "uidialog.h"
#include "ranges.h"
class uiSEGYFileOpts;
class uiSEGYFileSpec;
class uiSEGYFilePars;
class uiComboBox;
class uiGenInput;
class IOObj;


/*!\brief Initial dialog for SEG-Y I/O. */

class uiSEGYBasic : public uiDialog
{
public:

    struct Setup : public uiDialog::Setup
    {
			Setup();

	TypeSet<Seis::GeomType> geoms_; // empty=get uiSEGYRead default
    };

			uiSEGYBasic(uiParent*,const Setup&,IOPar&);
			~uiSEGYBasic();

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


/*!\brief Dialog to set SEG-Y Read parameters. */

class uiSEGYFileOptsDlg : public uiDialog
{
public :

    class Setup : public uiDialog::Setup
    {
    public:

    			Setup( Seis::GeomType gt,
				uiSEGYRead::Purpose pp=uiSEGYRead::Import )
			    : uiDialog::Setup("SEG-Y information content",
				    	      mNoDlgTitle, "103.1.5")
			    , geom_(gt)	
			    , purpose_(pp)	
			    , isrev1_(false)		{}

	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(uiSEGYRead::Purpose,purpose)
	mDefSetupMemb(bool,isrev1)
    };

			uiSEGYFileOptsDlg(uiParent*,const Setup&,IOPar&);
			~uiSEGYFileOptsDlg();

protected:

    const Setup		setup_;
    IOPar&		pars_;

    uiSEGYFileOpts*	optsfld_;
    uiToolBar*		uitb_;

    bool		getFromScreen(bool);
    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif
