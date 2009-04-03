#ifndef uiveldesc_h
#define uiveldesc_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          November 2007
 RCS:           $Id: uiveldesc.h,v 1.8 2009-04-03 17:19:23 cvskris Exp $
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

    mClass Setup
    {
    public:
				Setup( const VelocityDesc* vd=0, bool ws=true )
				    : withspan_(ws)
				{ if ( vd ) desc_ = *vd; }

	mDefSetupMemb(VelocityDesc,desc)
	mDefSetupMemb(bool,withspan)
    };

    				uiVelocityDesc(uiParent*,const Setup* s=0);

    VelocityDesc		get() const;
    void			set(const VelocityDesc&);
    bool			updateAndCommit(IOObj&);

protected:

    void			velTypeChange(CallBacker*);

    uiGenInput*			typefld_;
    uiGenInput*			samplefld_;
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
   CtxtIOObj&		ctxt_;

};


//!Field that selects a velocity volume, and edit it's properties/velocity tag


mClass uiVelSel : public uiSeisSel
{
public:
    				uiVelSel(uiParent*,IOObjContext&,
					 const uiSeisSel::Setup&);

    void			updateInput();
    static const IOObjContext&	ioContext();

protected:

    void			editCB(CallBacker*);
    uiPushButton*		editcubebutt_;
};


#endif
