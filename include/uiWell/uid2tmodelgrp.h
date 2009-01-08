#ifndef uid2tmodelgrp_h
#define uid2tmodelgrp_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		August 2006
 RCS:		$Id: uid2tmodelgrp.h,v 1.3 2009-01-08 09:16:21 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class uiFileInput;
class uiGenInput;


mClass uiD2TModelGroup : public uiGroup
{
public:
    			uiD2TModelGroup(uiParent*,bool withunitfld,
					const char* filefldlbl=0);

    const char*		fileName() const;
    bool		isTVD() const;
    bool		isTWT() const;
    bool		zInFeet() const;

protected:

    uiFileInput*	filefld_;
    uiGenInput*		tvdfld_;
    uiGenInput*		unitfld_;
    uiGenInput*		twtfld_;
};


#endif
