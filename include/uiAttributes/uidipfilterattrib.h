#ifndef uidipfilterattrib_h
#define uidipfilterattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2001
 RCS:           $Id: uidipfilterattrib.h,v 1.10 2007-02-12 09:53:53 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;
class uiLabeledSpinBox;

/*! \brief DipFilter Attribute description editor */

class uiDipFilterAttrib : public uiAttrDescEd
{
public:

			uiDipFilterAttrib(uiParent*,bool);

    virtual void	getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*          inpfld;
    uiLabeledSpinBox*	szfld;
    uiGenInput*		fltrtpfld;
    uiGenInput*		velfld;
    uiGenInput*		azifld;
    uiGenInput*		aziintfld;
    uiGenInput*		taperfld;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    void		filtSel(CallBacker*);
    void		aziSel(CallBacker*);

    			mDeclReqAttribUIFns
};


#endif
