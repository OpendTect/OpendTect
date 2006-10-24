#ifndef uimathattrib_h
#define uimathattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2001
 RCS:           $Id: uimathattrib.h,v 1.5 2006-10-24 15:21:36 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };
class uiAttrSel;
class uiGenInput;
class uiPushButton;

/*! \brief Math Attribute description editor */

class uiMathAttrib : public uiAttrDescEd
{
public:

			uiMathAttrib(uiParent*);

    void                getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiGenInput*         inpfld_;
    uiPushButton*	parsebut_;
    ObjectSet<uiAttrSel>  attribflds_;
    ObjectSet<uiGenInput> cstsflds_;

    int			nrvariables_;
    void 		parsePush(CallBacker*);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    

    			mDeclReqAttribUIFns
};

#endif
