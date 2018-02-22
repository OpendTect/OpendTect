#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2009
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidlggroup.h"
#include "factory.h"
#include "position.h"
#include "uistring.h"

class Array2DInterpol;
class uiCheckBox;
class uiGenInput;
class uiArray2DInterpol;


mExpClass(uiTools) uiArray2DInterpolSel : public uiDlgGroup
{ mODTextTranslationClass(uiArray2DInterpolSel);
public:

				uiArray2DInterpolSel(uiParent*,bool filltype,
					bool holesz, bool withclassification,
					const Array2DInterpol* oldvals,
					bool withstep=false);

    bool			acceptOK();
    Array2DInterpol*		getResult();
				//!<\note Becomes caller's

    void			setDistanceInFeet(bool yn=true);

    HelpKey			helpKey() const;
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

    mDefineFactory1ParamInClass(uiArray2DInterpol,uiParent*,factory);

    virtual void	setValuesFrom(const Array2DInterpol&)		{}
			//*!Dose only work if provided object is of 'your' type.

    bool		acceptOK()					= 0;
    virtual void	setDistanceInFeet(bool yn=true)			{}

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
    bool		isCornersFirst() const;
    int			stepSize() const;
    int			nrSteps() const;

protected:

    bool		acceptOK();

    uiGenInput*		cornersfirstfld_;
    uiGenInput*		stepsizefld_;
    uiGenInput*		nrstepsfld_;

};


mExpClass(uiTools) uiInverseDistanceArray2DInterpol : public uiArray2DInterpol
{ mODTextTranslationClass(uiInverseDistanceArray2DInterpol);
public:

				uiInverseDistanceArray2DInterpol(uiParent*);

    static void			initClass();
    static uiArray2DInterpol*	create(uiParent*);

    void			setValuesFrom(const Array2DInterpol&);
    bool			acceptOK();
    virtual void		setDistanceInFeet(bool yn=true);

    HelpKey			helpKey() const;

protected:

    uiGenInput*			radiusfld_;
    uiButton*			parbut_;

    bool			cornersfirst_;
    int				stepsz_;
    int				nrsteps_;

    void			doParamDlg(CallBacker*);
    virtual Array2DInterpol*	createResult() const;

    friend class		uiInvDistA2DInterpolPars;

};


mExpClass(uiTools) uiTriangulationArray2DInterpol : public uiArray2DInterpol
{ mODTextTranslationClass(uiTriangulationArray2DInterpol);
public:

				uiTriangulationArray2DInterpol(uiParent*);

    static void			initClass();
    static uiArray2DInterpol*	create(uiParent*);

    bool			acceptOK();
    virtual void		setDistanceInFeet(bool yn=true);
    void			setValuesFrom(const Array2DInterpol&);

protected:

    void			intCB(CallBacker*);
    virtual Array2DInterpol*	createResult() const;

    uiGenInput*			maxdistfld_;
    uiCheckBox*			useneighborfld_;
};


mExpClass(uiTools) uiExtensionArray2DInterpol : public uiArray2DInterpol
{ mODTextTranslationClass(uiExtensionArray2DInterpol);
public:

				uiExtensionArray2DInterpol(uiParent*);

    static void			initClass();
    static uiArray2DInterpol*	create(uiParent*);
    void			setValuesFrom(const Array2DInterpol&);

    bool			acceptOK();

protected:

    virtual Array2DInterpol*	createResult() const;
    int				nrsteps_;
    uiGenInput*                 nrstepsfld_;
};
