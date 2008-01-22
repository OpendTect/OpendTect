#ifndef uimathattrib_h
#define uimathattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2001
 RCS:           $Id: uimathattrib.h,v 1.8 2008-01-22 16:24:39 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };
class uiAttrSel;
class uiGenInput;
class uiPushButton;
class uiTable;

/*! \brief Math Attribute description editor */

class uiMathAttrib : public uiAttrDescEd
{
public:

			uiMathAttrib(uiParent*,bool);

    void                getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiGenInput*         inpfld_;
    uiPushButton*	parsebut_;
    ObjectSet<uiAttrSel>  attribflds_;
    ObjectSet<uiGenInput> cstsflds_;
    uiGenInput*		recstartfld_;

    int			nrvariables_;
    int			nrxvars_;
    int			nrcstvars_;
    void 		parsePush(CallBacker*);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    

    			mDeclReqAttribUIFns
};

#endif
