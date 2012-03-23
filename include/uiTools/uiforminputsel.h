#ifndef uiforminputsel_h
#define uiforminputsel_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	H. Huck
 Date:		Mar 2012
 RCS:		$Id: uiforminputsel.h,v 1.1 2012-03-23 08:59:06 cvshelene Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class BufferString;
class BufferStringSet;
class MathExpression;
class uiGenInput;
class uiLabeledComboBox;
class UnitOfMeasure;

mClass uiFormInputSel : public uiGroup
{
public:

				uiFormInputSel(uiGroup*,const BufferStringSet&,
					       int curselidx =0,
					       bool displayuom =true);

    virtual void		use(MathExpression*);
    bool                        hasVarName(const char*) const;
    const char*			getInput() const;
    void                        setUnit(const char*);
    const UnitOfMeasure*	getUnit() const;

protected:

    const int			idx_;			//needed?
    uiLabeledComboBox*         	inpfld_;
    uiLabeledComboBox*         	unfld_;
    uiGenInput*         	cstvalfld_;
    const BufferStringSet&      posinpnms_;
    BufferString		varnm_;
};


#endif
