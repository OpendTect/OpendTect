#ifndef I_LAYOUTITEM_H
#define I_LAYOUTITEM_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          29/06/2001
 RCS:           $Id: i_layoutitem.h,v 1.3 2001-08-30 10:49:59 arend Exp $
________________________________________________________________________

-*/

#include "i_layout.h"
#include "uiobjbody.h"
#include "uihandle.h"
#include <qsize.h>

class QLayoutItem;

//! Wrapper around QLayoutItem class. Stores some dGB specific layout info.
class i_LayoutItem : public uiBody
{   
    friend class		i_LayoutMngr;
    friend class		i_LayoutIterator;

public: 
				i_LayoutItem( i_LayoutMngr& , QLayoutItem& );

    virtual			~i_LayoutItem();



    virtual int			horAlign() const 
				    { return pos().left(); }
    virtual int			horCentre() const 
				    { return ( pos().left() 
					     + pos().right() ) / 2; 
				    }

    virtual QSize 		minimumSize() const 
				    { return qwidget()->minimumSize(); }

    QSize			sizeHint() const
				    { return mQLayoutItem().sizeHint(); }

    virtual void       		invalidate();

    uiSize			actualSize( bool include_border = true) const;

    inline const i_LayoutMngr& 	mngr() const 	{ return mngr_; } 

    inline const uiRect& 	pos() const 	{ return pos_[mngr_.curMode()];}
    inline uiRect&		pos() 		{ return pos_[mngr_.curMode()];}

    constraintIterator		iterator();

protected:

    int 			stretch( bool hor );
    void			commitGeometrySet();

    void			initLayout ( int mngrTop, int mngrLeft );
    void			layout ();

    void 			updated() const	{ mngr_.childUpdated();}

    void			attach( constraintType, 
					i_LayoutItem *other, int margin);

    virtual uiObject*		obj2Layout()		{ return 0; }
    virtual uiObjectBody*	bodyLayouted()		{ return 0; }

    inline QLayoutItem&		mQLayoutItem()		{ return mQLayoutItem_;}
    inline const QLayoutItem&	mQLayoutItem() const 	{ return mQLayoutItem_;}

    virtual const QWidget*	qwidget_() const 
				    { return mQLayoutItem_.widget(); }
    virtual const QWidget*	managewidg_() const 
				    { return mQLayoutItem_.widget(); }


private:

    QLayoutItem&		mQLayoutItem_;
    i_LayoutMngr&		mngr_;

    constraintList		constrList;

    uiRect			pos_[ nLayoutMode ];
    bool			preferred_pos_inited;
    bool			minimum_pos_inited;

};

//! Wrapper around QLayoutItems that have been wrapped by a i_QObjWrp wrapper and therefore have a reference to a uiObject.
class i_uiLayoutItem : public i_LayoutItem
{
public:
			i_uiLayoutItem( i_LayoutMngr& mngr, uiObjectBody& obj )
			    : i_LayoutItem( mngr, 
					*new QWidgetItem(obj.qwidget()) )
			    , uiObjBody_( obj ) 
			    {}

    virtual QSize 	minimumSize() const
			    { 
				uiSize s =  uiObjBody_.minimumSize();
				if( s.width()!=mUndefIntVal )  
				    return QSize( s.width() , s.height() ); 

				return i_LayoutItem::minimumSize();
			    }

    virtual uiObject*	  obj2Layout()	{ return &uiObjBody_.uiObjHandle(); }
    virtual uiObjectBody* bodyLayouted(){ return &uiObjBody_; }

protected:

    uiObjectBody&	uiObjBody_;

};

#endif
