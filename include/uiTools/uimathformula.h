#ifndef uimathformula_h
#define uimathformula_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Apr 2014
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "mathformula.h"
class UnitOfMeasure;
class uiButton;
class uiUnitSel;
class uiToolButton;
class uiToolButtonSetup;
class uiMathExpression;
class uiMathExpressionVariable;
namespace Math { class Form; }


/* edits a Math::Formula */


mExpClass(uiTools) uiMathFormula : public uiGroup
{
public:

    mExpClass(uiTools) Setup
    {
    public:
			Setup( const char* lbl=0 )
			    : label_(lbl)
			    , withunits_(true)
			    , maxnrinps_(6)		{}

	mDefSetupMemb(BufferString,label);
	mDefSetupMemb(bool,withunits);
	mDefSetupMemb(int,maxnrinps);

    };

			uiMathFormula(uiParent*,Math::Formula&,const Setup&);
    void		setNonSpecInputs(const BufferStringSet&,int iinp=-1);
					//!< iinp == -1 does all

    bool		setText(const char*);
    const char*		text();

    bool		useForm(const TypeSet<PropertyRef::StdType>* inptyps=0);
    bool		updateForm() const;

			// shortcuts for things available in form
    int			nrInputs() const;
    const char*		getInput(int) const;
    bool		isSpec(int) const;
    bool		isConst(int) const;
    double		getConstVal(int) const;
    const UnitOfMeasure* getUnit() const;

    uiButton*		addButton(const uiToolButtonSetup&);
    void		addInpViewIcon(const char* icnm,const char* tooltip,
					const CallBack&);

    Notifier<uiMathFormula> formSet;
    Notifier<uiMathFormula> inpSet;
    Notifier<uiMathFormula> formUnitSet;

    uiMathExpression*	exprFld()		{ return exprfld_; }
    int			nrInpFlds() const	{ return inpflds_.size(); }
    uiMathExpressionVariable* inpFld( int idx )	{ return inpflds_[idx]; }
    uiUnitSel*		unitFld()		{ return unitfld_; }
    int			inpSelNotifNr() const	{ return notifinpnr_; }
    int			vwLogInpNr(CallBacker*) const;

protected:

    Math::Formula&	form_;
    uiMathExpression*	exprfld_;
    ObjectSet<uiMathExpressionVariable> inpflds_;
    uiUnitSel*		unitfld_;
    uiToolButton*	recbut_;
    int			notifinpnr_;

    Setup		setup_;
    TypeSet<double>	recvals_;

    bool		checkValidNrInputs() const;

    void		formSetCB(CallBacker*);
    void		inpSetCB(CallBacker*);
    void		formUnitSetCB(CallBacker*);
    void		recButPush(CallBacker*);

};


#endif

