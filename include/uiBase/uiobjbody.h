#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/06/2001
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

    Color		uibackgroundColor() const;
    void		uisetBackgroundColor(const Color&);
    void		uisetBackgroundPixmap(const uiPixmap&);
    void		uisetTextColor(const Color&);
    void		uisetSensitive(bool yn=true);
    bool		uisensitive() const;
    bool		uivisible() const;

    int			prefHNrPics() const;
    void		setPrefWidth(int);
    float		prefWidthInCharSet() const  { return pref_char_width; }
    void		setPrefWidthInChar(float);
    void		setMinimumWidth(int);
    void		setMaximumWidth(int);

    int			prefVNrPics() const;
    void		setPrefHeight(int);
    float		prefHeightInCharSet() const { return pref_char_height; }
    void		setPrefHeightInChar(float);
    void		setMinimumHeight(int);
    void		setMaximumHeight(int);

    void		setStretch(int,int);
    virtual int		stretch(bool,bool retUndef=false) const;

    virtual int		nrTxtLines() const	{ return -1; }

    void		attach(ConstraintType,uiObject* other=0,
			       int margin=-1,bool reciprocal=true);
    void		attach(ConstraintType t,uiParent* other=0,
			       int m=-1,bool r=true)
			{ attach(t,other->mainObject(),m,r ); }

    void		uisetFont(const uiFont&);
    const uiFont*	uifont() const;

    virtual uiSize	actualSize(bool include_border=true) const;
			//!< Beware! this is during layout only
			//!< use uiObject::width() and height() for 'live' objs

    virtual uiSize	minimumSize() const
			{ return uiSize(mUdf(int),mUdf(int)); }

    void		uisetCaption(const uiString&);

    virtual void	reDraw(bool);

    virtual uiObject&	uiObjHandle()		=0;

    const i_LayoutItem*	layoutItem()		{ return layoutItem_; }
    i_LayoutItem*	mkLayoutItem(i_LayoutMngr&);

    virtual void	finalise();
    virtual bool	finalised() const	{ return finalised_; }

    virtual void	fontchanged();

    int			fontHeight() const;
    int			fontWidth(bool max=false) const;
    int			fontWidthFor(const uiString&) const;
    int mDeprecated	fontWidthFor(const char*) const;

    void		setHSzPol(uiObject::SzPolicy);
    void		setVSzPol(uiObject::SzPolicy);
    uiObject::SzPolicy	szPol(bool hor=true) const
			{ return hor ? hszpol: vszpol ; }

    void		setShrinkAllowed(bool yn) { allowshrnk = yn; }
    bool		shrinkAllowed()		{ return allowshrnk; }

    bool		isHidden()		{ return is_hidden; }
    bool		itemInited() const;

    void		reParent(uiParentBody* pb)
			{ if ( pb ) parent_ = pb; }

protected:

    int			hStretch;
    int			vStretch;

    virtual const mQtclass(QWidget*) managewidg_() const { return qwidget_(); }

    virtual i_LayoutItem* mkLayoutItem_(i_LayoutMngr& mngr);

    virtual void	finalise_()             {}

    void		doDisplay(CallBacker*);

    void		loitemDeleted()		{ layoutItem_ = 0; }

private:

    i_LayoutItem*	layoutItem_;
    uiParentBody*	parent_;
    const uiFont*	font_;

    bool		allowshrnk;

    bool		is_hidden;
    bool		finalised_;
    bool		display_;
    bool		display_maximized;

    int			pref_width_;
    int			pref_height_;

    int			pref_width_set;
    float		pref_char_width;
    int			pref_height_set;
    float		pref_char_height;
    int			pref_width_hint;
    int			pref_height_hint;

    int			fnt_hgt;
    int			fnt_wdt;
    int			fnt_maxwdt;
    mQtclass(QFontMetrics*) fm;

    uiObject::SzPolicy	hszpol;
    uiObject::SzPolicy	vszpol;

    void		gtFntWdtHgt() const;
    void		getSzHint();

#ifdef USE_DISPLAY_TIMER
    Timer&		displaytimer;
#endif
};
