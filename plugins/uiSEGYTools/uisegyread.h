#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uivarwizard.h"
#include "multiid.h"
#include "uistring.h"

class IOObj;
class CtxtIOObj;
namespace SEGY { class Scanner; }
class uiSEGYDefDlg;
class uiSEGYImpDlg;
class uiSEGYScanDlg;
class uiSEGYExamine;
class uiSEGYReadRev1Question;


/*!\brief 'Server' for SEG-Y Reading */

mExpClass(uiSEGYTools) uiSEGYRead : public uiVarWizard
{ mODTextTranslationClass(uiSEGYRead);
public:

    enum Purpose	{ Import, SurvSetup, DirectDef };
    enum RevType	{ Rev0, WeakRev1, Rev1 };
    enum State		{ BasicOpts=10, SetupImport=11, SetupScan=12 };

    mExpClass(uiSEGYTools) Setup
    {
    public:
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

    virtual void	raiseCurrent();

    Seis::GeomType	geomType() const	{ return geom_; }
    int			revision() const	{ return rev_; }
    void		fillPar(IOPar&) const;
    SEGY::Scanner*	getScanner()		//!< Becomes yours
			{ SEGY::Scanner* s = scanner_; scanner_ = 0; return s; }
    MultiID		outputID() const	{ return outid_; }

    static CtxtIOObj*	getCtio(bool,Seis::GeomType);

protected:


    Setup		setup_;
    Seis::GeomType	geom_;
    RevType		rev_;
    int			revpolnr_;
    SEGY::Scanner*	scanner_;
    MultiID		outid_;

    uiSEGYDefDlg*	defdlg_;
    uiSEGYImpDlg*	impdlg_;
    uiSEGYScanDlg*	scandlg_;
    uiSEGYExamine*	examdlg_;
    uiSEGYReadRev1Question* rev1qdlg_;

    virtual void	doPart();
    virtual void	closeDown();
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

    uiSEGYDefDlg*	newdefdlg_; // no longer used, will disappear after 6.0
};
