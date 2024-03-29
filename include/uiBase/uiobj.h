#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uibaseobject.h"

#include "uigeom.h"
#include "uilayout.h"
#include "uistring.h"

#include "color.h"

mFDQtclass(QWidget)
mFDQtclass(QString)

class MouseCursor;
class i_LayoutItem;

class uiFont;
class uiMainWin;
class uiObjEventFilter;
class uiObjectBody;
class uiParent;
class uiPixmap;


/*!
\brief The base class for most UI elements.
*/

mExpClass(uiBase) uiObject : public uiBaseObject
{
    friend class	uiObjectBody;
    friend class	i_LayoutItem;

public:
			~uiObject();
			mOD_DisableCopy(uiObject)

/*! \brief How should the object's size behave?
    Undef       : use default.
    Small       : 1 base sz.
    Medium      : 2* base sz + 1.
    Wide        : 4* base sz + 3.
    The xxVar options specify that the element may have a bigger internal
    preferred size. In that case, the maximum is taken.
    The xxMax options specify that the element should take all available
    space ( stretch = 2 )
*/
    enum		SzPolicy{ Undef, Small, Medium, Wide,
				  SmallVar, MedVar, WideVar,
				  SmallMax, MedMax, WideMax };

    void		setHSzPol(SzPolicy);
    void		setVSzPol(SzPolicy);
    SzPolicy		szPol( bool hor=true) const;

    virtual int		width() const;	//!< Actual size in pixels
    virtual int		height() const;	//!< Actual size in pixels

    virtual void	setName(const char*) override;

    void		setToolTip(const uiString&);
    const uiString&	toolTip() const;
    static void		updateToolTips();

    void		translateText() override;

    void		display(bool yn,bool shrink=false,bool maximized=false);
    void		setFocus();
    bool		hasFocus() const;
    void		disabFocus();

    virtual void	setCursor(const MouseCursor&);
    bool		isCursorInside() const;

    virtual void	setStyleSheet(const char*);
    virtual OD::Color	backgroundColor() const;
    OD::Color		roBackgroundColor() const;
    virtual void	setBackgroundColor(const OD::Color&);
    virtual void	setBackgroundPixmap(const uiPixmap&);
    virtual void	setTextColor(const OD::Color&);
    void		setSensitive(bool yn=true);
    bool		isSensitive() const	{ return sensitive(); }
    bool		sensitive() const;
    bool		visible() const;
    bool		isDisplayed() const;
    virtual bool	isEmpty() const { return false; }

    int			prefHNrPics() const;
    virtual void	setPrefWidth(int);
    virtual void	setPrefWidthInChar(int);
    void		setPrefWidthInChar(float);
    void		setMinimumWidth(int);
    void		setMaximumWidth(int);
    void		setMinimumWidthInChar(int);
    void		setMaximumWidthInChar(int);

    int			prefVNrPics() const;
    virtual void	setPrefHeight(int);
    virtual void	setPrefHeightInChar(int);
    void		setPrefHeightInChar(float);
    void		setMinimumHeight(int);
    void		setMaximumHeight(int);
    void		setMinimumHeightInChar(int);
    void		setMaximumHeightInChar(int);

/*! \brief Sets stretch factors for object
    If stretch factor is > 1, then object will already grow at pop-up.
*/
    void		setStretch(int hor,int ver);
    int			stretch(bool hor) const;


/*! \brief attaches object to another
    In case the stretched... options are used, margin=-1 (default) stretches
    the object not to cross the border.
    margin=-2 stretches the object to fill the parent's border. This looks nice
    with separators.
*/
    void		attach(ConstraintType,int margin=-1);
    void		attach(ConstraintType,uiObject*,int margin=-1,
				bool reciprocal=true);
    void		attach(ConstraintType,uiParent*,int margin=-1,
				bool reciprocal=true);

    static void		setTabOrder(uiObject* first,uiObject* second);

    void		setFont(const uiFont&);
    const uiFont*	font() const;
    void		setCaption(const uiString&);


    void		shallowRedraw()		{ reDraw( false ); }
    void		deepRedraw()		{ reDraw( true ); }
    void		reDraw(bool deep);

    uiSize		actualSize(bool include_border=true) const;

    uiParent*		parent()			{ return parent_; }
    const uiParent*	parent() const
			    { return const_cast<uiObject*>(this)->parent(); }
    void		reParent(uiParent*);

    uiMainWin*		mainwin();

    mQtclass(QWidget*)	getWidget() override { return qwidget(); }
    mQtclass(QWidget*)	qwidget();
    const mQtclass(QWidget*)	qwidget() const
			{ return const_cast<uiObject*>(this)->qwidget(); }

    virtual bool	handleLongTabletPress();

    virtual const ObjectSet<uiBaseObject>* childList() const { return nullptr; }

    Notifier<uiObject>	closed;
			//!< Triggered when object closes.
    void		close();


			/*! \brief triggered when getting a new geometry
			    A reference to the new geometry is passed
			    which *can* be manipulated, before the
			    geometry is actually set to the QWidget.
			*/
    CNotifier<uiObject,uiRect&>	setGeometry;

    static int		baseFldSize();
    static int		iconSize();

protected:
			uiObject(uiParent*,const char* nm);
			uiObject(uiParent*,const char* nm,uiObjectBody&);

    uiObjectBody*	objBody();
    const uiObjectBody* objBody() const;

			//! hook. Accepts/denies closing of window.
    virtual bool	closeOK()	{ closed.trigger(); return true; }

			//! setGeometry should be triggered by this's layoutItem
    void		triggerSetGeometry(const i_LayoutItem*,uiRect&);

    void		updateToolTip(CallBacker* = nullptr);

    uiString		tooltip_;

    uiObjEventFilter*	uiobjeventfilter_;

private:
    uiParent*		parent_;

};


#define mTemplTypeDef(fromclass,templ_arg,toclass) \
	typedef fromclass<templ_arg> toclass;
#define mTemplTypeDefT(fromclass,templ_arg,toclass) \
	mTemplTypeDef(fromclass,templ_arg,toclass)


#define mUsrEvGuiThread	   mQtclass(QEvent)::Type( mQtclass(QEvent)::User + 0 )
#define mUsrEvPopUpReady   mQtclass(QEvent)::Type( mQtclass(QEvent)::User + 1 )
#define mUsrEvLongTabletPress  mQtclass(QEvent)::Type(mQtclass(QEvent)::User+2)
