#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidlggroup.h"
#include "factory.h"
#include "uistring.h"

class Array2DInterpol;
class uiCheckBox;
class uiGenInput;
class uiArray2DInterpol;


mExpClass(uiTools) uiArray2DInterpolSel : public uiDlgGroup
{ mODTextTranslationClass(uiArray2DInterpolSel);
public:
    mDefineFactory1ParamInClass(uiArray2DInterpol,uiParent*,factory);

				uiArray2DInterpolSel(uiParent*,bool filltype,
					bool holesz, bool withclassification,
					const Array2DInterpol* oldvals,
					bool withstep=false);

    bool			acceptOK() override;
    Array2DInterpol*		getResult();
				//!<\note Becomes caller's

    void			setDistanceUnit(const uiString&);
				//!<A unitstring in [] that tells what the
				//!<unit is for going from one cell to another

    HelpKey			helpKey() const override;
    void			fillPar(IOPar&) const;

    void			setStep(const BinID&);
    BinID			getStep() const;

protected:
					~uiArray2DInterpolSel();

    uiParent*				getTopObject();
    void				selChangeCB(CallBacker*);

    Array2DInterpol*			result_;

    uiGenInput*				filltypefld_;
    uiGenInput*				maxholeszfld_;
    uiGenInput*				methodsel_;
    uiGenInput*				isclassificationfld_;
    uiGenInput*				stepfld_;

    ObjectSet<uiArray2DInterpol>	params_;
};


mExpClass(uiTools) uiArray2DInterpol : public uiDlgGroup
{ mODTextTranslationClass(uiArray2DInterpol);
public:
    virtual void	setValuesFrom(const Array2DInterpol&)		{}
			//*!Dose only work if provided object is of 'your' type.

    bool		acceptOK() override				= 0;
    virtual void	setDistanceUnit(const uiString&)		{}

    Array2DInterpol*	getResult();
			//!<Becomes caller's
protected:
			uiArray2DInterpol(uiParent*,const uiString& caption);
			~uiArray2DInterpol();

    virtual Array2DInterpol*	createResult() const	{ return 0; }

    Array2DInterpol*	result_;
};


mExpClass(uiTools) uiInvDistInterpolPars : public uiDialog
{ mODTextTranslationClass(uiInvDistInterpolPars);
public:

			uiInvDistInterpolPars(uiParent* p,bool cornersfirst,
					      int stepsz,int nrsteps);
			~uiInvDistInterpolPars();

    bool		isCornersFirst() const;
    int			stepSize() const;
    int			nrSteps() const;

protected:

    bool		acceptOK(CallBacker*) override;

    uiGenInput*		cornersfirstfld_;
    uiGenInput*		stepsizefld_;
    uiGenInput*		nrstepsfld_;
};


mExpClass(uiTools) uiInverseDistanceArray2DInterpol : public uiArray2DInterpol
{ mODTextTranslationClass(uiInverseDistanceArray2DInterpol);
public:
				uiInverseDistanceArray2DInterpol(uiParent*);
				~uiInverseDistanceArray2DInterpol();

    static void			initClass();
    static uiArray2DInterpol*	create(uiParent*);

    void			setValuesFrom(const Array2DInterpol&) override;
    bool			acceptOK() override;
    void			setDistanceUnit(const uiString&) override;

    HelpKey			helpKey() const override;

protected:

    uiGenInput*			radiusfld_;
    uiButton*			parbut_;

    bool			cornersfirst_;
    int				stepsz_;
    int				nrsteps_;

    void			doParamDlg(CallBacker*);
    Array2DInterpol*		createResult() const override;

    friend class		uiInvDistA2DInterpolPars;

};


mExpClass(uiTools) uiTriangulationArray2DInterpol : public uiArray2DInterpol
{ mODTextTranslationClass(uiTriangulationArray2DInterpol);
public:
				uiTriangulationArray2DInterpol(uiParent*);
				~uiTriangulationArray2DInterpol();

    static void			initClass();
    static uiArray2DInterpol*	create(uiParent*);

    bool			acceptOK() override;
    void			setDistanceUnit(const uiString&) override;
    void			setValuesFrom(const Array2DInterpol&) override;

protected:

    void			intCB(CallBacker*);
    Array2DInterpol*		createResult() const override;

    uiGenInput*			maxdistfld_;
    uiCheckBox*			useneighborfld_;
};


mExpClass(uiTools) uiExtensionArray2DInterpol : public uiArray2DInterpol
{ mODTextTranslationClass(uiExtensionArray2DInterpol);
public:
				uiExtensionArray2DInterpol(uiParent*);
				~uiExtensionArray2DInterpol();

    static void			initClass();
    static uiArray2DInterpol*	create(uiParent*);
    void			setValuesFrom(const Array2DInterpol&) override;

    bool			acceptOK() override;

protected:

    Array2DInterpol*		createResult() const override;
    int				nrsteps_;
    uiGenInput*			nrstepsfld_;
};
