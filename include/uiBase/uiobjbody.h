#ifndef uiobjbody_H
#define uiobjbody_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/06/2001
 RCS:           $Id: uiobjbody.h,v 1.6 2001-09-26 14:47:42 arend Exp $
________________________________________________________________________

-*/


#include "uibody.h"
#include "uiparentbody.h"
#include "uiobj.h"
#include "uifont.h"
//#include "i_layout.h"


class uiButtonGroup;
class QWidget;
class QCloseEvent;
class i_LayoutItem;
class i_LayoutMngr;

class uiObjectBody : public uiBody
{
//friend class 		i_LayoutMngr;
//friend class 		i_LayoutItem;
//friend class 		i_uiLayoutItem; 
//friend class 		uiGroup;
//friend class 		uiMainWin;

protected:
				uiObjectBody( uiParent* );
public:

    virtual			~uiObjectBody();

    void			setToolTip(const char*);
    static void			enableToolTips(bool yn=true);
    static bool			toolTipsEnabled();

    virtual void		uiShow();
    virtual void		uiHide(bool shrink);
    void			uisetFocus();
    bool			uiCloseOK()	{ return uiObjHandle().closeOK(); }

    Color              		uibackgroundColor() const;
    void              		uisetBackgroundColor(const Color&);
    void			uisetSensitive(bool yn=true);
    bool			uisensitive() const;

    virtual int			prefHNrPics() const;
    void			setPrefWidth( int w )      
				{ 
				    pref_char_width = -1;
				    pref_width = w; 
				}
    void			setPrefWidthInChar( float w )
				{ 
				    pref_width = -1;
				    pref_char_width = w; 
				}

    virtual int			prefVNrPics() const;
    void			setPrefHeight( int h )     
				{ 
				    pref_char_height = -1;
				    pref_height = h; 
				}
    void			setPrefHeightInChar( float h )
				{ 
				    pref_height = -1;
				    pref_char_height = h; 
				}

    void               		setStretch( int hor, int ver ) 
				    { hStretch = hor; vStretch = ver; }

    virtual int			stretch( bool hor, bool retUndef=false ) const
				{ 
				    int s = hor ? hStretch : vStretch;
				    if( retUndef ) return s;
				    return s != mUndefIntVal ? s : 0;
				}

    virtual bool		isSingleLine() const { return false; }

    void			attach ( constraintType, uiObject *other=0, 
				 int margin=-1 );

    void 			uisetFont( const uiFont& );
    const uiFont*		uifont() const;

    virtual uiSize		actualSize( bool include_border = true) const;
    virtual uiSize		minimumSize() const
				    { return uiSize(mUndefIntVal,mUndefIntVal);}

    void			uisetCaption( const char* );

    virtual void		reDraw( bool deep );

    virtual uiObject&		uiObjHandle()			=0;

    const i_LayoutItem*		layoutItem()		{ return layoutItem_; }
    i_LayoutItem*		mkLayoutItem( i_LayoutMngr& mngr )
				{ 
				    if( layoutItem_ ) 
				    { 
					pErrMsg("ALreadt have a layout item"); 
					return layoutItem_ ;
				    }
				    layoutItem_ = mkLayoutItem_( mngr );
				    return layoutItem_;
				}

    virtual void		finalise() 
				{ 
				    uiObjHandle().finalising.trigger(
								uiObjHandle()); 
				    finalise_();
				}

protected:

    virtual const QWidget*	managewidg_() const	{ return qwidget_(); }

    virtual i_LayoutItem*	mkLayoutItem_( i_LayoutMngr& mngr );

    virtual void                finalise_()             {}

private:

    i_LayoutItem*		layoutItem_;
    uiParentBody*      		parent_;
    const uiFont*		font_;

    int				hStretch;
    int				vStretch;

    bool			is_hidden;
    int				pref_width;
    float			pref_char_width;
    int				pref_height;
    float			pref_char_height;
    int				cached_pref_width;
    int				cached_pref_height;

};

/*! \brief Default (Template) implementation of uiObjectBody.


    Any uiObjectBody must implement thiswidget_() and uiObjHandle() 
    and must also implement some QWidget methods.
    These are implemented using the pre-processor, because it is difficult
    to templatize for all cases since some Qt objects need a specific 
    constructor.
    However, most QWidgets just need a parent and a name 
    and then this template implementation can be used.

    \code

    #define mHANDLE_OBJ		uiObject_or_child_class, default O
    #define mQWIDGET_BODY       Widget_or_child_class, default T
    #define mQWIDGET_BASE       QWidget
    #include			"i_uiobjqtbody.h"

    \endcode

    The macro's mQWIDGET_BODY and mQWIDGET_BASE are undef'ed 
    in "i_uiobjqtbody.h"

    The "i_uiobjqtbody.h" header file uses no multiple-inclusion protection
    and implements the following methods:

    \code

	virtual QWidget*	qwidget_();
	virtual void		setFont( const QFont &font );
	virtual void		closeEvent( QCloseEvent *e );
	virtual void		polish();

    protected:

	virtual uiObject&	uiObjHandle();

    \endcode

    It also declares a protected member handle_, of type mHANDLE_OBJ&, which 
    must be initialised in any constructor of objects using "i_uiobjqtbody.h"


*/
template <class C, class T>
class uiObjBodyImpl : public uiObjectBody, public T
{
public:

                        uiObjBodyImpl( C& handle, uiParent* parnt, 
				       const char* nm )
			    : uiObjectBody( parnt )
			    , T( parnt && parnt->body() ? 
				     parnt->body()->managewidg() : 0 , nm )
			    , handle_( handle )
			    {}

#include		"i_uiobjqtbody.h"

};



template <class C, class T>
class uiParentObjectTemplateBody : public uiObjectBody, 
				   public uiParentBody, 
				   public T
{
public:

                        uiParentObjectTemplateBody( C& handle, uiParent* parnt, 
						    const char* nm )
			    : uiObjectBody( parnt )
			    , uiParentBody()
			    , T( parnt && parnt->body() ? 
				     parnt->body()->managewidg() : 0 , nm )
			    , handle_( handle )
			    {}

#include		"i_uiobjqtbody.h"

};

#endif
