#ifndef I_LAYOUT_H
#define I_LAYOUT_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          18/08/1999
 RCS:           $Id: i_layout.h,v 1.13 2001-09-20 08:30:59 arend Exp $
________________________________________________________________________

-*/

#include "uilayout.h"
//#include "i_layoutitem.h"
#include "uigeom.h"
#include "uiobj.h"

#include <qlayout.h>
#include <qlist.h>
#include <qrect.h>

class resizeItem;

//!  internal enum used to determine in which direction a widget can be stretched and to check which outer limit must be checked
enum stretchLimitTp { left=1, right=2, above=4, below=8, 
                      rightLimit=16, bottomLimit=32 };


class uiConstraint
{
friend class i_LayoutItem;
public:

			uiConstraint ( constraintType t, i_LayoutItem* o,
				       int marg )
			: other(o), type(t), margin( marg ), enabled_( true )
			{
			    if( !other &&
				((type < leftBorder)||( type > bottomBorder))
			    )
				{ pErrMsg("No attachment defined!!"); }
			}

    bool		enabled()		{ return enabled_ ; }
    void		disable(bool yn=true)	{ enabled_ = !yn; }

protected:
    constraintType      type;
    i_LayoutItem*       other;
    int                 margin;
    bool		enabled_;
};

mTemplTypeDef(QList,uiConstraint,constraintList)
mTemplTypeDef(QListIterator,uiConstraint,constraintIterator)

class i_LayoutItem;


enum layoutMode { minimum=0, preferred=1, setGeom=2, all=3 };
       // all is used for setting cached positions dirty
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
class i_LayoutMngr : public QLayout, public UserIDObject
{
    friend class	i_LayoutItem;

public:

			i_LayoutMngr( QWidget* prnt,
				      int border, int space,
				      const char* name=0 );

    virtual		~i_LayoutMngr();
 
    virtual void 	addItem( QLayoutItem*);
    void	 	addItem( i_LayoutItem* );

    virtual QSize 	sizeHint() const;
    virtual QSize 	minimumSize() const;

    virtual QLayoutIterator iterator();
    virtual QSizePolicy::ExpandData expanding() const;

    virtual void       	invalidate();
	
    bool 		attach ( constraintType, QWidget&, QWidget*, int);

/*! \brief sets layout item where position is stored. 
    A Layout manager needs positioning storage for itself. In case we're 
    managing a uiGroup, this info is already there in the assicated 
    layoutitem for that group. So, we have to use that position.
*/
#if 0
    void		setLayoutPosItm( i_LayoutItem* itm );

    const uiRect&	pos(layoutMode m) const	
                        { return const_cast<i_LayoutMngr*>(this)->pos(m); }

    uiRect&		pos(layoutMode m)
			{ 
			    if( !layoutpos )
			    {
				pErrMsg("no layout position info");
				layoutpos = new uiRect[nLayoutMode];
			    }
			    return layoutpos[m]; 
			}

#else

    const uiRect&	pos(layoutMode m) const	{ return layoutpos[m]; }
    uiRect&		pos(layoutMode m)	{ return layoutpos[m]; }

#endif

    void		forceChildrenRedraw( uiObjectBody*, bool deep );
    void		childrenClear( uiObject* );
    void		setMinTxtWidgHgt( int h )   { mintxtwidgethgt=h; }
    int			minTxtWidgHgt() const       { return mintxtwidgethgt; }

    int                 childStretch( bool hor ) const;

    int			borderSpace() const	    { return margin(); }
    int			horSpacing() const 	    { return spacing(); }
    int			verSpacing() const 
			{ int s = spacing(); return s > 3 ? s-2 : 2; }
protected:

    void 		setGeometry( const QRect& );
 
private:

    void 		doLayout( layoutMode m, const QRect& );
    inline void 	doLayout( layoutMode m, const QRect& r ) const 
                        { const_cast<i_LayoutMngr*>(this)->doLayout(m,r); }

    void 		moveChildrenTo( int , int, layoutMode );
    void 		fillResizeList( ObjectSet<resizeItem>&, 
					int&, int&, int&, int& );
    void		resizeTo( QRect& );
    void		childrenCommitGeometrySet();

    uiRect 		childrenRect( layoutMode m );
    void 		layoutChildren( layoutMode m );

    QList<i_LayoutItem>	childrenList;

//    uiRect*		layoutpos;
    uiRect		layoutpos[ nLayoutMode ];

    static int		mintxtwidgethgt;

    QRect		prevGeometry;
    bool		minimumDone;
    bool		preferredDone;

};

#endif 
