#ifndef i_layoutitem_h
#define i_layoutitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          29/06/2001
 RCS:           $Id: i_layoutitem.h,v 1.35 2011/04/21 13:09:13 cvsbert Exp $
________________________________________________________________________

-*/

#include "i_layout.h"
#include "uiobjbody.h"

#include <QSize>
#include <QWidget>

class QLayoutItem;

//! Wrapper around QLayoutItem class. Stores some dGB specific layout info.
mClass i_LayoutItem : public uiBody, public NamedObject
{   
    friend class		i_LayoutMngr;
    friend class		i_LayoutIterator;
#ifdef __debug__
    friend class		uiGroupParentBody;
#endif

public: 
				i_LayoutItem(i_LayoutMngr&,QLayoutItem&);
    virtual			~i_LayoutItem();

    virtual int			horAlign(LayoutMode m ) const
				    { return curpos(m).left(); }
    virtual int			centre(LayoutMode m, bool hor=true) const 
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
					    uiSize( s.width(), s.height());
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
					QSize ps( qlayoutItm().sizeHint() );
					int width = ps.width();
					if ( width==0 ) width = 1;
					int height = ps.height();
					if ( height==0 ) height = 1;
					self->prefSz = 
					    uiSize(width,height);
				    }

				    return prefSz;
				}

    virtual void       		invalidate();
    virtual void       		updatedAlignment(LayoutMode)	{}
    virtual void       		initChildLayout(LayoutMode)	{}

    uiSize			actualsize(bool include_border = true) const;
    				//!< live objs: use uiObject::width() etc

    inline const i_LayoutMngr& 	mngr() const 		{ return mngr_; } 

    inline const uiRect& 	curpos(LayoutMode m) const
				    { return layoutpos[m];}
    inline uiRect&		curpos(LayoutMode m)	{ return layoutpos[m];}

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

    void			initLayout( LayoutMode m, int mngrTop, 
							  int mngrLeft );
    bool			layout( LayoutMode m, const int, bool );

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

    inline QLayoutItem&		qlayoutItm()	    { return *qlayoutitm; }
    inline const QLayoutItem&	qlayoutItm() const  { return *qlayoutitm; }

				// Immediately delete me if you take my
				// qlayoutitm !!
    inline QLayoutItem*		takeQlayoutItm()
				{
				    QLayoutItem* ret = qlayoutitm;
				    qlayoutitm = 0;
				    return ret;
				}

    virtual const QWidget*	qwidget_() const 
				    { return qlayoutitm->widget(); }
    virtual const QWidget*	managewidg_() const 
				    { return qlayoutitm->widget(); }

    inline i_LayoutMngr& 	mngr()			{ return mngr_; } 

    bool			isAligned() const;

private:

    QLayoutItem*		qlayoutitm;
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
mClass i_uiLayoutItem : public i_LayoutItem
{
public:
			i_uiLayoutItem( i_LayoutMngr& mgr, uiObjectBody& obj )
			    : i_LayoutItem(mgr,*new QWidgetItem(obj.qwidget()) )
			    , uiObjBody_(obj)		{}

    virtual		~i_uiLayoutItem();

    virtual uiSize 	minimumsize() const
			    { 
				uiSize s = uiObjBody_.minimumsize();
				if ( !mIsUdf(s.hNrPics()) )  
				    return s;

				return i_LayoutItem::minimumsize();
			    }

    virtual uiObject*	  objLayouted()	{ return &uiObjBody_.uiObjHandle(); }
    virtual uiObjectBody* bodyLayouted(){ return &uiObjBody_; }


protected:

    uiObjectBody&	uiObjBody_;


};

#endif
