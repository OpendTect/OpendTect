#ifndef uihor3dfrom2ddlg_h
#define uihor3dfrom2ddlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          January 2007
 RCS:           $Id: uihor3dfrom2ddlg.h,v 1.2 2007-01-24 17:16:02 cvsjaap Exp $
________________________________________________________________________

-*/


#include "uidialog.h"

namespace EM { class Horizon2D; };


/*! \brief Dialog to expand a 2D horizon to create a 3D horizon */

class uiHor3DFrom2DDlg : public uiDialog
{
public:    
				uiHor3DFrom2DDlg(uiParent*,
						 const EM::Horizon2D&);

protected:
    bool			acceptOK(CallBacker*);

    const EM::Horizon2D& 	hor2d_;

};

#endif
