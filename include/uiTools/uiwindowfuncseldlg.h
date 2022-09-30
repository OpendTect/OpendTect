#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uitoolsmod.h"
#include "uidialog.h"
#include "uifuncdrawerbase.h"
#include "uigraphicsview.h"
#include "uigroup.h"
#include "uibutton.h"
#include "bufstringset.h"
#include "color.h"
#include "mathfunc.h"
#include "multiid.h"
#include "arrayndalgo.h"
#include "arrayndimpl.h"
#include "uistring.h"

class uiAxisHandler;
class uiGenInput;
class uiGraphicsItemGroup;
class uiFuncTaperDisp;
class uiListBox;
class uiRectItem;
class uiWorld2Ui;
class uiSliceSelDlg;

class ArrayNDWindow;
class WindowFunction;

/*!brief Displays a mathfunction. */

mExpClass(uiTools) uiFunctionDrawer :	public uiFuncDrawerBase,
					public uiGraphicsView
{ mODTextTranslationClass(uiFunctionDrawer);
public:

			uiFunctionDrawer(uiParent*,const Setup&);
			~uiFunctionDrawer();

    void		draw(CallBacker*) override;
    void 		setUpAxis();
    uiObject*		uiobj() override	{ return this; }

    uiAxisHandler*	xAxis() const;
    uiAxisHandler*	yAxis() const;

protected:

    uiWorld2Ui*		transform_;
    uiRectItem*		borderrectitem_;
    uiGraphicsItemGroup* polyitemgrp_;

    void		createLine(DrawFunction*);
    void		setFrame();
};


mExpClass(uiTools) uiFuncSelDraw : public uiGroup
{ mODTextTranslationClass(uiFuncSelDraw);
public:
			uiFuncSelDraw(uiParent*,const uiFuncDrawerBase::Setup&);
			~uiFuncSelDraw();

    Notifier<uiFuncSelDraw> funclistselChged;

    void		addFunction(const char* nm=0, FloatMathFunction* f=0,
					bool withcolor=true);
    int			getListSize() const;
    int			getNrSel() const;
    const char*		getCurrentListName() const;
    void		getSelectedItems(TypeSet<int>&) const;
    void		setSelectedItems(const TypeSet<int>&);
    bool		isSelected(int) const;
    void		removeItem(int);
    int			removeLastItem();
    void		setAsCurrent(const char*);
    void		setSelected(int);
    void		setFunctionRange(Interval<float>);
    void		setAxisRange(Interval<float>);

    void		funcSelChg(CallBacker*);
    void		funcCheckChg(CallBacker*);

protected:

    uiFuncDrawerBase*	view_;
    uiListBox*		funclistfld_;
    TypeSet<OD::Color>	colors_;
    ObjectSet<FloatMathFunction> mathfunc_;
};


/*!brief Displays a windowfunction. */
mExpClass(uiTools) uiWindowFuncSelDlg : public uiDialog
{ mODTextTranslationClass(uiWindowFuncSelDlg);
public:
			uiWindowFuncSelDlg(uiParent*,const char*,float);
			~uiWindowFuncSelDlg();

    void		funcSelChg(CallBacker*);
    const char*		getCurrentWindowName() const;
    void		setCurrentWindowFunc(const char*,float);
    void		setVariable(float);
    float		getVariable();

protected:

    BufferStringSet	funcnames_;
    float		variable_;
    bool		isfrequency_;
    uiGenInput*		varinpfld_;
    uiFuncSelDraw*	funcdrawer_;
    ObjectSet<WindowFunction>	winfunc_;

    WindowFunction*	getWindowFuncByName(const char*);
};
