#ifndef vizmain_h
#define vizmain_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          06/02/2002
 RCS:           $Id: uicmain.h,v 1.1 2002-02-07 14:58:40 arend Exp $
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
