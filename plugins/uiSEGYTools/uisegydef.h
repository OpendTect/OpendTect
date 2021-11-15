#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uigroup.h"
#include "uisegyread.h"
#include "segyfiledef.h"
#include "uistring.h"
#include "coordsystem.h"

class IOObj;
namespace SEGY { class TrcHeaderDef; }
class uiLabel;
class uiButton;
class uiCheckBox;
class uiGenInput;
class uiTabStack;
class uiFileInput;
class uiSEGYByteSpec;


/*!\brief base class for specification of SEG-Y file stuff */

mExpClass(uiSEGYTools) uiSEGYDefGroup : public uiGroup
{ mODTextTranslationClass(uiSEGYDefGroup);
public:
			uiSEGYDefGroup( uiParent* p, const char* grpnm,
					bool forread )
			    : uiGroup(p,grpnm)
			    , forread_(forread)				{}

    virtual bool	fillPar(IOPar&,bool permissive=false) const	= 0;
    virtual void	usePar(const IOPar&)				= 0;
    virtual void	use(const IOObj*,bool force)			= 0;

    virtual void	getReport( IOPar& iop ) const
			{ fillPar( iop, true ); }

protected:

    bool		forread_;
};


/*!\brief UI for Specification of SEG-Y in- or output file(s) */

mExpClass(uiSEGYTools) uiSEGYFileSpec : public uiSEGYDefGroup
{ mODTextTranslationClass(uiSEGYFileSpec);
public:
    mExpClass(uiSEGYTools) Setup
    {
    public:
			Setup( bool needmulti )
			    : forread_(true)
			    , canbe3d_(true)
			    , needmultifile_(needmulti)
			    , pars_(nullptr)		{}

	mDefSetupMemb(bool,forread)
	mDefSetupMemb(bool,canbe3d)
	mDefSetupMemb(bool,needmultifile) //!< only used if for read
	mDefSetupMemb(const IOPar*,pars)
    };

			uiSEGYFileSpec(uiParent*,const Setup&);

    bool		fillPar(IOPar&,bool permissive=false) const;
    void		usePar(const IOPar&);
    void		use(const IOObj*,bool force);
    void		setFileName(const char*);
    BufferString	getJobNameFromFileName() const;

    SEGY::FileSpec	getSpec() const;
    void		setSpec(const SEGY::FileSpec&);

    void		setInp2D(bool);
    bool		isProbablySwapped() const	{ return swpd_; }
    bool		isIEEEFmt() const		{ return isieee_; }
    bool		isProbablySeisWare() const	{ return issw_; }

    static const char*	sKeyLineNmToken()	{ return "#L"; }
    static const char*	fileFilter()
			{ return  "SEG-Y files (*.sgy *.SGY *.segy)"; }

    Notifier<uiSEGYFileSpec>	fileSelected;

protected:

    uiFileInput*	fnmfld_;
    uiGenInput*		multifld_;
    uiButton*		manipbut_;
    bool		is2d_;
    bool		needmulti_;
    bool		swpd_;
    bool		isieee_;
    bool		issw_;

    void		setMultiInput(const StepInterval<int>&,int);

    void		fileSel(CallBacker*);
    void		manipFile(CallBacker*);

    BufferString	getFileName() const;
};


/*!\brief UI for Specification of SEG-Y information needed to examine */

mExpClass(uiSEGYTools) uiSEGYFilePars : public uiSEGYDefGroup
{ mODTextTranslationClass(uiSEGYFilePars);
public:
			uiSEGYFilePars(uiParent*,bool forread,IOPar* iop=0,
					bool withiobuts=true);

    bool		fillPar(IOPar&,bool permissive=false) const;
    void		usePar(const IOPar&);
    void		use(const IOObj*,bool force);

    FilePars		getPars() const;
    void		setPars(const FilePars&);

    void		setBytesSwapped(bool fullswap,bool dataswap=false);
			//!< dataswap only used if fullswap is false

    Notifier<uiSEGYFilePars> readParsReq;
    Notifier<uiSEGYFilePars> writeParsReq;

protected:

    uiGenInput*		nrsamplesfld_;
    uiGenInput*		fmtfld_;
    uiGenInput*		byteswapfld_;
    ConstRefMan<Coords::CoordSystem> coordsys_;

    void		readParsPush(CallBacker*);
    void		writeParsPush(CallBacker*);

public:

    static uiString	sRetSavedGrp() { return tr("Retrieve Saved Group"); }

};


