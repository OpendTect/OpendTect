#ifndef uisegyread_h
#define uisegyread_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegyread.h,v 1.15 2009-07-22 16:01:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "seistype.h"
#include "odusgclient.h"
#include "iopar.h"
class IOObj;
class uiParent;
class CtxtIOObj;
namespace SEGY { class Scanner; }
class uiSEGYDefDlg;
class uiSEGYExamine;
class uiSEGYImpDlg;
class uiSEGYScanDlg;
class uiSEGYReadRev1Question;


/*!\brief 'Server' for SEG-Y Reading */

mClass uiSEGYRead : public CallBacker
		  , public Usage::Client
{
public:

    enum Purpose	{ Import, SurvSetup, DirectDef };
    enum RevType	{ Rev0, WeakRev1, Rev1 };
    enum State		{ Cancelled, Finished, BasicOpts, Wait4Dialog,
			  SetupImport, SetupScan };

    struct Setup
    {
			Setup( Purpose pp=Import )
			    : purpose_(pp)
			    , initialstate_(BasicOpts)
			{ getDefaultTypes(geoms_,pp==SurvSetup);}

	mDefSetupMemb(Purpose,	purpose)
	mDefSetupMemb(State,	initialstate)
	TypeSet<Seis::GeomType>	geoms_;	//!< Default all

	bool		forScan() const		{ return purpose_ != Import; }
	static void	getDefaultTypes(TypeSet<Seis::GeomType>&,
					bool forsurvsetup=false);

    };

			uiSEGYRead(uiParent*,const Setup&,const IOPar* iop=0);
			~uiSEGYRead();

    void		use(const IOObj*,bool force);
    void		usePar(const IOPar&);

    bool		go();

    Seis::GeomType	geomType() const	{ return geom_; }
    int			revision() const	{ return rev_; }
    void		fillPar(IOPar&) const;
    SEGY::Scanner*	getScanner()		//!< Becomes yours
			{ SEGY::Scanner* s = scanner_; scanner_ = 0; return s; }

    Notifier<uiSEGYRead> processEnded;
    State		state() const		{ return state_; }

    static CtxtIOObj*	getCtio(bool,Seis::GeomType);

protected:

    State		state_;

    Setup		setup_;
    uiParent*		parent_;
    Seis::GeomType	geom_;
    IOPar		pars_;
    RevType		rev_;
    int			revpolnr_;
    SEGY::Scanner*	scanner_;

    uiSEGYDefDlg*	defdlg_;
    uiSEGYExamine*	examdlg_;
    uiSEGYImpDlg*	impdlg_;
    uiSEGYScanDlg*	scandlg_;
    uiSEGYReadRev1Question* rev1qdlg_;

    void		nextAction();
    void		closeDown();
    void		getBasicOpts();
    void		basicOptsGot();
    void		determineRevPol();
    void		setupImport();
    void		setupScan();

    void		readReq(CallBacker*);
    void		writeReq(CallBacker*);
    void		preScanReq(CallBacker*);

    void		defDlgClose(CallBacker*);
    void		examDlgClose(CallBacker*);
    void		rev1qDlgClose(CallBacker*);
    void		impDlgClose(CallBacker*);
    void		scanDlgClose(CallBacker*);

    void		setGeomType(const IOObj&);
    CtxtIOObj*		getCtio(bool) const;

    uiSEGYDefDlg*	newdefdlg_;
};


#endif
