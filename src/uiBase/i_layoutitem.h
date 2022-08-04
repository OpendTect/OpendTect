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

    virtual int			horAlign(LayoutMode m ) const
				    { return curpos(m).left(); }
    virtual int			center(LayoutMode,bool hor=true) const;
    virtual uiSize		minimumSize() const;
    uiSize			prefSize() const;

    virtual void		invalidate();
    virtual void		updatedAlignment(LayoutMode)	{}
    virtual void		initChildLayout(LayoutMode)	{}

    uiSize			actualSize(bool include_border = true) const;
				//!< live objs: use uiObject::width() etc

    inline const i_LayoutMngr&	mngr() const 		{ return *mngr_; }

    inline const uiRect&	curpos(LayoutMode m) const
				{ return layoutpos_[m];}
    inline uiRect&		curpos(LayoutMode m)
				{ return layoutpos_[m];}

    bool			inited() const;

protected:

    bool			preferred_pos_inited_		= false;
    bool			minimum_pos_inited_		= false;

    uiRect			layoutpos_[nLayoutMode];

    int				stretch(bool hor) const;
    virtual void		commitGeometrySet(bool);

    void			initLayout(LayoutMode,int mngrtop,int mngrleft);
    bool			layout(LayoutMode,int,bool);

    void			attach(constraintType,i_LayoutItem* other,
				       int margin,bool reciprocal=true);

    virtual uiObject*		objLayouted()		{ return nullptr; }
    const uiObject*		objLayouted() const;
    virtual uiObjectBody*	bodyLayouted()		{ return nullptr; }
    const uiObjectBody*		bodyLayouted() const;

    mQtclass(QLayoutItem&)		qlayoutItm();
    const mQtclass(QLayoutItem&)	qlayoutItm() const;
    mQtclass(QLayoutItem*)		takeQlayoutItm();

    const mQtclass(QWidget*)		qwidget_() const override;
    virtual const mQtclass(QWidget*)	managewidg_() const;

    inline i_LayoutMngr&	mngr()			{ return *mngr_; }

    bool			isAligned() const;

private:

    mQtclass(QLayoutItem*)	qlayoutitm_;
    i_LayoutMngr*		mngr_;

    void			managerDeletedCB(CallBacker*);

    TypeSet<uiConstraint>	constraintlist_;

#ifdef __debug__
    int				isPosOk(uiConstraint*,int,bool);
#endif
    bool			prefszdone_		= false;
    uiSize			prefsz_;
    bool			hsameas_		= false;
    bool			vsameas_		= false;
};


//! Wrapper around QLayoutItems that have been wrapped by a i_QObjWrp wrapper
//! and therefore have a reference to a uiObject.
mExpClass(uiBase) i_uiLayoutItem : public i_LayoutItem
{
public:
			i_uiLayoutItem( i_LayoutMngr& mgr, uiObjectBody& obj )
			    : i_LayoutItem(mgr,
				    *new mQtclass(QWidgetItem)(obj.qwidget()) )
			    , uiobjbody_(obj)		{}

    virtual		~i_uiLayoutItem();

    uiSize		minimumSize() const override;

    uiObject*		objLayouted() override
			{ return &uiobjbody_.uiObjHandle(); }

    uiObjectBody*	bodyLayouted() override		{ return &uiobjbody_; }


protected:

    uiObjectBody&	uiobjbody_;


};
