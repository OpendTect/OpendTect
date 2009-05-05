#ifndef uiveldesc_h
#define uiveldesc_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          November 2007
 RCS:           $Id: uiveldesc.h,v 1.10 2009-05-05 18:33:02 cvskris Exp $
________________________________________________________________________

-*/

#include "uiseissel.h"
#include "veldesc.h"

class uiGenInput;
class uiSeisSel;
class uiLabeledComboBox;

/*!Group that allows the user to edit VelocityDesc information. */

mClass uiVelocityDesc : public uiGroup
{
public:

    mClass Setup
    {
    public:
				Setup( const VelocityDesc* vd=0 )
				{ if ( vd ) desc_ = *vd; }

	mDefSetupMemb(VelocityDesc,desc)
    };

    				uiVelocityDesc(uiParent*,const Setup* s=0);

    bool			get(VelocityDesc&,bool displayerrors) const;
    void			set(const VelocityDesc&);
    bool			updateAndCommit(IOObj&,bool displayerrors);

protected:

    void			updateFlds(CallBacker*);

    uiGenInput*			typefld_;
    uiGenInput*			hasstaticsfld_;
    uiIOObjSel*			staticshorfld_;
    uiGenInput*			useconstantvelfld_;
    uiGenInput*			constantvelfld_;
    uiLabeledComboBox*		horattribfld_;
};


/*!Dialog that allows the user to edit VelocityDesc information. */
mClass uiVelocityDescDlg : public uiDialog
{
public:
    			uiVelocityDescDlg(uiParent*,const IOObj* cursel=0,
					  const uiVelocityDesc::Setup* s=0);
			~uiVelocityDescDlg();

   IOObj*		getSelection() const;
   			//!<returned object must be managed by caller

protected:

   bool			acceptOK(CallBacker*);
   void			volSelChange(CallBacker*);

   uiSeisSel*		volsel_;
   uiVelocityDesc*	veldesc_;
};


//!Field that selects a velocity volume, and edit it's properties/velocity tag


mClass uiVelSel : public uiSeisSel
{
public:
    				uiVelSel(uiParent*,IOObjContext&,
					 const uiSeisSel::Setup&);

    void			setInput(const MultiID&);
    static const IOObjContext&	ioContext();

protected:

    void			updateEditButton(CallBacker*);
    void			editCB(CallBacker*);
    uiPushButton*		editcubebutt_;
};


#endif
