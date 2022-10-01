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
#include "uicompoundparsel.h"
#include "seistype.h"
#include "ctxtioobj.h"

class uiSeisIOObjInfo;
class uiLabeledComboBox;
class uiListBox;
class uiCheckBox;

mExpClass(uiSeis) uiSeisSel : public uiIOObjSel
{ mODTextTranslationClass(uiSeisSel);
public:

    struct Setup : public uiIOObjSel::Setup
    {
	enum SteerPol	{ NoSteering=0, OnlySteering=1, InclSteer=2 };

			Setup( Seis::GeomType gt )
			    : geom_(gt)
			    , allowsetdefault_(true)
			    , steerpol_(NoSteering)
			    , enabotherdomain_(false)
			    , survdefsubsel_( 0 )
			    , allowsetsurvdefault_(false)
			    , explprepost_(false)
			    , selectcomp_(false)	{}
			Setup( bool is2d, bool isps )
			    : geom_(Seis::geomTypeOf(is2d,isps))
			    , allowsetdefault_(true)
			    , steerpol_(NoSteering)
			    , enabotherdomain_(false)
			    , survdefsubsel_( 0 )
			    , allowsetsurvdefault_(false)
			    , explprepost_(false)
			    , selectcomp_(false)	{}
			~Setup()			{}

	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(bool,allowsetdefault)	//!< Fill with def cube/line?
	mDefSetupMemb(bool,enabotherdomain)	//!< write only: T vs Depth
	mDefSetupMemb(SteerPol,steerpol)
	mDefSetupMemb(BufferString,zdomkey)
	mDefSetupMemb(const char*,survdefsubsel)
	mDefSetupMemb(bool,allowsetsurvdefault)
	mDefSetupMemb(bool,explprepost)		//!<Spell out if pre or post stk
	mDefSetupMemb(bool,selectcomp)		//!< Select only one component

	Setup&		wantSteering( bool yn=true )
			{
			    steerpol_ = yn ? OnlySteering : NoSteering;
			    return *this;
			}
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

    static IOObjContext	ioContext(Seis::GeomType,bool forread);

protected:

    Setup		seissetup_;
    int			compnr_;
    mutable BufferString curusrnm_;
    IOPar		dlgiopar_;
    uiCheckBox*		othdombox_;

    Setup		mkSetup(const Setup&,bool forread);
    void		fillDefault() override;
    void		newSelection(uiIOObjRetDlg*) override;
    void		commitSucceeded() override;
    const char*		userNameFromKey(const char*) const override;
    virtual const char* compNameFromKey(const char*) const;
    uiIOObjRetDlg*	mkDlg() override;
    void		mkOthDomBox();

    virtual const char* getDefaultKey(Seis::GeomType) const;

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

    uiLabeledComboBox*	compfld_;
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

    const char*		getDefaultKey(Seis::GeomType gt) const override;

};
