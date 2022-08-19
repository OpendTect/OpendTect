#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
