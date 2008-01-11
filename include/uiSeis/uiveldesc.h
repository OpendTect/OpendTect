#ifndef uiveldesc_h
#define uiveldesc_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          November 2007
 RCS:           $Id: uiveldesc.h,v 1.3 2008-01-11 18:26:14 cvskris Exp $
________________________________________________________________________

-*/

#include "uiseissel.h"
#include "veldesc.h"

class uiGenInput;
class uiSeisSel;
class CtxtIOObj;

/*!Group that allows the user to edit VelocityDesc information. */

class uiVelocityDesc : public uiGroup
{
public:
    				uiVelocityDesc(uiParent*,const VelocityDesc&);

    VelocityDesc		get() const;
    void			set(const VelocityDesc&);

protected:

    void			velTypeChange(CallBacker*);

    uiGenInput*			typefld_;
    uiGenInput*			samplefld_;
};


/*!Dialog that allows the user to edit VelocityDesc information. */
class uiVelocityDescDlg : public uiDialog
{
public:
    			uiVelocityDescDlg(uiParent*,const IOObj* cursel);
			~uiVelocityDescDlg();

protected:
   bool			acceptOK(CallBacker*);
   void			volSelChange(CallBacker*);

   uiSeisSel*		volsel_;
   uiVelocityDesc*	veldesc_;
   CtxtIOObj&		ctxt_;
};


//!Field that selects a velocity volume, and edit it's properties/velocity tag


class uiVelSel : public uiSeisSel
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
