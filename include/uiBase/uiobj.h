#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/08/1999
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uibaseobject.h"
#include "uigeom.h"
#include "uilayout.h"
#include "color.h"

mFDQtclass(QString)
class MouseCursor;
class uiFont;
class uiObjectBody;
class uiParent;
class uiMainWin;
class i_LayoutItem;
class uiPixmap;
class uiObjEventFilter;


/*!\brief The base class for most UI elements.

Noteworthy:

  * setStretch(int hor,int ver
	Sets stretch factors for object. 0=no stretch in that direction.
	a factor > 1 makes it take up as much space as possible in its parent.
  * SzPolicy
    Many objects can take any size, but to actually set them to any size
    looks terribly chaotic. Thusm we use 3 options:
	Small:	1 base sz.
	Medium:	2* base sz + 1.
	Wide:	4* base sz + 3.
    The xxVar options specify that the element may have a bigger internal
    preferred size. In that case, the maximum is taken.
    The xxMax options specify that the element should take all available
    space ( stretch = 2 )
  * attach(ConstraintType,...)
	tie objects together in a certain way. Very important is the concept of
	'align obejct', which is the object itself if it's stand-alone, but
	usually one of teh objects in a group. It s the object that is logical
	to align with other objects. For example, simple label+input box groups
	have the input box as horizontal align object.
	For the 'stretched' aligning, margin=-1 (default) stretches the object
	not to cross any border. Margin=-2 stretches the object to fill the
	parent's border. This looks nice with separators.

*/

mExpClass(uiBase) uiObject : public uiBaseObject
{
    friend class	uiObjectBody;
    friend class	i_LayoutItem;

public:
			~uiObject();

    enum		SzPolicy    //!< see class comments
			{	UseDefault,
				Small, Medium, Wide,
				SmallVar, MedVar, WideVar,
				SmallMax, MedMax, WideMax
			};
    void		setHSzPol(SzPolicy);
    void		setVSzPol(SzPolicy);
    SzPolicy		szPol(bool hor=true) const;

    virtual int		width() const;	//!< Actual size in pixels
    virtual int		height() const;	//!< Actual size in pixels
    uiSize		actualSize(bool include_border=true) const;

    virtual void	setName(const char*);
			//!< This is an identifier; rarely displayed to the user

    void		setToolTip(const uiString&);
    const uiString&	toolTip() const;
    static void		updateAllToolTips();

    void		translateText();

    void		setFocus();
    bool		hasFocus() const;
    void		disabFocus();

    virtual Color	backgroundColor() const;
    Color		roBackgroundColor() const;
    virtual void	setBackgroundColor(const Color&);
    virtual void	setBackgroundPixmap(const uiPixmap&);
    virtual void	setTextColor(const Color&);

    bool		isDisplayed() const;
    inline void		display( bool yn )  { display( yn, false ); }
    bool		isSensitive() const;
    void		setSensitive(bool yn=true);
    bool		isVisible() const;
    virtual void	setEmpty()	    {}
    virtual bool	isEmpty() const	    { return false; }

    int			prefHNrPics() const;
    virtual void	setPrefWidth(int);
    void		setPrefWidthInChar(int);
    void		setPrefWidthInChar(float);
    void		setMinimumWidth(int);
    void		setMaximumWidth(int);
    int			prefVNrPics() const;
    virtual void	setPrefHeight(int);
    void		setPrefHeightInChar(int);
    void		setPrefHeightInChar(float);
    void		setMinimumHeight(int);
    void		setMaximumHeight(int);

    void		setStretch(int hor,int ver);
				    //!< see class comments
    void		attach(ConstraintType,int margin=-1);
				    //!< see class comments
    void		attach(ConstraintType,uiObject*,int margin=-1,
				bool reciprocal=true);
    void		attach(ConstraintType,uiParent*,int margin=-1,
				bool reciprocal=true);

    static void		setTabOrder(uiObject* first, uiObject* second);

    void		setFont(const uiFont&);
    const uiFont*	font() const;

    uiMainWin*		mainwin();
    uiParent*		parent()			{ return parent_; }
    const uiParent*	parent() const
			    { return const_cast<uiObject*>(this)->parent(); }

protected:
			uiObject(uiParent*,const char* nm);
			uiObject(uiParent*,const char* nm,uiObjectBody&);

    uiString		tooltip_;
    uiObjEventFilter*	uiobjeventfilter_;

    virtual bool	closeOK()	{ closed.trigger(); return true; }
			//!< hook. Accepts/denies closing of window.

    void		triggerSetGeometry(const i_LayoutItem*,uiRect&);
			//<! should be triggered by this's layoutItem
    void		updateToolTip(CallBacker* = 0);

private:

    uiParent*		parent_;

public:

    /// Following functions are rarely useful for everyday programming

    int			getNrWidgets() const	{ return 1; }
    mQtclass(QWidget*)	getWidget(int);

    virtual bool	handleLongTabletPress();

    virtual const ObjectSet<uiBaseObject>* childList() const	{ return 0; }

    Notifier<uiObject>	closed;
			//!< Triggered when object closes.
    void		close();

    CNotifier<uiObject,uiRect&>	setGeometry;
			/*!< triggered when getting a new geometry
			    A reference to the new geometry is passed
			    which *can* be manipulated, before the
			    geometry is actually set to the QWidget. */

    void		display(bool yn,bool shrink,bool maximized=false);
    void		reParent(uiParent*);
    void		shallowRedraw(CallBacker* =0)	{ reDraw( false ); }
    void		deepRedraw(CallBacker* =0)	{ reDraw( true ); }
    void		reDraw(bool deep);
    void		setCaption(const uiString&);
    virtual void	setStyleSheet(const char*);
    virtual void	setCursor(const MouseCursor&);
    bool		isCursorInside() const;

    static int		baseFldSize();
    static int		toolButtonSize();
    mDeprecated inline static int iconSize() { return toolButtonSize(); }

};


#define mTemplTypeDef(fromclass,templ_arg,toclass) \
	typedef fromclass<templ_arg> toclass;
#define mTemplTypeDefT(fromclass,templ_arg,toclass) \
	mTemplTypeDef(fromclass,templ_arg,toclass)


#define mUsrEvGuiThread	   mQtclass(QEvent)::Type( mQtclass(QEvent)::User + 0 )
#define mUsrEvPopUpReady   mQtclass(QEvent)::Type( mQtclass(QEvent)::User + 1 )
#define mUsrEvLongTabletPress  mQtclass(QEvent)::Type(mQtclass(QEvent)::User+2)
