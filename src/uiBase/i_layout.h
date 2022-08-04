#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          18/08/1999
________________________________________________________________________

-*/

#include "uilayout.h"
#include "uigeom.h"
#include "uiobj.h"
#include "sets.h"

#include <QLayout>
#include <QRect>


class ResizeItem;
class Timer;

//! internal enum used to determine in which direction a widget

//! can be stretched and to check which outer limit must be checked
enum stretchLimitTp { left=1, right=2, above=4, below=8,
		      rightLimit=16, bottomLimit=32 };


class i_LayoutItem;

// all is used for setting cached positions dirty
enum LayoutMode { minimum=0, preferred=1, setGeom=2, all=3 };
const int nLayoutMode = 3;

//! dGB's layout manager
/*!
    This is our own layout manager for Qt. It manages widgets, etc. using
    constraints like "rightOf" etc.

Because the i_LayoutMngr is a QLayout, it can be used by
QWidgets to automatically add new children to the manager when they are
constructed with a QWidget (with layout) as parent.

The actual adding to a manager is is done using QEvents.
Whenever a QObject inserts a new child, it posts a ChildInserted event
to itself. However, a QLayout constructor installs an event filter on its
parent, and it registers itself to the parent as its layouter
(setWidgetLayout), so future calls to the parent's sizeHint(), etc. are
redirected to this new layoutmanager.

If setAutoAdd() is called on a layoutmanager, and the layout manager is
"topLevel", i.e. THE manager for a certain widget, then whenever a new
widget is constructed with the manager's parent widget as parent,
the new widget is automatically added (by Qt) to the manager by the manager's
eventfilter, using 'addItem( new QWidgetItem( w ) )'.
Unfortunately, Qt does not call addItem before the main application loop
is running. This results to the problem that no attachments can be used until
the main loop is running when we let Qt handle the addItem() calls.
Therefore, we explicitily call addItem() on the correct layout manager at
construction uiObjects. AutoAdd is also enabled in case someone wants to
eses native Qt methods. (Multiple insertion is protected. Manager checks if
widget already present).


*/
class i_LayoutMngr : public mQtclass(QLayout), public NamedCallBacker
{
    friend class	i_LayoutItem;
    friend class	uiGroupParentBody;

public:
			i_LayoutMngr(mQtclass(QWidget*) prnt,
				     const char* name,uiObjectBody& mngbdy);

    virtual		~i_LayoutMngr();

    void		addItem(mQtclass(QLayoutItem*)) override;
    void		addItem(i_LayoutItem*);

    mQtclass(QSize)	sizeHint() const override;
    mQtclass(QSize)	minimumSize() const override;

    mQtclass(QLayoutItem*) itemAt(int idx) const override;
    mQtclass(QLayoutItem*) takeAt(int idx) override;
    int			count() const override;

    void		invalidate() override;
    virtual void	updatedAlignment(LayoutMode);
    virtual void	initChildLayout(LayoutMode);

    bool		attach(constraintType,mQtclass(QWidget&),
			       mQtclass(QWidget*),int,
			       bool reciprocal=true);

    const uiRect&	curpos(LayoutMode) const;
    uiRect&		curpos(LayoutMode);
    uiRect		winpos(LayoutMode) const;

    void		forceChildrenRedraw(uiObjectBody*,bool deep);
    void		childrenClear(uiObject*);
    bool		isChild(uiObject*);

    int			childStretch(bool hor) const;

    int			borderSpace() const	{ return borderspc_; }
    int			horSpacing() const;
    int			verSpacing() const	{ return vspacing_; }

    void		setHSpacing( int s )	{ hspacing_ = s; }
    void		setVSpacing( int s )	{ vspacing_ = s; }
    void		setBorderSpace( int s )	{ borderspc_ = s; }

    void		setIsMain( bool yn )	{ ismain_ = yn; }
    void		layoutChildren(LayoutMode,bool finalLoop=false);

private:

    void		setGeometry(const mQtclass(QRect&)) override;

    inline void		doLayout( LayoutMode m, const mQtclass(QRect&) r ) const
			{ const_cast<i_LayoutMngr*>(this)->doLayout(m,r); }
    void		doLayout(LayoutMode,const mQtclass(QRect&));

    void		itemDel(CallBacker*);

    void		moveChildrenTo(int,int,LayoutMode);
    void		fillResizeList( ObjectSet<ResizeItem>&, bool );
    bool		tryToGrowItem(ResizeItem&,int,int,
				      int,int,const mQtclass(QRect&),int);
    void		resizeTo(const mQtclass(QRect&));
    void		childrenCommitGeometrySet(bool);
    bool		isMainWinFinalized() const;

    uiRect		childrenRect(LayoutMode);

    ObjectSet<i_LayoutItem> childrenlist_;

    uiRect		layoutpos_[nLayoutMode];
    mQtclass(QRect)	prefgeometry_;

    bool		minimumdone_		= false;
    bool		preferreddone_		= false;
    bool		prefposstored_		= false;
    bool		ismain_			= false;

    int			hspacing_		= -1;
    int			vspacing_		= 2;
    int			borderspc_		= 0;

    uiObjectBody&	managedbody_;

    void		startPoptimer();
    void		popTimTick(CallBacker*);
    Timer&		poptimer_;
    bool		poppedup_		= false;
    bool		timerrunning_		= false;

    friend class i_LayoutItem;
};
