#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          July 2001
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

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    inline Seis::GeomType geomType() const { return seissetup_.geom_; }
    inline bool		is2D() const	{ return Seis::is2D(seissetup_.geom_); }
    inline bool		isPS() const	{ return Seis::isPS(seissetup_.geom_); }

    void		setCompNr(int);
    int			compNr() const	{ return compnr_; }
    virtual void	processInput();
    virtual bool	existingTyped() const;
    virtual void	updateInput();

    static IOObjContext	ioContext(Seis::GeomType,bool forread);

protected:

    Setup		seissetup_;
    int			compnr_;
    mutable BufferString curusrnm_;
    IOPar		dlgiopar_;
    uiCheckBox*		othdombox_;

    Setup		mkSetup(const Setup&,bool forread);
    virtual void	fillDefault();
    virtual void	newSelection(uiIOObjRetDlg*);
    virtual void	commitSucceeded();
    virtual const char*	userNameFromKey(const char*) const;
    virtual const char* compNameFromKey(const char*) const;
    virtual uiIOObjRetDlg* mkDlg();
    void		mkOthDomBox();

    virtual const char* getDefaultKey(Seis::GeomType) const;

};


mExpClass(uiSeis) uiSeisSelDlg : public uiIOObjSelDlg
{ mODTextTranslationClass(uiSeisSelDlg);
public:

			uiSeisSelDlg(uiParent*,const CtxtIOObj&,
				     const uiSeisSel::Setup&);

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

protected:

    uiLabeledComboBox*	compfld_;
    int			steerpol_;
    BufferString	notalloweddatatype_;	// 2D only
    BufferString	zdomainkey_;	// 2D only

    void		entrySel(CallBacker*);
    const char*		getDataType();
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

protected:

    virtual const char*		getDefaultKey(Seis::GeomType gt) const;

};


