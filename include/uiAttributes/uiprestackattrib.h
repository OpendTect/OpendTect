#ifndef uiprestackattrib_h
#define uiprestackattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        B.Bril & H.Huck
 Date:          Jan 2008
 RCS:           $Id: uiprestackattrib.h,v 1.9 2009-02-25 10:56:01 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class CtxtIOObj;
namespace Attrib { class Desc; };

class uiLabel;
class uiGenInput;
class uiSeisSel;
namespace PreStack { class uiProcSel; }

/*! \brief PreStack Attribute ui */

mClass uiPreStackAttrib : public uiAttrDescEd
{
public:

			uiPreStackAttrib(uiParent*,bool);
			~uiPreStackAttrib();

    void                getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiSeisSel*				inpfld_;
    uiGenInput*				dopreprocessfld_;
    PreStack::uiProcSel*		preprocsel_;
    uiGenInput*				offsrgfld_;
    uiGenInput*				calctypefld_;
    uiGenInput*				stattypefld_;
    uiGenInput*				lsqtypefld_;
    uiGenInput*				offsaxtypefld_;
    uiGenInput*				valaxtypefld_;
    uiGenInput*				useazimfld_;
    uiLabel*				xlbl_;

    CtxtIOObj&				ctio_;

    bool		setParameters(const Attrib::Desc&);
    bool		getParameters(Attrib::Desc&);

    void		calcTypSel(CallBacker*);
    void		doPreProcSel(CallBacker*);

    			mDeclReqAttribUIFns
};

#endif
