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

				uiMathExpressionVariable(uiGroup*,
						const BufferStringSet&,
						int varidx,
						bool displayuom=true);

    virtual bool		use(const Math::Formula&);
				    //!< returns whether field is displayed
    virtual bool		use(const Math::Expression*);
				    //!< returns whether field is displayed

    const BufferString&		varName() const		{ return varnm_; }
    bool                        hasVarName( const char* nm ) const
							{ return varnm_ == nm; }

    const char*			getInput() const;
    void			setUnit(const UnitOfMeasure*);
    void			setUnit(const char*);
    const UnitOfMeasure*	getUnit() const;
    float			getCstVal() const;
    bool			isCst() const;
    void			setCurSelIdx(int);

    Notifier<uiMathExpressionVariable> inpSel;

protected:

    void			selChg(CallBacker*);

    const int			varidx_;
    uiLabeledComboBox*		inpfld_;
    uiUnitSel*			unfld_;
    uiGenInput*			cstvalfld_;
    const BufferStringSet&	posinpnms_;
    BufferString		varnm_;

    bool			newVar(const char*);

};


#endif

