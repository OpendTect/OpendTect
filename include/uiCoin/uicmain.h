#ifndef vizmain_h
#define vizmain_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          06/02/2002
 RCS:           $Id: uicmain.h,v 1.3 2004-02-17 16:22:34 bert Exp $
________________________________________________________________________

-*/

#include <uimain.h>


/*!\brief COIN - Qt main window */

class uicMain : public uiMain
{
public:
			uicMain(int argc,char** argv);

    virtual int		exec();	

protected:

    virtual void	init( QWidget* );

};


/*!\mainpage Where COIN meets Qt

 Qt and COIN are both wonderful packages. Unfortunately, they both want to
 grab the main loop of an application. Fortunately, the COIN people from SIM
 (http://www.sim.no) have made the bridge: the SoQt package.

 As both COIN and Qt are covered with our interfaces, we also had to cover
 the SoQt part with the uiCoin module.

*/


#endif
