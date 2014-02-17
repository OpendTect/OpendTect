// No multiple inclusion protection
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/06/2001
 RCS:           $Id$
________________________________________________________________________

-*/

#ifndef mHANDLE_OBJ
# define mHANDLE_OBJ	C
#endif

#ifndef mQWIDGET_BODY
# define mQWIDGET_BODY	T
#endif

#ifndef mQWIDGET_BASE
# define mQWIDGET_BASE	mQWIDGET_BODY
#endif


public:

    virtual const QWidget* qwidget_() const { return this; }

#ifndef UIBASEBODY_ONLY
			//! override Qt
    virtual void	setFont( const QFont& )
			{
			    if ( !uifont() ) { pErrMsg("no uifont!"); return; }
			    mQWIDGET_BASE::setFont( uifont()->qFont() );
			}

    virtual void	fontChange( const QFont& oldFont )
			{
			    uiBody::fontchanged();
			}

			//! override Qt
    virtual void	closeEvent( QCloseEvent *e )
			{
			    if ( uiCloseOK() )
				mQWIDGET_BASE::closeEvent(e);
			}

protected:

    virtual uiObject&	uiObjHandle()              { return handle_; }

#endif

#ifdef UIPARENT_BODY_CENTR_WIDGET

public:

    uiGroup*		uiCentralWidg()		{ return centralwidget_; }


    virtual void        addChild( uiBaseObject& child )
			{
			    if ( !initing_ && centralwidget_ )
				centralwidget_->addChild( child );
			    else
				uiParentBody::addChild( child );
			}

    virtual void        manageChld_( uiBaseObject& o, uiObjectBody& b )
			{
			    if ( !initing_ && centralwidget_ )
				centralwidget_->manageChld( o, b );

			}

    virtual void	attachChild ( constraintType tp,
                                              uiObject* child,
                                              uiObject* other, int margin,
					      bool reciprocal )
                        {
                            if ( !child || initing_ ) return;

			    centralwidget_->attachChild( tp, child, other,
							margin, reciprocal);
                        }
protected:

    bool		initing_;

    uiGroup*		centralwidget_;

protected:

    virtual const QWidget* managewidg_() const
			{
			    if ( !initing_ )
				return centralwidget_->pbody()->managewidg();
			    return qwidget_();
			}
#endif

protected:

    mHANDLE_OBJ&	handle()		{ return handle_; }

protected:

    mHANDLE_OBJ&	handle_;


#undef mHANDLE_OBJ
#undef mQWIDGET_BASE
#undef mQWIDGET_BODY
#undef mTHIS_QWIDGET
#ifdef UIBASEBODY_ONLY
# undef UIBASEBODY_ONLY
#endif
#ifdef UIPARENT_BODY_CENTR_WIDGET
# undef UIPARENT_BODY_CENTR_WIDGET
#endif
