#ifndef uicmain_h
#define uicmain_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          06/02/2002
 RCS:           $Id: uicmain.h,v 1.9 2012-08-03 13:00:54 cvskris Exp $
________________________________________________________________________

-*/

#include "uicoinmod.h"
#include "uimain.h"


/*!\brief COIN - Qt main window */

mClass(uiCoin) uicMain : public uiMain
{
public:
			uicMain(int& argc,char** argv);

    virtual int		exec();	

protected:

    virtual void	init(QWidget*);

};


/*!\mainpage Where COIN meets Qt

 Qt and COIN are both wonderful packages. Unfortunately, they both want to
 grab the main loop of an application. Fortunately, the COIN people from SIM
 (http://www.sim.no) have made the bridge: the SoQt package.

 As both COIN and Qt are covered with our interfaces, we also had to cover
 the SoQt part with the uiCoin module.

 The core class in this module is the uiSoViewer class, derived from COIN's
 SoQtExaminerViewer. This is the window where the scene will be rendered.
 Other classes handle viewer related issues, like keybindings and stereoviewing.

*/


#endif

