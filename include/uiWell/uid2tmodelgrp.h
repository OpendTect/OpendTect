#ifndef uid2tmodelgrp_h
#define uid2tmodelgrp_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		August 2006
 RCS:		$Id: uid2tmodelgrp.h,v 1.6 2009-04-20 13:29:58 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "wellimpasc.h"

class uiFileInput;
class uiGenInput;
class uiCheckBox;
class uiLabel;

mClass uiD2TModelGroup : public uiGroup
{
public:

    mClass Setup
    {
    public:
			Setup( bool fopt=true )
			    : fileoptional_(fopt)
			    , withunitfld_(true)
			    , filefldlbl_("Depth to Time model file")	{}

	mDefSetupMemb(bool,fileoptional)
	mDefSetupMemb(bool,withunitfld)
	mDefSetupMemb(BufferString,filefldlbl)

    };

    			uiD2TModelGroup(uiParent*,const Setup&);

    const char*		checkInput() const;
    const Well::AscImporter::D2TModelInfo&	getMI() const	{ return mi_; }

protected:

    Setup		setup_;
    Well::AscImporter::D2TModelInfo	mi_;

    uiFileInput*	filefld_;
    uiGenInput*		velfld_;
    uiGenInput*		tvdfld_;
    uiLabel*	        tvdsslbl_;
    uiGenInput*		unitfld_;
    uiGenInput*		twtfld_;

    void 		fileFldChecked(CallBacker*);
};


#endif
