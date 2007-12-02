/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne and Kristofer
 Date:          December 2007
 RCS:           $Id: initalgo.cc,v 1.1 2007-12-02 09:22:51 cvsnanne Exp $
________________________________________________________________________

-*/

#include "initalgo.h"
#include "windowfunction.h"

void Algo::initStdClasses()
{
    BoxWindow::initClass();
    HammingWindow::initClass();
    HanningWindow::initClass();
    BlackmanWindow::initClass();
    BartlettWindow::initClass();
    CosTaperWindow::initClass();
}
