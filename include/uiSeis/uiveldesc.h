#ifndef uiveldesc_h
#define uiveldesc_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          November 2007
 RCS:           $Id: uiveldesc.h,v 1.16 2010-03-18 18:14:34 cvskris Exp $
________________________________________________________________________

-*/

#include "uiseissel.h"
#include "veldesc.h"
#include "uizaxistransform.h"

class uiGenInput;
class uiSeisSel;
class uiCheckBox;
class uiStaticsDesc;
class VelocityStretcher;

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
    uiStaticsDesc*		staticsfld_;
    uiCheckBox*			setdefbox_;

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
   void			scanAvgVelCB(CallBacker*);

   uiSeisSel*		volselfld_;
   uiVelocityDesc*	veldescfld_;
   uiGenInput*		topavgvelfld_;
   uiGenInput*		botavgvelfld_;
   uiPushButton*	scanavgvel_;
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


mClass uiTimeDepthBase : public uiZAxisTransform
{
public:
    ZAxisTransform*		getSelection();
    StepInterval<float>		getZRange() const;
    				//!Only if no ZAxisTransform
    const char*			selName() const;
protected:
    				uiTimeDepthBase(uiParent*,bool);
    				~uiTimeDepthBase();
    FixedString			getZDomain() const;
    bool			acceptOK();
    void			useVelChangeCB(CallBacker*);

    void			getDefaultZRange(StepInterval<float>&) const;

    VelocityStretcher*		transform_;
    BufferString		selname_;

    uiGenInput*			usevelfld_;
    uiGenInput*			rangefld_;

    uiVelSel*			velsel_;
    bool 			t2d_;
};


mClass uiTime2Depth : public uiTimeDepthBase
{
public:
    static void			initClass();
    static uiZAxisTransform*	create(uiParent*,const char*);

				uiTime2Depth(uiParent*);
};


mClass uiDepth2Time : public uiTimeDepthBase
{
public:
    static void			initClass();
    static uiZAxisTransform*	create(uiParent*,const char*);

				uiDepth2Time(uiParent*);
};


#endif
