#ifndef uibody_h
#define uibody_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2001
 RCS:           $Id: uibody.h,v 1.13 2009-03-23 05:08:44 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiparent.h"

class QWidget;

mClass uiBody
{
public:
				uiBody()				{}
    virtual			~uiBody()				{}

    virtual void		finalise()				{}
    virtual bool		finalised() const	{ return false; }
    virtual void		clear()					{}
    virtual void		fontchanged()				{}


				//! can return 0
    inline const QWidget*       qwidget() const		{ return qwidget_();}
				//! can return 0
    inline QWidget*             qwidget()
                                   {return const_cast<QWidget*>(qwidget_());}

protected:

    virtual const QWidget*	qwidget_() const		=0;

};


/*! \brief Simple delegating implementation of uiBody.

Useful when a Qt object is already made, such as a QStatusBar, QMenuBar, etc.

*/
template <class C, class T>
class uiBodyImpl : public uiBody
{
public:
                        uiBodyImpl( C& handle, uiParent* parnt, T& qthing ) 
			    : uiBody()
			    , qthing_(&qthing)
			    , handle_(handle)
			    {}



    T*			qthing()			{ return qthing_; }
    const T*		qthing() const			{ return qthing_; }

    inline const C&	handle()			{ return handle_; }

protected:

    virtual const QWidget* qwidget_() const		
			    { return dynamic_cast<QWidget*>( qthing_ ); }

    T*			qthing_;

private:

    C&			handle_;

};

#endif
