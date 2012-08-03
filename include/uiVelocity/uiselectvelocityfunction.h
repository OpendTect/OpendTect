#ifndef uiselectvelocityfunction_h
#define uiselectvelocityfunction_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          November 2006
 RCS:           $Id: uiselectvelocityfunction.h,v 1.5 2012-08-03 13:01:16 cvskris Exp $
________________________________________________________________________

-*/

#include "uivelocitymod.h"
#include "factory.h"
#include "uidialog.h"
#include "uigroup.h"

class Color;
class uiListBox;
class uiColorInput;
class uiGenInput;
class uiPushButton;

namespace Vel
{
class FunctionSource;


//!uiGroup to select a velocity function type

mClass(uiVelocity) uiFunctionSel : public uiGroup
{
public:

				uiFunctionSel(uiParent*,
				    const ObjectSet<FunctionSource>&,
				    const TypeSet<Color>*);
				~uiFunctionSel();

    ObjectSet<FunctionSource>&	getVelSources();
    const TypeSet<Color>&	getColor() const { return colors_; }

    Notifier<uiFunctionSel>	listChange;

protected:
    void			updateList();
    void			selChangedCB(CallBacker*);
    void			addPushedCB(CallBacker*);
    void			removePushedCB(CallBacker*);
    void			propPushedCB(CallBacker*);
    void			colorChanged(CallBacker*);

    uiListBox*				list_;
    uiPushButton*			addbutton_;
    uiPushButton*			removebutton_;
    uiPushButton*			propbutton_;
    uiColorInput*			colorfld_;

    ObjectSet<FunctionSource>		velsources_;
    TypeSet<Color>			colors_;
};


//!Base class for velocity function settings
mClass(uiVelocity) uiFunctionSettings : public uiGroup
{
public:
    mDefineFactory2ParamInClass( uiFunctionSettings, uiParent*,
				 FunctionSource*, factory );
				uiFunctionSettings(uiParent* p,const char* nm)
				    : uiGroup( p, nm ) {}

    virtual FunctionSource*	getSource() 	= 0;
    virtual bool		acceptOK() 	= 0;
};




mClass(uiVelocity) uiAddFunction : public uiDialog
{
public:
    				uiAddFunction( uiParent* );
    FunctionSource*		getSource();
public:
    void			typeSelChangeCB(CallBacker*);
    bool			acceptOK(CallBacker*);

    uiGenInput*				typesel_;
    ObjectSet<uiFunctionSettings>	settingldgs_;
};


mClass(uiVelocity) uiEditFunction : public uiDialog
{
public:
    				uiEditFunction( uiParent*,
						FunctionSource* );
    bool			isOK() const { return dlggrp_; }
public:
    bool			acceptOK(CallBacker*);

    uiFunctionSettings*		dlggrp_;
};

}; //namespace


#endif

