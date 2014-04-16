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
#include "propertyref.h"
class uiButton;
class uiUnitSel;
class uiToolButton;
class uiToolButtonSetup;
class uiMathExpression;
class uiMathExpressionVariable;
namespace Math { class Formula; }


mExpClass(uiTools) uiMathFormula : public uiGroup
{
public:

    mExpClass(uiTools) Setup
    {
    public:
			Setup( const char* lbl=0 )
			    : label_(lbl)
			    , withunits_(true)
			    , maxnrvars_(6)		{}

	mDefSetupMemb(BufferString,label);
	mDefSetupMemb(bool,withunits);
	mDefSetupMemb(int,maxnrvars);

    };

			uiMathFormula(uiParent*,const Setup&);

    bool		setText(const char*);
    const char*		text();

    bool		useForm(const Math::Formula&,
	    		    const TypeSet<PropertyRef::StdType>* inputtypes=0);
    bool		updateForm(Math::Formula&) const;

    uiButton*		addButton(const uiToolButtonSetup&);
    void		addInpViewIcon(const char* icnm,const char* tooltip,
	    				const CallBack&);

    Notifier<uiMathFormula> formSet;
    Notifier<uiMathFormula> inpSet;
    Notifier<uiMathFormula> formUnitSet;

    uiMathExpression*	exprFld()		{ return exprfld_; }
    uiMathExpressionVariable* varFld( int idx )	{ return varflds_[idx]; }
    uiUnitSel*		unitFld()		{ return unitfld_; }

protected:

    uiMathExpression*	exprfld_;
    ObjectSet<uiMathExpressionVariable> varflds_;
    uiUnitSel*		unitfld_;
    uiToolButton*	recbut_;

    Setup		setup_;
    TypeSet<double>	recvals_;

    bool		checkValidNrInputs(const Math::Formula&) const;

    void		formSetCB(CallBacker*);
    void		inpSetCB(CallBacker*);
    void		formUnitSetCB(CallBacker*);
    void		recButPush(CallBacker*);

};


#endif

