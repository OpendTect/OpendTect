#ifndef uiobjbody_H
#define uiobjbody_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/06/2001
 RCS:           $Id: uiobjbody.h,v 1.22 2002-05-17 11:34:54 arend Exp $
________________________________________________________________________

-*/


#include "uibody.h"
#include "uiparentbody.h"
#include "uiobj.h"
#include "uifont.h"
//#include "i_layout.h"

#define USE_DISPLAY_TIMER

class uiButtonGroup;
class QWidget;
class QCloseEvent;
class i_LayoutItem;
class i_LayoutMngr;
class Timer;
class QFontMetrics;

class uiObjectBody : public uiBody, public CallBacker
{
//friend class 		i_LayoutMngr;
//friend class 		i_LayoutItem;
friend class 		i_uiLayoutItem; 
//friend class 		uiGroup;
//friend class 		uiMainWin;

protected:
				uiObjectBody( uiParent* );
public:

    virtual			~uiObjectBody();

    void			setToolTip(const char*);
    static void			enableToolTips(bool yn=true);
    static bool			toolTipsEnabled();

    void 			display( bool yn = true, bool shrink=false,
					 bool maximised=false );
    void			uisetFocus();
    bool			uiCloseOK() { return uiObjHandle().closeOK(); }
    bool			isDisplayed() const { return display_; }

    Color              		uibackgroundColor() const;
    void              		uisetBackgroundColor(const Color&);
    void			uisetSensitive(bool yn=true);
    bool			uisensitive() const;

    int				prefHNrPics() const;
    void			setPrefWidth( int w )      
				{ 
				    if( itemInited() )
				    { 
					if( pref_width_set != w )
					 pErrMsg("Not allowed when finalized.");
					return;
				    }
				    pref_char_width = -1;
				    pref_width_set = w; 
				    pref_width_  = 0;
				    pref_height_ = 0;
				}
    float			prefWidthInCharSet() const 
				    { return pref_char_width; }
    void			setPrefWidthInChar( float w )
				{ 
				    if( itemInited() )
				    { 
					if( pref_char_width != w )
					 pErrMsg("Not allowed when finalized.");
					return;
				    }
				    pref_width_set = -1;
				    pref_char_width = w; 
				    pref_width_  = 0;
				    pref_height_ = 0;
				}

    int				prefVNrPics() const;
    void			setPrefHeight( int h )     
				{ 
				    if( itemInited() )
				    { 
					if( pref_height_set != h )
					 pErrMsg("Not allowed when finalized.");
					return;
				    }
				    pref_char_height = -1;
				    pref_height_set = h; 
				    pref_width_  = 0;
				    pref_height_ = 0;
				}
    float			prefHeightInCharSet() const 
				    { return pref_char_height; }
    void			setPrefHeightInChar( float h )
				{ 
				    if( itemInited() )
				    { 
					if( pref_char_height != h )
					 pErrMsg("Not allowed when finalized.");
					return;
				    }
				    pref_height_set = -1;
				    pref_char_height = h; 
				    pref_width_  = 0;
				    pref_height_ = 0;
				}

    void               		setStretch( int hor, int ver ) 
				{ 
				    if( itemInited() )
				    { 
					if( hStretch != hor || vStretch != ver )
					 pErrMsg("Not allowed when finalized.");
					return;
				    }
				    hStretch = hor; vStretch = ver; 
				}

    virtual int			stretch( bool hor, bool retUndef=false ) const
				{ 
				    int s = hor ? hStretch : vStretch;
				    if( retUndef ) return s;
				    return s != mUndefIntVal ? s : 0;
				}

    virtual int			nrTxtLines() const	{ return -1; }

    void			attach ( constraintType, uiObject *other=0, 
				 int margin=-1 );

    void 			uisetFont( const uiFont& );
    const uiFont*		uifont() const;

    virtual uiSize		actualSize( bool include_border = true) const;
    virtual uiSize		minimumSize() const
				{return uiSize(mUndefIntVal,mUndefIntVal,true);}

    void			uisetCaption( const char* );

    virtual void		reDraw( bool deep );

    virtual uiObject&		uiObjHandle()			=0;

    const i_LayoutItem*		layoutItem()		{ return layoutItem_; }
    i_LayoutItem*		mkLayoutItem( i_LayoutMngr& mngr )
				    { 
					if( layoutItem_ ) 
					{ 
					    pErrMsg("Already have layout itm"); 
					    return layoutItem_ ;
					}
					layoutItem_ = mkLayoutItem_( mngr );
					return layoutItem_;
				    }

	virtual void		finalise() 
				    { 
					if( finalised )	return;

					uiObjHandle().finalising.trigger(
								uiObjHandle()); 
					finalise_();
					finalised = true;
					if( !display_ ) display( display_ );
				    }

	virtual void		fontchanged();

	int				fontHgt() const 
					{ gtFntWdtHgt(); return fnt_hgt; }
	int				fontWdt(bool max=false) const
					{ 
					    gtFntWdtHgt(); 
					    return max ? fnt_maxwdt : fnt_wdt; 
					}

	void			setSzPol( const SzPolicySpec& p ) 
				    { 
					szpol = p; 
					if( p.hSzPol() >= SzPolicySpec::smallmax
					    && 
					    p.hSzPol() <= SzPolicySpec::widemax)
					{
					    int hs = mMAX( 2, stretch(true) );
					    int vs = stretch( false );

					    setStretch( hs, vs);
					}
					if( p.vSzPol() >= SzPolicySpec::smallmax
					    &&
					    p.vSzPol() <= SzPolicySpec::widemax)
					{
					    int hs = stretch( true );
					    int vs = mMAX( 2, stretch(false) );

					    setStretch( hs, vs);
					}

				    }
	SzPolicySpec		szPol() const		{ return szpol; }

	void			setShrinkAllowed( bool yn ) { allowshrnk = yn; }
	bool			shrinkAllowed()		{ return allowshrnk; }


	bool			isHidden()		{ return is_hidden; }
	bool			itemInited() const;

    protected:

	int			hStretch;
	int			vStretch;

	virtual const QWidget*	managewidg_() const	{ return qwidget_(); }

	virtual i_LayoutItem*	mkLayoutItem_( i_LayoutMngr& mngr );

	virtual void		finalise_()             {}

	void 			doDisplay(CallBacker*);

	int			fontWdtFor( const char* ) const;


	void			loitemDeleted()		{ layoutItem_ = 0; }


    private:

	i_LayoutItem*		layoutItem_;
	uiParentBody* 		parent_;
	const uiFont*		font_;

	bool			allowshrnk;

	bool			is_hidden;
	bool			finalised;
	bool			display_;
	bool			display_maximised;

	int			pref_width_;
	int			pref_height_;

	int			pref_width_set;
	float			pref_char_width;
	int			pref_height_set;
	float			pref_char_height;
	int			pref_width_hint;
	int			pref_height_hint;

	int			fnt_hgt;
	int			fnt_wdt;
	int			fnt_maxwdt;
	QFontMetrics*		fm;

	SzPolicySpec		szpol;

	void                	gtFntWdtHgt() const;
	void			getSzHint();

    #ifdef USE_DISPLAY_TIMER
	Timer&			displTim;
    #endif
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
