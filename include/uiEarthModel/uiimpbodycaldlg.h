#ifndef uiimpbodycaldlg_h
#define uiimpbodycaldlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          April 2012
 RCS:           $Id: uiimpbodycaldlg.h,v 1.1 2012-04-18 15:25:21 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;
namespace EM	{ class Body; class ImplicitBody; }


/*! \brief UI for volume calculation of implicit body */

mClass uiImplBodyCalDlg : public uiDialog
{
public:
			uiImplBodyCalDlg(uiParent*,const EM::Body&);
			~uiImplBodyCalDlg();

protected:
    
    void		calcCB(CallBacker*);
    void		getImpBody();

    const EM::Body&	embody_;
    EM::ImplicitBody*	impbody_;
    uiGenInput*		velfld_;
    uiGenInput*		volfld_;
};


#endif
