#ifndef qtclss_h
#define qtclss_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Jan 2009
 * ID       : $Id$
-*/

#include "uiqtappmod.h"
#include "commondefs.h"

class QMainWindow;
class QPushButton;
class QWidget;

mClass(uiQtApp) QtClss
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
