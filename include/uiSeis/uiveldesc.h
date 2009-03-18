#ifndef uiveldesc_h
#define uiveldesc_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          November 2007
 RCS:           $Id: uiveldesc.h,v 1.6 2009-03-18 18:30:32 cvskris Exp $
________________________________________________________________________

-*/

#include "uiseissel.h"
#include "veldesc.h"

class uiGenInput;
class uiSeisSel;
class CtxtIOObj;

/*!Group that allows the user to edit VelocityDesc information. */

mClass uiVelocityDesc : public uiGroup
{
public:
    				uiVelocityDesc(uiParent*,const VelocityDesc&,
					       bool withsamplespan=true);

    VelocityDesc		get() const;
    void			set(const VelocityDesc&);

protected:

    void			velTypeChange(CallBacker*);

    uiGenInput*			typefld_;
    uiGenInput*			samplefld_;
};


/*!Dialog that allows the user to edit VelocityDesc information. */
mClass uiVelocityDescDlg : public uiDialog
{
public:
    			uiVelocityDescDlg(uiParent*,const IOObj* cursel);
			~uiVelocityDescDlg();

   IOObj*		getSelection() const;
   			//!<returned object must be managed by caller

protected:
   bool			acceptOK(CallBacker*);
   void			volSelChange(CallBacker*);

   uiSeisSel*		volsel_;
   uiVelocityDesc*	veldesc_;
   CtxtIOObj&		ctxt_;
};


//!Field that selects a velocity volume, and edit it's properties/velocity tag


mClass uiVelSel : public uiSeisSel
{
public:
    				uiVelSel(uiParent*,CtxtIOObj&,
					 const uiSeisSel::Setup&);

    void			updateInput();
    static const IOObjContext&	ioContext();

protected:

    void			editCB(CallBacker*);
    uiPushButton*		editcubebutt_;
};


#endif
