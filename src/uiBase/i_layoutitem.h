#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          29/06/2001
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "i_layout.h"
#include "uiobjbody.h"

#include <QSize>
#include <QWidget>

mFDQtclass(QLayoutItem)

//! Wrapper around QLayoutItem class. Stores some dGB specific layout info.
mExpClass(uiBase) i_LayoutItem : public uiBody, public NamedCallBacker
{
    friend class		i_LayoutMngr;
    friend class		i_LayoutIterator;
#ifdef __debug__
    friend class		uiGroupParentBody;
#endif

public:
				i_LayoutItem(i_LayoutMngr&,
					     mQtclass(QLayoutItem&));
    virtual			~i_LayoutItem();

    inline const i_LayoutMngr&	mngr() const		{ return mngr_; }

    virtual int			horAlign(LayoutMode) const;
    virtual int			centre(LayoutMode,bool hor=true) const;

    virtual uiSize		minimumSize() const;
    uiSize			prefSize() const;
    virtual void		invalidate();
    virtual void		updatedAlignment(LayoutMode)	{}
    virtual void		initChildLayout(LayoutMode)	{}
    uiSize			actualSize(bool include_border=true) const;
				//!< live objs: use uiObject::width() etc

    const uiRect&		curpos(LayoutMode) const;
    uiRect&			curpos(LayoutMode);

    bool			inited() const;

protected:

    inline i_LayoutMngr&	mngr()			{ return mngr_; }

    int				stretch(bool hor) const;
    virtual void		commitGeometrySet(bool);

    void			initLayout(LayoutMode m,int mngrtop,
							int mngrleft);
    bool			layout(LayoutMode m,int,bool finalloop);
    void			attach(ConstraintType,
					i_LayoutItem* other,int margin,
					bool reciprocal=true);
    bool			isAligned() const;

    virtual uiObject*		objLayouted()		{ return 0; }
    const uiObject*		objLayouted() const;
    virtual uiObjectBody*	bodyLayouted()		{ return 0; }
    const uiObjectBody*		bodyLayouted() const;

    mQtclass(QLayoutItem&)	qlayoutItm();
    const mQtclass(QLayoutItem&) qlayoutItm() const;

				// Immediately delete me if you take my
				// qlayoutitm_ !!
    mQtclass(QLayoutItem*)	takeQlayoutItm();

    virtual const mQtclass(QWidget*)	qwidget_() const;
    virtual const mQtclass(QWidget*)	managewidg_() const;

private:

    mQtclass(QLayoutItem*)	qlayoutitm_;
    i_LayoutMngr&		mngr_;

    uiRect			layoutpos_[nLayoutMode];
    constraintList		constrlist_;

#ifdef __debug__
    int				isPosOk(uiConstraint*,int,bool);
#endif

    bool			preferred_pos_inited_;
    bool			minimum_pos_inited_;
    bool			prefszdone_;
    uiSize			prefsz_;
    bool			hsameas_;
    bool			vsameas_;
};


//! Wrapper around QLayoutItems that have been wrapped by a i_QObjWrp wrapper and therefore have a reference to a uiObject.
mExpClass(uiBase) i_uiLayoutItem : public i_LayoutItem
{
public:
				i_uiLayoutItem(i_LayoutMngr&,uiObjectBody&);
    virtual			~i_uiLayoutItem();

    virtual uiSize		minimumSize() const;

    virtual uiObject*		objLayouted();
    virtual uiObjectBody*	bodyLayouted();

protected:

    uiObjectBody&	uiobjbody_;
};
