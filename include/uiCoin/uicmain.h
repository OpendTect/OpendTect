#ifndef vizmain_h
#define vizmain_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          06/02/2002
 RCS:           $Id: uicmain.h,v 1.2 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include <uimain.h>


class uicMain : public uiMain
{
public:
			uicMain(int argc,char** argv);

    virtual int		exec();	

protected:

    virtual void	init( QWidget* );

};


#endif
