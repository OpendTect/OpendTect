#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uibody.h"
#include "uifont.h"
#include "uiobj.h"
#include "uiparentbody.h"

#include "color.h"

class i_LayoutItem;
class i_LayoutMngr;
class uiPixmap;
class Timer;

mFDQtclass(QCloseEvent)
mFDQtclass(QFontMetrics)
mFDQtclass(QWidget)

#define USE_DISPLAY_TIMER 1

mExpClass(uiBase) uiObjectBody : public uiBody, public NamedCallBacker
{
friend class		i_uiLayoutItem;

protected:
			uiObjectBody(uiParent*,const char* nm);
public:

    virtual		~uiObjectBody();

    void		setToolTip(const uiString&);

    void		display(bool yn,bool shrink=false,
				bool maximized=false);
    void		uisetFocus();
    bool		uihasFocus() const;
    bool		uiCloseOK()	{ return uiObjHandle().closeOK(); }
    bool		isDisplayed() const { return display_; }

    OD::Color		uibackgroundColor() const;
    void		uisetBackgroundColor(const OD::Color&);
    void		uisetBackgroundPixmap(const uiPixmap&);
    void		uisetTextColor(const OD::Color&);
    void		uisetSensitive(bool yn=true);
    bool		uisensitive() const;
    bool		uivisible() const;

    int			prefHNrPics() const;
    void		setPrefWidth(int);
    float		prefWidthInCharSet() const;
    void		setPrefWidthInChar(float);
    void		setMinimumWidth(int);
    void		setMaximumWidth(int);

    int			prefVNrPics() const;
    void		setPrefHeight(int);
    float		prefHeightInCharSet() const;
    void		setPrefHeightInChar(float);
    void		setMinimumHeight(int);
    void		setMaximumHeight(int);

    void		setStretch(int,int);
    virtual int		stretch(bool,bool retUndef=false) const;

    virtual int		nrTxtLines() const	{ return -1; }

    void		attach(constraintType,uiObject* other=nullptr,
			       int margin=-1,bool reciprocal=true);
    void		attach(constraintType,uiParent* other=nullptr,
			       int margin=-1,bool reciprocal=true);

    void		uisetFont(const uiFont&);
    const uiFont*	uifont() const;

    virtual uiSize	actualSize(bool include_border=true) const;
			//!< Beware! this is during layout only
			//!< use uiObject::width() and height() for 'live' objs

    virtual uiSize	minimumSize() const
			{ return uiSize(mUdf(int),mUdf(int)); }

    mDeprecated("Use setCaption")
    void		uisetCaption( const uiString& capt )
			{ setCaption( capt ); }
    void		setCaption(const uiString&);

    virtual void	reDraw(bool);

    virtual uiObject&	uiObjHandle()		= 0;

    const i_LayoutItem* layoutItem() const	{ return layoutitem_; }
    i_LayoutItem*	mkLayoutItem(i_LayoutMngr&);

    void		finalize() override;
    bool		finalized() const override	{ return finalized_; }

    void		fontchanged() override;

    int			fontHeight() const;
    int			fontWidth(bool max=false) const;
    int			fontWidthFor(const uiString&) const;
    int			fontWidthFor(const char*) const;

    void		setHSzPol(uiObject::SzPolicy);
    void		setVSzPol(uiObject::SzPolicy);
    uiObject::SzPolicy	szPol(bool hor=true) const
			{ return hor ? hszpol_: vszpol_ ; }

    void		setShrinkAllowed( bool yn )	{ allowshrnk_ = yn; }
    bool		shrinkAllowed()			{ return allowshrnk_; }

    bool		isHidden()			{ return is_hidden_; }
    bool		itemInited() const;

    void		reParent( uiParentBody* pb )
			{ if ( pb ) parent_ = pb; }

protected:

    int			hstretch_		= mUdf(int);
    int			vstretch_		= mUdf(int);

    virtual const mQtclass(QWidget*) managewidg_() const { return qwidget_(); }

    virtual i_LayoutItem* mkLayoutItem_(i_LayoutMngr& mngr);

    virtual void	finalize_()		{}

    void		doDisplay(CallBacker*);

    void		loitemDeleted()		{ layoutitem_ = nullptr; }

private:

    i_LayoutItem*	layoutitem_		= nullptr;
    uiParentBody*	parent_;
    const uiFont*	font_			= nullptr;

    bool		allowshrnk_		= false;

    bool		is_hidden_		= false;
    bool		finalized_		= false;
    bool		display_		= true;
    bool		display_maximized_	= false;

    int			pref_width_		= 0;
    int			pref_height_		= 0;

    int			pref_width_set_		= -1;
    float		pref_char_width_	= -1.f;
    int			pref_height_set_	= -1;
    float		pref_char_height_	= -1.f;
    int			pref_width_hint_	= 0;
    int			pref_height_hint_	= 0;

    int			fnt_hgt_		= 0;
    int			fnt_wdt_		= 0;
    int			fnt_maxwdt_		= 0;

    uiObject::SzPolicy	hszpol_			= uiObject::Undef;
    uiObject::SzPolicy	vszpol_			= uiObject::Undef;

    void		gtFntWdtHgt() const;
    void		getSzHint();

#ifdef USE_DISPLAY_TIMER
    Timer*		displaytimer_		= nullptr;
#endif
};
