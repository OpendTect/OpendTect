#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiioobjsel.h"
#include "uiioobjseldlg.h"
#include "seistype.h"
#include "ctxtioobj.h"

class uiCheckBox;
class uiComboBox;
class uiLabeledComboBox;
class uiListBox;
class uiSeisIOObjInfo;
class UnitOfMeasure;
namespace ZDomain { class Info; }

mExpClass(uiSeis) uiSeisSel : public uiIOObjSel
{ mODTextTranslationClass(uiSeisSel);
public:

    mExpClass(uiSeis) Setup : public uiIOObjSel::Setup
    {
    public:
	enum SteerPol	{ NoSteering=0, OnlySteering=1, InclSteer=2 };
	enum CompNrPol	{ SingleCompOnly=0, MultiCompOnly=1, Both=2};

			Setup(Seis::GeomType);
			Setup(bool is2d,bool isps);
			~Setup();

	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(bool,allowsetdefault) //!< Fill with def cube/line? True
	mDefSetupMemb(bool,enabotherdomain) //!< write only: T vs Depth False
	mDefSetupMemb(CompNrPol,compnrpol)  //!< Both
	mDefSetupMemb(SteerPol,steerpol)    //!< NoSteering
	mDefSetupMemb(BufferString,zdomkey)
	mDefSetupMemb(bool,allowsetsurvdefault) //!< False
	mDefSetupMemb(bool,explprepost)		//!<Spell out if pre or post stk
	mDefSetupMemb(bool,selectcomp)		/*!< Select only one component
						     False */

	Setup&		wantSteering(bool yn=true);
    };

			uiSeisSel(uiParent*,const IOObjContext&,const Setup&);
			~uiSeisSel();

    bool		fillPar(IOPar&) const override;
    void		usePar(const IOPar&) override;

    inline Seis::GeomType geomType() const { return seissetup_.geom_; }
    inline bool		is2D() const	{ return Seis::is2D(seissetup_.geom_); }
    inline bool		isPS() const	{ return Seis::isPS(seissetup_.geom_); }

    void		setCompNr(int);
    int			compNr() const	{ return compnr_; }
    void		processInput() override;
    bool		existingTyped() const override;
    void		updateInput() override;
    void		updateOutputOpts(bool issteering);
    bool		outputSupportsMultiComp() const;
    const ZDomain::Info& getZDomain() const;
    void		setZDomain(const ZDomain::Info&);

    static IOObjContext	ioContext(Seis::GeomType,bool forread);

    CNotifier<uiSeisSel,const ZDomain::Info&> domainChanged;
    CNotifier<uiSeisSel,BufferString> zUnitChanged;

protected:

    Setup		seissetup_;
    int			compnr_ = 0;
    mutable BufferString curusrnm_;
    IOPar		dlgiopar_;
    uiCheckBox*		othdombox_		= nullptr;
    uiComboBox*		othunitfld_		= nullptr;

    Setup		mkSetup(const Setup&,const IOObjContext&);
    void		fillDefault() override;

    void		initGrpCB(CallBacker*);
    void		domainChgCB(CallBacker*);
    void		zUnitChgCB(CallBacker*);
    void		newSelection(uiIOObjRetDlg*) override;
    void		commitSucceeded() override;
    const char*		userNameFromKey(const char*) const override;
    virtual const char* compNameFromKey(const char*) const;
    uiIOObjRetDlg*	mkDlg() override;
    BufferString	zUnit() const;

    virtual BufferString getDefaultKey(Seis::GeomType) const;

};


mExpClass(uiSeis) uiSeisSelDlg : public uiIOObjSelDlg
{ mODTextTranslationClass(uiSeisSelDlg);
public:

			uiSeisSelDlg(uiParent*,const CtxtIOObj&,
				     const uiSeisSel::Setup&);
			~uiSeisSelDlg();

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

protected:

    uiLabeledComboBox*	compfld_		= nullptr;
    int			steerpol_;
    BufferString	notalloweddatatype_;	// 2D only
    BufferString	zdomainkey_;	// 2D only

    void		entrySel(CallBacker*);
    BufferString	getDataType();
    void		getComponentNames(BufferStringSet&) const;

private:
    static uiString	gtSelTxt(const uiSeisSel::Setup& setup,bool forread);
    friend		class uiSeisSel;
};


mExpClass(uiSeis) uiSteerCubeSel : public uiSeisSel
{ mODTextTranslationClass(uiSteerCubeSel)
public:

				uiSteerCubeSel(uiParent*,bool is2d,
					       bool forread=true,
					       const uiString& txt=
					       toUiString("%1 %2")
					       .arg(uiStrings::sSteering())
					       .arg(uiStrings::sData()));
				~uiSteerCubeSel();

protected:

    BufferString		getDefaultKey(Seis::GeomType) const override;

};
