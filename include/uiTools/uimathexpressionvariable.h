#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	H. Huck
 Date:		Mar 2012
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "mathformula.h"
#include "propertyref.h"
class BufferStringSet;
class UnitOfMeasure;
namespace Math { class Formula; class Expression; }
class uiLabel;
class uiUnitSel;
class uiGenInput;
class uiComboBox;
class uiToolButton;


mExpClass(uiTools) uiMathExpressionVariable : public uiGroup
{ mODTextTranslationClass(uiMathExpressionVariable);
public:

			uiMathExpressionVariable(uiParent*,int varidx,
				    bool withuom=true,
				    bool withsubinps=false,
				    const Math::SpecVarSet* svs=0);
			~uiMathExpressionVariable();

    void		addInpViewIcon(const char* inm,const char* tooltip,
					const CallBack&);
    void		setNonSpecInputs(const BufferStringSet&);
    void		setNonSpecSubInputs(const BufferStringSet&);

    virtual void	use(const Math::Formula&);
    virtual void	use(const Math::Expression*);

    int			varIdx() const		{ return varidx_; }
    const OD::String&	varName() const		{ return varnm_; }
    bool		hasVarName( const char* nm ) const
						{ return varnm_ == nm; }

    bool		isActive() const	{ return isactive_; }
    bool		isConst() const		{ return isconst_; }
    int			specIdx() const		{ return specidx_; }
    const char*		getInput() const;
    const UnitOfMeasure* getUnit() const;
    void		fill(Math::Formula&) const;

    void		selectInput(const char*,bool exact=false);
    void		selectSubInput(int);
    void		setUnit(const UnitOfMeasure*);
    mDeprecatedDef
    void		setUnit(const char*);
    void		setPropType(Mnemonic::StdType);

    Notifier<uiMathExpressionVariable> inpSel;
    Notifier<uiMathExpressionVariable> subInpSel;

    uiGroup*		rightMostField();
    const uiToolButton*	viewBut() const		{ return vwbut_; }

protected:

    void		initFlds( CallBacker* )	{ updateDisp(); }
    void		inpChg( CallBacker* )	{ inpSel.trigger(); }
    void		subInpChg( CallBacker* ) { subInpSel.trigger(); }
    void		showHideVwBut(CallBacker* cb=0);

    const int		varidx_;
    BufferString	varnm_;
    BufferStringSet	nonspecinputs_;
    BufferStringSet	nonspecsubinputs_;
    bool		isactive_;
    bool		isconst_;
    int			specidx_;
    Math::SpecVarSet&	specvars_;

    uiGroup*		inpgrp_;
    uiLabel*		inplbl_;
    uiComboBox*		inpfld_;
    uiComboBox*		subinpfld_;
    uiGenInput*		constfld_;
    uiUnitSel*		unfld_;
    uiToolButton*	vwbut_;

    void		updateDisp();
    void		updateInpNms(bool sub);
    void		setActive(bool);
    void		setVariable(const char*,bool);

};


