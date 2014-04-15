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
class BufferStringSet;
class UnitOfMeasure;
namespace Math { class Formula; class Expression; }
class uiUnitSel;
class uiGenInput;
class uiLabeledComboBox;


mExpClass(uiTools) uiMathExpressionVariable : public uiGroup
{
public:

				uiMathExpressionVariable(uiParent*,
					    int varidx,bool displayuom=true,
					    const BufferStringSet* inpnms=0);

    virtual void		use(const Math::Formula&);
    virtual void		use(const Math::Expression*);

    const BufferString&		varName() const		{ return varnm_; }
    bool                        hasVarName( const char* nm ) const
							{ return varnm_ == nm; }

    bool			isActive() const	{ return isactive_; }
    bool			isConst() const		{ return isconst_; }
    const char*			getInput() const;
    const UnitOfMeasure*	getUnit() const;
    void			fill(Math::Formula&) const;

    void			selectInput(const char*,bool exact=false);
    void			setUnit(const UnitOfMeasure*);
    void			setUnit(const char*);

    Notifier<uiMathExpressionVariable> inpSel;

protected:

    void			initFlds(CallBacker*);
    void			selChg(CallBacker*);

    const int			varidx_;
    BufferString		varnm_;
    bool			isactive_;
    bool			isconst_;

    uiLabeledComboBox*		varfld_;
    uiGenInput*			constfld_;
    uiUnitSel*			unfld_;

    void			updateDisp();
    void			setActive(bool);
    void			setVariable(const char*);

};


#endif

