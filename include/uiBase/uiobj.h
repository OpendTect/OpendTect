#ifndef uiobj_H
#define uiobj_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/08/1999
 RCS:           $Id: uiobj.h,v 1.3 2001-02-16 17:01:41 arend Exp $
________________________________________________________________________

-*/


#include "uidobj.h"
#include "uilayout.h"
#include "uigeom.h"
#include "color.h"
#include "errh.h"

#include <stdlib.h>

#define mTemplTypeDef(fromclass,templ_arg,toclass) \
	typedef fromclass<templ_arg> toclass;
#define mTemplTypeDefT(fromclass,templ_arg,toclass) \
	mTemplTypeDef(fromclass,templ_arg,toclass)

class QWidget;
class i_LayoutMngr;
class i_LayoutItem;
class uiFont;
class i_QObjWrp;
class uiButtonGroup;
class Timer;

template <class T> class i_QObjWrapper;

class uiObject : public UserIDObject 
{
friend class 		i_LayoutMngr;
friend class 		i_LayoutItem;
friend class 		i_uiLayoutItem; 
friend class 		uiGroup;
friend class 		uiMainWin;
template <class T> friend i_QObjWrapper<T>;

protected:
			uiObject( uiObject* parnt = 0, 
				  const char* nm = "uiObject" );

public:
    virtual		~uiObject();

    void		setToolTip(const char*);
    static void		enableToolTips(bool yn=true);
    static bool		toolTipsEnabled();


    inline void		display( bool yn = true )
			{ if ( yn ) show(); else hide(); }
    void		show();
    void		hide();
    void		setFocus();

    Color               backgroundColor() const;
    void                setBackgroundColor(const Color&);
    void		setSensitive(bool yn=true);
    bool		sensitive() const;

    virtual int		preferredWidth() const;
    void                setPrefWidth( int w )      
			{ 
			    pref_char_width = -1;
			    pref_width = w; 
			}
    void                setPrefWidthInChar( float w )
			{ 
			    pref_width = -1;
			    pref_char_width = w; 
			}
    virtual int		preferredHeight() const;
    virtual void	setPrefHeight( int h )     
			{ 
			    pref_char_height = -1;
			    pref_height = h; 
			}
    virtual void	setPrefHeightInChar( float h )
			{ 
			    pref_height = -1;
			    pref_char_height = h; 
			}
    void                setStretch( int hor, int ver )
                        { horStretch = hor; verStretch = ver; }

    virtual bool	isSingleLine() const { return false; }

    inline QWidget&	qWidget() 
			{ 
			    if( ! qWidget_() ) 
			    {
				pErrMsg("FATAL: no qWidget_!");
				exit(-1);
			    }
			    return *const_cast<QWidget*>(qWidget_()); 
			}
    inline const QWidget& qWidget() const
			{ 
			    if( ! qWidget_() )
			    {
				pErrMsg("FATAL: no qWidget_!");
				exit(-1);
			    }
			    return *qWidget_(); 
			}

    virtual void	qThingDel( i_QObjWrp* qth )  =0;

    inline uiObject& 	clientWidget()
			{ return const_cast<uiObject&>(clientWidget_()); }
    inline const uiObject& clientWidget() const{ return clientWidget_(); }

    inline QWidget&	clientQWidget() 
			{  return clientWidget().qWidget(); }
    inline const QWidget& clientQWidget() const
			{ return clientWidget().qWidget(); }

    bool		attach ( constraintType, uiObject *other=0, 
				 int margin=-1 );

    void 		setFont( const uiFont& );
    const uiFont*	font() const;

    uiSize		actualSize( bool include_border = true) const;

    void		setCaption( const char* );
    int			borderSpace() const;

    void		shallowRedraw( CallBacker* =0 )	{ forceRedraw_(false); }
    void		deepRedraw( CallBacker* =0 )	{ forceRedraw_(true); }

protected:

    virtual const QWidget*	qWidget_() const 	= 0;
    virtual const uiObject& clientWidget_()const	{ return *this; }


