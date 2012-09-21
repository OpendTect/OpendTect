#ifndef uiveldesc_h
#define uiveldesc_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          November 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uigeninput.h"
#include "uiseissel.h"
#include "uizaxistransform.h"
#include "veldesc.h"

class uiSeisSel;
class uiCheckBox;
class uiStaticsDesc;
class VelocityStretcher;

/*!Group that allows the user to edit VelocityDesc information. */

mClass(uiSeis) uiVelocityDesc : public uiGroup
{
public:

    mClass(uiSeis) Setup
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

    NotifierAccess&		typeChangeNotifier();

protected:

    void			updateFlds(CallBacker*);

    uiGenInput*			typefld_;
    uiGenInput*			hasstaticsfld_;
    uiStaticsDesc*		staticsfld_;
    uiCheckBox*			setdefbox_;

};


/*!Dialog that allows the user to edit VelocityDesc information. */
mClass(uiSeis) uiVelocityDescDlg : public uiDialog
{
public:
    			uiVelocityDescDlg(uiParent*,const IOObj* cursel=0,
					  const uiVelocityDesc::Setup* s=0);
			~uiVelocityDescDlg();

   IOObj*		getSelection() const;
   			//!<returned object must be managed by caller
    Interval<float>	getVelocityTopRange() const	
    			{ return topavgvelfld_->getFInterval(0); }
    Interval<float>	getVelocityBottomRange() const	
    			{ return botavgvelfld_->getFInterval(0); }

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


mClass(uiSeis) uiVelSel : public uiSeisSel
{
public:
    				uiVelSel(uiParent*,IOObjContext&,
					 const uiSeisSel::Setup&,
					 bool iseditbutton=true);

    void			setInput(const MultiID&);
    static const IOObjContext&	ioContext();
    
    Interval<float>		getVelocityTopRange() const	{ return trg_; }
    Interval<float>		getVelocityBottomRange() const	{ return brg_; }
    Notifier<uiVelSel>		velrgchanged;

protected:

    void			selectionDoneCB(CallBacker*);
    void			updateEditButton();
    void			editCB(CallBacker*);
    uiPushButton*		editcubebutt_;
    Interval<float>		trg_;
    Interval<float>		brg_;
};


mClass(uiSeis) uiTimeDepthBase : public uiZAxisTransform
{
public:
    bool			acceptOK();

    ZAxisTransform*		getSelection();
    StepInterval<float>		getZRange() const;
    				//!Only if no ZAxisTransform
    const char*			selName() const;
    const MultiID&		selID() const { return selkey_; }
protected:
    				uiTimeDepthBase(uiParent*,bool);
    				~uiTimeDepthBase();
    FixedString			getZDomain() const;

    void			setZRangeCB(CallBacker*);

    VelocityStretcher*		transform_;
    BufferString		selname_;
    MultiID			selkey_;

    uiGenInput*			rangefld_;

    uiVelSel*			velsel_;
    bool 			t2d_;
};


mClass(uiSeis) uiTime2Depth : public uiTimeDepthBase
{
public:
    static void			initClass();
    static uiZAxisTransform*	create(uiParent*,const char*,const char*);

				uiTime2Depth(uiParent*);
};


mClass(uiSeis) uiDepth2Time : public uiTimeDepthBase
{
public:
    static void			initClass();
    static uiZAxisTransform*	create(uiParent*,const char*,const char*);

				uiDepth2Time(uiParent*);
};


#endif

