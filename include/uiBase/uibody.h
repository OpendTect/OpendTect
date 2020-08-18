#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          16/05/2001
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiparent.h"

mFDQtclass(QWidget)

mExpClass(uiBase) uiBody
{
public:
    virtual			~uiBody()				{}

    virtual void		finalise()				{}
    virtual bool		finalised() const	{ return false; }
    virtual void		clear()					{}
    virtual void		fontchanged()				{}


				//! can return 0
    inline const mQtclass(QWidget*)  qwidget() const
				     { return qwidget_();}
				//! can return 0
    inline mQtclass(QWidget*) qwidget()
                            {return const_cast<mQtclass(QWidget*)>(qwidget_());}

protected:
				uiBody()				{}

    virtual const mQtclass(QWidget*)	qwidget_() const		=0;

};


/*! \brief Simple delegating implementation of uiBody.

Useful when a Qt object is already made, such as a QStatusBar, QMenuBar, etc.

*/
template <class C, class T>
mClass(uiBase) uiBodyImpl : public uiBody
{
public:
    virtual		~uiBodyImpl()			{ delete qthing_; }

    T*			qthing()			{ return qthing_; }
    const T*		qthing() const			{ return qthing_; }

    inline const C&	handle()			{ return handle_; }

protected:
                        uiBodyImpl( C& hndle, uiParent* parnt, T& qthng )
			    : uiBody()
			    , qthing_(&qthng)
			    , handle_(hndle)
			    {}

    virtual const mQtclass(QWidget*) qwidget_() const
			   {return dynamic_cast<mQtclass(QWidget*)>( qthing_ );}

    T*			qthing_;

private:

    C&			handle_;

};
