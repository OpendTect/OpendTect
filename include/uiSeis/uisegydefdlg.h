#ifndef uisegydefdlg_h
#define uisegydefdlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegydefdlg.h,v 1.1 2008-09-19 14:28:44 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegyio.h"
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
			Setup(bool forrd);

	mDefSetupMemb(bool,forread)
	TypeSet<Seis::GeomType> geoms_; // empty=get uiSEGYIO default
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


/*!\brief Dialog to set further SEG-Y I/O parameters. */

class uiSEGYFileOptsDlg : public uiDialog
{
public :

    class Setup : public uiDialog::Setup
    {
    public:

    			Setup( Seis::GeomType gt,
				uiSEGYIO::Operation op=uiSEGYIO::Scan,
				uiSEGYIO::Purpose pp=uiSEGYIO::ImpExp )
			    : uiDialog::Setup("SEG-Y information content",
				    	      mNoDlgTitle, "103.1.5")
			    , geom_(gt)	
			    , operation_(op)	
			    , purpose_(pp)	
			    , isrev1_(false)		{}

	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(uiSEGYIO::Operation,operation)
	mDefSetupMemb(uiSEGYIO::Purpose,purpose)
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