    virtual bool	closeOK() { return true; } 
                        //!< hook. Accepts/denies closing of window.
    virtual void	setGeometry(uiRect) {}
                        //!< hook. Called when geometry on widget is set. 

                        /*! called after a widget has been fully created 
			    and before it is shown the very first time. */ 
    virtual void	polish() {}

    virtual uiSize	minimumSize() const;

    virtual i_LayoutMngr* mLayoutMngr() 
                        { return parent_ ? parent_->mLayoutMngr() : 0; }
                        //!< manager used by children 
    inline const i_LayoutMngr* mLayoutMngr() const
			{ return const_cast<uiObject*>(this)->mLayoutMngr(); }

    virtual i_LayoutMngr* prntLayoutMngr() 
                        { return parent_ ? parent_->mLayoutMngr() : 0; }
                        //!< manager who manages 'this'
    inline const i_LayoutMngr* prntLayoutMngr() const
		    { return const_cast<uiObject*>(this)->prntLayoutMngr(); }

    virtual int		horAlign() const;
    virtual int		horCentre() const;

    virtual void	forceRedraw_( bool deep );

    inline void		finalise() { if(!finalised) finalise_(); }
                        //!< hook. Called for uiGroup and uiDialog by mngr. 
    virtual void	finalise_()
			    { finalised= true; }

    uiObject*       	parent_;
    i_uiLayoutItem*	mLayoutItm; //!< initialised in c'tor of i_LayoutItem

    int			horStretch;
    int			verStretch;

    bool		isHidden;
    int			pref_width;
    float		pref_char_width;
    int			pref_height;
    float		pref_char_height;
    int			cached_pref_width;
    int			cached_pref_height;

    bool		finalised;


private:
    const uiFont*	font_;
};


//! Template implementation of uiObject.
/*!
*/
template <class P, class C>
class uiMltplObj : public P
{
public:
                        uiMltplObj( C* qthng, uiObject* parnt=0, 
				    const char* nm=0, bool addToMngr = true )
                        : P( parnt, nm )
			, qtThing( qthng )
                        {
			    if( qthng && addToMngr && prntLayoutMngr() )
				prntLayoutMngr()->add( qthng );
			}

    virtual		~uiMltplObj() {}

    virtual void	qThingDel( i_QObjWrp* qth ){}

    inline C*		mQtThing() { return qtThing; }
    inline const C*	mQtThing() const { return qtThing; }

protected:
    C*			qtThing;
};



//! Template implementation of uiObject.
/*!
*/
template <class P, class C>
class uiMltplWrapObj : public uiMltplObj<P,C>
{
public:
                        uiMltplWrapObj( C* qthng, uiObject* parnt=0, 
					const char* nm=0, bool addToMngr=true)
			: uiMltplObj<P,C>(qthng, parnt, nm, addToMngr ){}

    virtual		~uiMltplWrapObj()
			{ 
			    if( qtThing )
			    {   // avoid infinite recursion!
				qtThing->clientDel( this );
				delete qtThing;
			    }
			}

    virtual void	qThingDel( i_QObjWrp* qth )
			{
#if 0	
			    if( dynamic_cast< QWidget* >(qth)     != 
				dynamic_cast< QWidget* >(qtThing)    )
			    {
				pErrMsg("Warning: Unkown qtThing delete");
				return;
			    }
#endif
			    qtThing=0;
			}
};

//! Template implementation of uiObject.
/*!
*/
template <class T>
class uiNoWrapObj : public uiMltplObj< uiObject , T >
{
public:
                        uiNoWrapObj( T* qthng, uiObject* parnt, const char* nm, 
			       bool addToMngr=true )
                        : uiMltplObj<uiObject,T>( qthng, parnt, nm, addToMngr )
                        {}
};

//! Template implementation of uiObject.
/*!
*/
template <class T>
class uiWrapObj : public uiMltplWrapObj< uiObject , T >
{
public:
                        uiWrapObj( T* qthng, 
				   uiObject* parnt, const char* nm, 
				   bool addToMngr=true )
                        : uiMltplWrapObj<uiObject, T> 
					    ( qthng, parnt, nm, addToMngr )
                        {}


};
#endif