/*!\brief UI for Specification of SEG-Y fields needed for proper import

  The idea is that you know beforehand whether the file is Rev.1 or not.
  If it's Rev. 1, the positioning part will not be present.

 */

mExpClass(uiSEGYTools) uiSEGYFileOpts : public uiSEGYDefGroup
{ mODTextTranslationClass(uiSEGYFileOpts);
public:

    mExpClass(uiSEGYTools) Setup
    {
    public:
				Setup( Seis::GeomType gt,
				    uiSEGYRead::Purpose pp=uiSEGYRead::Import,
				    uiSEGYRead::RevType rt=uiSEGYRead::Rev0 )
				    : geom_(gt)
				    , purpose_(pp)
				    , revtype_(rt)	{}

	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(uiSEGYRead::Purpose,purpose)
	mDefSetupMemb(uiSEGYRead::RevType,revtype)
    };

			uiSEGYFileOpts(uiParent*,const Setup&,const IOPar* i=0);
			~uiSEGYFileOpts();

    const Setup&	setup() const		{ return setup_; }
    bool		forScan() const
			{ return setup_.purpose_ != uiSEGYRead::Import; }

    bool		fillPar(IOPar&,bool permissive=false) const;
    void		usePar(const IOPar&);
    void		getReport(IOPar&) const;
    void		use(const IOObj*,bool force);

    Notifier<uiSEGYFileOpts> readParsReq;
    Notifier<uiSEGYFileOpts> writeParsReq;
    Notifier<uiSEGYFileOpts> preScanReq;

protected:

    Setup		setup_;
    bool		is2d_;
    bool		isps_;
    SEGY::TrcHeaderDef&	thdef_;

    uiTabStack*		ts_;
    uiGroup*		posgrp_;
    uiGroup*		psgrp_;
    uiGroup*		orulegrp_;
    uiGroup*		coordgrp_;

    uiGenInput*		posfld_;
    uiGenInput*		psposfld_;
    uiSEGYByteSpec*	inldeffld_;
    uiSEGYByteSpec*	crldeffld_;
    uiSEGYByteSpec*	trnrdeffld_;
    uiSEGYByteSpec*	refnrdeffld_;
    uiSEGYByteSpec*	offsdeffld_;
    uiSEGYByteSpec*	azimdeffld_;
    uiSEGYByteSpec*	xcoorddeffld_;
    uiSEGYByteSpec*	ycoorddeffld_;
    uiGenInput*		regoffsfld_;
    uiGenInput*		readcoordsfld_;
    uiGenInput*		coordsstartfld_;
    uiGenInput*		coordsstepfld_;
    uiGenInput*		coordsextfld_;
    uiFileInput*	coordsfnmfld_;
    uiCheckBox*		coordsspecfnmbox_;
    uiLabel*		ensurepsxylbl_;

    uiGenInput*		scalcofld_;
    uiGenInput*		timeshiftfld_;
    uiGenInput*		sampleratefld_;
    uiGenInput*		havecoordsinhdrfld_;

    uiGroup*		mkORuleGrp(const IOPar&);
    uiGroup*		mkPosGrp(const IOPar&);
    uiGroup*		mkPSGrp(const IOPar&);
    uiGroup*		mkCoordGrp(const IOPar&);

    void		mkBinIDFlds(uiGroup*,const IOPar&);
    void		mkCoordFlds(uiGroup*,const IOPar&);

    void		initFlds(CallBacker*);
    void		psPosChg(CallBacker*);
    void		readParsPush(CallBacker*);
    void		writeParsPush(CallBacker*);
    void		preScanPush(CallBacker*);
    void		crdChk(CallBacker*);

    int			psPosType() const;
    void		toggledFldFillPar(uiGenInput*,const IOPar&,const char*,
					  bool isz=false);
    void		setToggled(IOPar&,const char*,uiGenInput*,
				   bool isz=false) const;

public:
    const uiString	sPreScanFiles() { return tr("Pre-scan the files"); }

};


