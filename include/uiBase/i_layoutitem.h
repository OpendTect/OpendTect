#ifndef I_LAYOUTITEM_H
#define I_LAYOUTITEM_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          29/06/2001
 RCS:           $Id: i_layoutitem.h,v 1.24 2003-11-07 12:21:53 bert Exp $
________________________________________________________________________

-*/

#include "i_layout.h"
#include "uiobjbody.h"
#include "uihandle.h"
#include <qsize.h>

class QLayoutItem;

//! Wrapper around QLayoutItem class. Stores some dGB specific layout info.
class i_LayoutItem : public uiBody, public UserIDObject
{   
    friend class		i_LayoutMngr;
    friend class		i_LayoutIterator;
#ifdef __debug__
    friend class		uiGroupParentBody;
#endif

public: 
				i_LayoutItem( i_LayoutMngr& , QLayoutItem& );

    virtual			~i_LayoutItem();


    virtual int			horAlign(layoutMode m ) const
				    { return curpos(m).left(); }
    virtual int			centre(layoutMode m, bool hor=true) const 
				    { 
					if( hor ) return ( curpos(m).left() 
					     + curpos(m).right() ) / 2; 
					return ( curpos(m).top()
                                             + curpos(m).bottom() ) / 2;
				    }

    virtual uiSize 		minimumsize() const 
				    {
					QSize s =  qwidget()->minimumSize();

					return
					    uiSize( s.width(), s.height(),true);
				    }

    uiSize			prefSize() const
				{ 
				    if( prefSzDone ) 
				       { pErrMsg("PrefSize already done.");}
				    else
				    {
					i_LayoutItem* self =
					    const_cast<i_LayoutItem*>(this);
					self->prefSzDone = true;
					QSize ps( mQLayoutItem().sizeHint() );
					self->prefSz = 
					    uiSize(ps.width(),ps.height(),true);
				    }

				    return prefSz;
				}

    virtual void       		invalidate();
    virtual void       		updatedAlignment(layoutMode)	{}
    virtual void       		initChildLayout(layoutMode)	{}

    uiSize			actualsize( bool include_border = true) const;

    inline const i_LayoutMngr& 	mngr() const 		{ return mngr_; } 

    inline const uiRect& 	curpos(layoutMode m) const
				    { return layoutpos[m];}
    inline uiRect&		curpos(layoutMode m)	{ return layoutpos[m];}

    bool			inited() const 
				{ 
				    return minimum_pos_inited 
					|| preferred_pos_inited; 
				}

protected:

    bool			preferred_pos_inited;
    bool			minimum_pos_inited;

    uiRect			layoutpos[ nLayoutMode ];

    int 			stretch( bool hor ) const;
    virtual void		commitGeometrySet(bool);

    void			initLayout( layoutMode m, int mngrTop, 
							  int mngrLeft );
    bool			layout( layoutMode m, const int, bool );

    void			attach( constraintType, 
					i_LayoutItem *other, int margin,
					bool reciprocal=true);

    virtual uiObject*		objLayouted()		{ return 0; }
    inline const uiObject*	objLayouted() const
				{ 
				    return const_cast<i_LayoutItem*>
							(this)->objLayouted(); 
				}

    virtual uiObjectBody*	bodyLayouted()		{ return 0; }
    inline const uiObjectBody*	bodyLayouted() const
				{ 
				    return const_cast<i_LayoutItem*>
							(this)->bodyLayouted(); 
				}

    inline QLayoutItem&		mQLayoutItem()		{ return mQLayoutItem_;}
    inline const QLayoutItem&	mQLayoutItem() const 	{ return mQLayoutItem_;}

    virtual const QWidget*	qwidget_() const 
				    { return mQLayoutItem_.widget(); }
    virtual const QWidget*	managewidg_() const 
				    { return mQLayoutItem_.widget(); }

    inline i_LayoutMngr& 	mngr()			{ return mngr_; } 

#ifdef __debug__
    bool			isAligned() const;
#endif

private:

    QLayoutItem&		mQLayoutItem_;
    i_LayoutMngr&		mngr_;

    constraintList		constrList;

#ifdef __debug__
    int 			isPosOk( uiConstraint*, int, bool );
#endif
    bool			prefSzDone;
    uiSize			prefSz;
    bool			hsameas;
    bool			vsameas;
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

    virtual		~i_uiLayoutItem();

    virtual uiSize 	minimumsize() const
			    { 
				uiSize s =  uiObjBody_.minimumsize();
				if( s.hNrPics()!=mUndefIntVal )  
				    return s;

				return i_LayoutItem::minimumsize();
			    }

    virtual uiObject*	  objLayouted()	{ return &uiObjBody_.uiObjHandle(); }
    virtual uiObjectBody* bodyLayouted(){ return &uiObjBody_; }


protected:

    uiObjectBody&	uiObjBody_;


};

#endif
