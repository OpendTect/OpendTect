#ifndef uimathattrib_h
#define uimathattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2001
 RCS:           $Id: uimathattrib.h,v 1.11 2008-05-14 15:09:26 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };
class uiAttrSel;
class uiGenInput;
class uiPushButton;
class uiTable;
class MathExpression;

/*! \brief Math Attribute description editor */

class uiMathAttrib : public uiAttrDescEd
{
public:

			uiMathAttrib(uiParent*,bool);

    void                getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiGenInput*         inpfld_;
    uiPushButton*	parsebut_;
    uiTable*            xtable_;
    uiTable*            ctable_;
    ObjectSet<uiAttrSel>  attribflds_;
    uiGenInput*		recstartfld_;
    uiGenInput*		recstartposfld_;

    int			nrvariables_;
    int			nrxvars_;
    int			nrcstvars_;
    
    void 		parsePush(CallBacker*);
    void		updateDisplay(bool);
    void		checkVarSpelAndShift(MathExpression*,bool&,bool&);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    

    			mDeclReqAttribUIFns
};

#endif
