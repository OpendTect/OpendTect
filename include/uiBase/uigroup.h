#ifndef uigroup_H
#define uigroup_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uigroup.h,v 1.5 2001-05-16 14:58:42 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>

class QWidget;
template <class T> class i_QObjWrapper;
mTemplTypeDefT( i_QObjWrapper, QWidget, i_QWidget )

class i_LayoutMngr;

class uiGroup : public uiWrapObj<i_QWidget>
{ 	
    friend class 	uiMainWin;
    friend class 	uiDialog;
    friend class 	i_LayoutMngr;
public:
			uiGroup( uiParent* , const char* nm="uiGroup", 
				 int border=0, int spacing=10);
protected:
			uiGroup( const char* nm, uiParent*, 
				 int border=10, int spacing=10);
			//!< C'tor for creating a client widget in a window.
public:
    virtual		~uiGroup();

    void		setSpacing( int ); 
    void		setBorder( int ); 
    virtual void	clear();

    void		setHAlignObj( uiObject* o ) 	{ hAlignObj = o;}
    void		setHCentreObj( uiObject* o ) 	{ hCentreObj = o;}
protected:

    const QWidget*	qWidget_() const;

    virtual i_LayoutMngr* mLayoutMngr() { return &loMngr; }
    virtual i_LayoutMngr* prntLayoutMngr()
			{ 
			    return ( parent_ && parent_->mLayoutMngr() )
					? parent_->mLayoutMngr() : &loMngr; 
			}
   
    i_LayoutMngr& 	loMngr;

    virtual void        forceRedraw_( bool deep );
    virtual void	finalise_();

    uiObject*		hCentreObj;
    uiObject*		hAlignObj;

private:

    virtual int         horAlign() const;
    virtual int         horCentre() const;

};


#endif
