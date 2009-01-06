#ifndef qtclss_h
#define qtclss_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Jan 2009
 * ID       : $Id: qtclss.h,v 1.1 2009-01-06 12:02:19 cvsbert Exp $
-*/

class QMainWindow;
class QPushButton;
class QWidget;

class QtClss
{
public:

    			QtClss(QWidget*);
			~QtClss();

    void		go();

protected:

    QMainWindow*	win_;
    QPushButton*	btn_;

};


#endif
