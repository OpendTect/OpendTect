#ifndef I_LAYOUT_H
#define I_LAYOUT_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          18/08/1999
 RCS:           $Id: i_layout.h,v 1.1 2000-11-27 10:19:26 bert Exp $
________________________________________________________________________

-*/

#include "uilayout.h"
#include "uigeom.h"

#include "uiobj.h"

#include <qlayout.h>
#include <qlist.h>

#include "i_qobjwrap.h"
#include "uigroup.h"

//!  internal enum used to determine in which direction a widget can be stretched and to check which outer limit must be checked
enum stretchLimitTp { left=1, right=2, above=4, below=8, 
                      rightLimit=16, bottomLimit=32 };


class uiConstraint;

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
*/
class i_LayoutMngr : public QLayout, public UserIDObject
{
    friend class	i_LayoutItem;

public:
			i_LayoutMngr( uiObject* parnt, 
				      int border, 
				      int space,
				      const char* name=0 );

//! constructor for if parnt doesn't know it's widget yet (constr. uiDialog)
			i_LayoutMngr( QWidget* prntWidg,
				      uiObject* parnt, 
				      int border, 
				      int space,
				      const char* name=0 );


    virtual		~i_LayoutMngr();
 
    virtual void 	addItem( QLayoutItem*);
    virtual QSize 	sizeHint() const;
    virtual QSize 	minimumSize() const;

    virtual QLayoutIterator iterator();
    virtual QSizePolicy::ExpandData expanding() const;

    virtual void       	invalidate();
	
    bool 		attach ( constraintType, QWidget&, QWidget*, int);

    layoutMode		curMode() const { return curmode; } 
    inline const uiRect& pos() const { return pos_[curMode()]; }
    void		forceChildrenRedraw( uiObject*, bool deep );
    int			borderSpace() const	{ return margin(); }
    int			minTxtWidgHgt() const { return mintextwidgetheight;}

protected:
    void 		setGeometry( const QRect& );
    void		childUpdated() 		{ a_child_updated = true; }
    int			horSpacing() const 	{ return spacing(); }
    int			verSpacing() const 	{ return spacing(); }
    void		setMode( layoutMode m ) { curmode = m; } 
    inline void 	setMode( layoutMode m  ) const 
                        { return const_cast<i_LayoutMngr*>(this)->setMode(m); }
  
private:
    void 		doLayout( const QRect& );
    inline void 	doLayout( const QRect& r ) const 
                        { return const_cast<i_LayoutMngr*>(this)->doLayout(r); }

    uiRect 		childrenRect();
    void 		layoutChildren(int* iterLeft =0);

    QList<i_LayoutItem>	childrenList;

    bool		a_child_updated;

    uiRect		pos_[ nLayoutMode ];

    uiObject*		parnt_;

    layoutMode		curmode;
    int			mintextwidgetheight;
};


//! Wrapper around QLayoutItem class. Stores some dGB specific layout info.
class i_LayoutItem
{   
    friend class	i_LayoutMngr;

public: 
			i_LayoutItem( i_LayoutMngr& , QLayoutItem& );

    virtual		~i_LayoutItem();

    virtual QWidget*   	widget() { return mQLayoutItem_.widget(); }
    const QWidget*  	widget() const
			    { return ((i_LayoutItem*)this)->widget(); }
    inline QLayoutItem& mQLayoutItem()		   { return mQLayoutItem_; }
    inline const QLayoutItem& mQLayoutItem() const { return mQLayoutItem_; }


    virtual int		horAlign() const 
			    { return pos().left(); }
    virtual int		horCentre() const 
			    { return ( pos().left() 
                                     + pos().right() ) / 2; 
			    }
    virtual QSize 	minimumSize() const 
			    { return mQLayoutItem_.widget() -> minimumSize(); }

    virtual void       	invalidate();

    uiSize		actualSize( bool include_border = true) const;

    const i_LayoutMngr& loMngr() const 	{ return mngr; } 
    inline const uiRect& pos() const 	{ return pos_[mngr.curMode()]; }
    inline uiRect&	pos() 		{ return pos_[mngr.curMode()]; }

    constraintIterator	iterator();
protected:

    void		setGeometry();

    void		initLayout ( int mngrTop, int mngrLeft );
    void		layout ();

    void 		updated() const		{ return mngr.childUpdated(); }
    int			horSpacing() const	{ return mngr.horSpacing(); }
    int			verSpacing() const	{ return mngr.verSpacing(); }

    void 		attach(constraintType, i_LayoutItem *other, int margin);

    const int* 		pt_hStretch;
    const int* 		pt_vStretch;

    virtual uiObject*	uiClient() { return 0; }
    const uiObject*  	uiClient() const
			{ return ((i_LayoutItem*)this)->uiClient(); }

private:

    QLayoutItem& 	mQLayoutItem_;   
    i_LayoutMngr&       mngr;

    constraintList	constrList;

    uiRect		pos_[ nLayoutMode ];
    bool		preferred_pos_inited;
    bool		minimum_pos_inited;

};


//! Wrapper around QLayoutItems that have been wrapped by a i_QObjWrp wrapper and therefore have a reference to a uiObject.
class i_uiLayoutItem : public i_LayoutItem
{
public:
			i_uiLayoutItem( uiObject& wrapper, 
					i_LayoutMngr& mngr, 
					QLayoutItem& item )
			: i_LayoutItem( mngr, item )
                        , uiObject_( wrapper ) 
                        {
			    pt_hStretch = &wrapper.horStretch;
			    pt_vStretch = &wrapper.verStretch;
                            wrapper.mLayoutItm = this;
                        }

    virtual int		horAlign() const 
			{ return uiObject_.horAlign(); }

    virtual int		horCentre() const 
			{ return uiObject_.horCentre(); }

    virtual QSize 	minimumSize() const
			{ uiSize s =  uiObject_.minimumSize();
			  return QSize( s.width() , s.height() ); 
			}

    virtual uiObject*	uiClient() { return &uiObject_; }
protected:

    uiObject&		uiObject_;

};

#ifdef __debug__ 
#define mChkmLayout()   if(!mLayoutItm) { pErrMsg("No mLayoutItm"); return 0; }
#else
#define mChkmLayout()	if(!mLayoutItm) { return 0; } 
#endif

#endif
