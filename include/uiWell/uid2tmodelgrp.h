#ifndef uid2tmodelgrp_h
#define uid2tmodelgrp_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		August 2006
 RCS:		$Id: uid2tmodelgrp.h,v 1.5 2009-01-23 09:51:05 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class uiFileInput;
class uiGenInput;
class uiCheckBox;
class uiLabel;

mClass uiD2TModelGroup : public uiGroup
{
public:
    			uiD2TModelGroup(uiParent*,bool withunitfld,
					const char* filefldlbl=0);

    const char*		fileName() const;
    bool		isTVD() const;
    bool		isTWT() const;
    bool		zInFeet() const;
    void 		modelSel();

protected:

    uiFileInput*	filefld_;
    uiGenInput*		tvdfld_;
    uiLabel*	        uilbl_;
    uiGenInput*		unitfld_;
    uiGenInput*		twtfld_;
    uiGenInput*         d2tmodelfld_;
};


#endif
