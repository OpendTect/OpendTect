#ifndef inputcommands_h
#define inputcommands_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          February 2009
 RCS:           $Id: inputcommands.h,v 1.1 2012-09-17 12:38:33 cvsjaap Exp $
 ________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "command.h"
#include "cmdcomposer.h"

class uiLineEdit;
class uiSpinBox;
class uiSlider;
//class uiThumbWheel;
class uiComboBox;

namespace CmdDrive
{

class CmdDriver;


mStartDeclCmdClass( Input, UiObjectCmd )		mEndDeclCmdClass

#define mDeclInputActivator( typ, objclass ) \
\
    mClass(CmdDriver) typ##Activator: public Activator \
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


mStartDeclCmdClass( Spin, UiObjectCmd )			mEndDeclCmdClass

mClass(CmdDriver) SpinActivator: public Activator
{
public:
			SpinActivator(const uiSpinBox&,int nrsteps);
    void		actCB(CallBacker*);
protected:
    uiSpinBox&		actspin_;
    int			actsteps_;
};


mStartDeclCmdClass( Slider, UiObjectCmd )		mEndDeclCmdClass

mClass(CmdDriver) SliderActivator: public Activator
{
public:
			SliderActivator(const uiSlider&,float fraction);
    void		actCB(CallBacker*);
protected:
    uiSlider&		actslider_;
    float		actfrac_;
};

/*
mStartDeclCmdClass( Wheel, UiObjectCmd )		mEndDeclCmdClass

mClass(CmdDriver) WheelActivator: public Activator
{
public:
			WheelActivator(const uiThumbWheel&,float angle);
    void		actCB(CallBacker*);
protected:
    uiThumbWheel&	actwheel_;
    float		actangle_;
};
*/

mStartDeclCmdClass( GetInput, UiObjQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( GetSpin, UiObjQuestionCmd )		mEndDeclCmdClass
mStartDeclCmdClass( GetSlider, UiObjQuestionCmd )	mEndDeclCmdClass
//mStartDeclCmdClass( GetWheel, UiObjQuestionCmd )	mEndDeclCmdClass


mStartDeclComposerClass( Slider, CmdComposer )		mEndDeclComposerClass 

mStartDeclComposerClassWithInit( Input, CmdComposer )
protected:
    float textchanged_;
mEndDeclComposerClass 
/*
mStartDeclComposerClassWithInit( Wheel, CmdComposer )
protected:
    float oldvalue_;
mEndDeclComposerClass 
*/

mStartDeclComposerClassWithInit( Spin, CmdComposer )
protected:
    int pendingsteps_;
    float pendinginput_;
mEndDeclComposerClass 


}; // namespace CmdDrive

#endif

