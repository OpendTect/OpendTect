#ifndef uimathattrib_h
#define uimathattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2001
 RCS:           $Id: uimathattrib.h,v 1.15 2010/11/08 10:14:19 cvshelene Exp $
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

mClass uiMathAttrib : public uiAttrDescEd
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

    int			nrvars_;
    int			nrcsts_;
    int			nrspecs_;
    BufferStringSet	varnms;
    BufferStringSet	cstnms;
    
    void 		parsePush(CallBacker*);
    void		updateDisplay(bool);
    void		getVarsNrAndNms(MathExpression*);
    void		setupOneRow(const uiAttrSelData&,int,bool);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    

    			mDeclReqAttribUIFns
};

#endif
