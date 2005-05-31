#ifndef ui3dfilterattrib_h
#define ui3dfilterattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2001
 RCS:           $Id: uidipfilterattrib.h,v 1.1 2005-05-31 12:35:24 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;
class uiLabeledSpinBox;

/*! \brief DipFilter Attribute description editor */

class ui3DFilterAttrib : public uiAttrDescEd
{
public:

			ui3DFilterAttrib(uiParent*);

protected:

    uiAttrSel*          inpfld;
    uiLabeledSpinBox*	szfld;
    uiGenInput*		fltrtpfld;
    uiGenInput*		velfld;
    uiGenInput*		azifld;
    uiGenInput*		aziintfld;
    uiGenInput*		taperfld;
    uiGenInput*         kernelfld;
    uiGenInput*         shapefld;
    uiGenInput*         outpfld;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    virtual void	updateForm(bool);
    virtual void	set2D(bool);

    void		filtSel(CallBacker*);
    void		aziSel(CallBacker*);
    void		kernelSel(CallBacker*);
};


#define mIfHaveVal(pars,var) \
if ( pars.var.isSet() || pars.var.getMode() == AttribParameter::Default )


#endif
