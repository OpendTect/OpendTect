#ifndef uimathexpressionvariable_h
#define uimathexpressionvariable_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	H. Huck
 Date:		Mar 2012
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "mathformula.h"
class BufferStringSet;
class UnitOfMeasure;
namespace Math { class Formula; class Expression; }
class uiUnitSel;
class uiGenInput;
class uiToolButton;
class uiLabeledComboBox;


mExpClass(uiTools) uiMathExpressionVariable : public uiGroup
{
public:

			uiMathExpressionVariable(uiParent*,int varidx,
				    bool displayuom=true,
				    const Math::SpecVarSet* svs=0);

    void		addInpViewIcon(const char* inm,const char* tooltip,
					const CallBack&);
    void		setNonSpecInputs(const BufferStringSet&);

    virtual void	use(const Math::Formula&);
    virtual void	use(const Math::Expression*);

    int			varIdx() const		{ return varidx_; }
    const BufferString&	varName() const		{ return varnm_; }
    bool		hasVarName( const char* nm ) const
						{ return varnm_ == nm; }

    bool		isActive() const	{ return isactive_; }
    bool		isConst() const		{ return isconst_; }
    int			specIdx() const		{ return specidx_; }
    const char*		getInput() const;
    const UnitOfMeasure* getUnit() const;
    void		fill(Math::Formula&) const;

    void		selectInput(const char*,bool exact=false);
    void		setUnit(const UnitOfMeasure*);
    void		setUnit(const char*);
    void		setPropType(PropertyRef::StdType);

    Notifier<uiMathExpressionVariable> inpSel;

    uiGroup*		rightMostField();
    const uiToolButton*	viewBut() const		{ return vwbut_; }

protected:

    void		initFlds(CallBacker*);
    void		selChg(CallBacker*);
    void		showHideVwBut(CallBacker*);

    const int		varidx_;
    BufferString	varnm_;
    BufferStringSet	nonspecinputs_;
    bool		isactive_;
    bool		isconst_;
    int			specidx_;
    Math::SpecVarSet	specvars_;

    uiLabeledComboBox*	varfld_;
    uiGenInput*		constfld_;
    uiUnitSel*		unfld_;
    uiToolButton*	vwbut_;

    void		getInpNms(BufferStringSet&) const;
    void		updateDisp();
    void		updateInpNms();
    void		setActive(bool);
    void		setVariable(const char*,bool);

};


#endif

