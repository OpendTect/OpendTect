#ifndef uiparent_h
#define uiparent_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2001
 RCS:           $Id: uiparent.h,v 1.1 2001-05-16 14:58:47 arend Exp $
________________________________________________________________________

-*/

#include <errh.h>

class QWidget;
class i_LayoutMngr;

class uiParent
{
friend class		uiObject;
friend class            uiGroup;

public:
			uiParent()			{}

    inline QWidget&     qWidget()
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

    inline uiParent&    clientWidget()
                        { return const_cast<uiParent&>(clientWidget_()); }
    inline const uiParent& clientWidget() const{ return clientWidget_(); }

    inline QWidget&     clientQWidget()
                        {  return clientWidget().qWidget(); }
    inline const QWidget& clientQWidget() const
                        { return clientWidget().qWidget(); }

protected:

    virtual const QWidget*	qWidget_() const	=0;
    virtual const uiParent&	clientWidget_()const	{ return *this; }

    virtual i_LayoutMngr*	mLayoutMngr()		{ return 0; }
    virtual i_LayoutMngr*	prntLayoutMngr()	{ return 0; }

};

#endif
