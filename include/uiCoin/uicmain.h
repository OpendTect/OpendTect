#ifndef uicmain_h
#define uicmain_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          06/02/2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uicoinmod.h"
#include "uimain.h"


/*!
\brief COIN - Qt main window
*/

mExpClass(uiCoin) uicMain : public uiMain
{
public:
			uicMain(int& argc,char** argv);

    virtual int		exec();	

protected:

    virtual void	init(QWidget*);

};

#endif

