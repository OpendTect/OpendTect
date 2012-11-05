#ifndef inputcommands_h
#define inputcommands_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          February 2009
 RCS:           $Id$
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
    mClass(uiCmdDriver) typ##Activator: public Activator \
    { \
    public: \
			typ##Activator(const objclass& obj,const char* txt=0, \
				       bool enter=true) \
			    : actobj_( const_cast<objclass&>(obj) ) \
			    , actbuf_(txt), acttxt_(txt ? actbuf_.buf() : 0) \
			    , actenter_(enter) \
			{} \
	void		actCB(CallBacker*); \
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

mClass(uiCmdDriver) SpinActivator: public Activator
{
public:
			SpinActivator(const uiSpinBox&,int nrsteps);
    void		actCB(CallBacker*);
protected:
    uiSpinBox&		actspin_;
    int			actsteps_;
};


mStartDeclCmdClass( uiCmdDriver, Slider, UiObjectCmd )		mEndDeclCmdClass

mClass(uiCmdDriver) SliderActivator: public Activator
{
public:
			SliderActivator(const uiSlider&,float fraction);
    void		actCB(CallBacker*);
protected:
    uiSlider&		actslider_;
    float		actfrac_;
};


mStartDeclCmdClass( uiCmdDriver, GetInput, UiObjQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetSpin, UiObjQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetSlider, UiObjQuestionCmd )	mEndDeclCmdClass


mStartDeclComposerClass( uiCmdDriver, Slider, CmdComposer, uiSlider )
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


}; // namespace CmdDrive

#endif

