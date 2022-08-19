#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "command.h"
#include "cmdcomposer.h"

#include "uilineedit.h"
#include "uislider.h"
#include "uispinbox.h"


class uiComboBox;


namespace CmdDrive
{

mStartDeclCmdClass( uiCmdDriver, Input, UiObjectCmd )		mEndDeclCmdClass

#define mDeclInputActivator( typ, objclass ) \
\
    mExpClass(uiCmdDriver) typ##Activator: public Activator \
    { \
    public: \
			typ##Activator(const objclass& obj,const char* txt=0, \
				       bool enter=true) \
			    : actobj_( const_cast<objclass&>(obj) ) \
			    , actbuf_(txt), acttxt_(txt ? actbuf_.buf() : 0) \
			    , actenter_(enter) \
			{} \
	void		actCB(CallBacker*) override; \
    protected: \
	objclass&	actobj_; \
	BufferString	actbuf_; \
	const char*	acttxt_; \
	bool		actenter_; \
    };

mDeclInputActivator( Input, uiLineEdit )
mDeclInputActivator( SpinInput, uiSpinBox )
mDeclInputActivator( ComboInput, uiComboBox )


mStartDeclCmdClass( uiCmdDriver, Spin, UiObjectCmd )		mEndDeclCmdClass

mExpClass(uiCmdDriver) SpinActivator: public Activator
{
public:
			SpinActivator(const uiSpinBox&,int nrsteps);
    void		actCB(CallBacker*) override;
protected:
    uiSpinBox&		actspin_;
    int			actsteps_;
};


mStartDeclCmdClass( uiCmdDriver, Slider, UiObjectCmd )		mEndDeclCmdClass

mExpClass(uiCmdDriver) SliderActivator: public Activator
{
public:
			SliderActivator(const uiSlider&,float fraction);
    void		actCB(CallBacker*) override;
protected:
    uiSlider&		actslider_;
    float		actfrac_;
};


mStartDeclCmdClass( uiCmdDriver, GetInput, UiObjQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetSpin, UiObjQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetSlider, UiObjQuestionCmd )	mEndDeclCmdClass


mStartDeclComposerClass( uiCmdDriver, Slider, CmdComposer, uiSliderObj )
    mEndDeclComposerClass

mStartDeclComposerClassWithInit( uiCmdDriver, Input, CmdComposer, uiLineEdit )
protected:
    float textchanged_;
mEndDeclComposerClass 

mStartDeclComposerClassWithInit( uiCmdDriver, Spin, CmdComposer, uiSpinBox )
protected:
    int pendingsteps_;
    float pendinginput_;
mEndDeclComposerClass 


} // namespace CmdDrive
