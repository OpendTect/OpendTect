// No multiple inclusion protection
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/06/2001
 RCS:           $Id: i_uiobjqtbody.h,v 1.2 2001-12-19 11:37:01 arend Exp $
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

#ifndef mTHIS_QWIDGET
# define mTHIS_QWIDGET	this
#endif

public:

    virtual const QWidget*
			qwidget_() const { return mTHIS_QWIDGET; }

#ifndef UIBASEBODY_ONLY
			//! over-ride Qt
    virtual void 	setFont( const QFont &font )
			{
			    if ( !uifont() ) { pErrMsg("no uifont!"); return; }
			    mQWIDGET_BASE::setFont( uifont()->qFont() );
			}

    virtual void	fontChange( const QFont& oldFont )
			{
			    uiBody::fontchanged();
			    mQWIDGET_BASE::fontChange( oldFont );
			}

			//! over-ride Qt
    virtual void	closeEvent( QCloseEvent *e )
			{
			    if ( uiCloseOK() ) 
				mQWIDGET_BASE::closeEvent(e);
			} 
protected:

    virtual uiObject&	uiObjHandle()              { return handle_; }

#endif

    mHANDLE_OBJ&     	handle()		{ return handle_; }

protected:

    mHANDLE_OBJ&       	handle_;


#undef mHANDLE_OBJ 
#undef mQWIDGET_BASE
#undef mQWIDGET_BODY
#undef mTHIS_QWIDGET
#ifdef UIBASEBODY_ONLY
# undef UIBASEBODY_ONLY
#endif
