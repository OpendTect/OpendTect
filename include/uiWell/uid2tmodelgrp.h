#ifndef uid2tmodelgrp_h
#define uid2tmodelgrp_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		August 2006
 RCS:		$Id: uid2tmodelgrp.h,v 1.7 2009-04-21 12:07:45 cvsbert Exp $
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
			    , asksetcsmdl_(false)
			    , filefldlbl_("Depth to Time model file")	{}

	mDefSetupMemb(bool,fileoptional)
	mDefSetupMemb(bool,withunitfld)
	mDefSetupMemb(bool,asksetcsmdl)
	mDefSetupMemb(BufferString,filefldlbl)

    };

    			uiD2TModelGroup(uiParent*,const Setup&);

    const char*		checkInput() const;
    const Well::AscImporter::D2TModelInfo&	getMI() const	{ return mi_; }

    bool		wantAsCSModel() const;

protected:

    Setup		setup_;
    Well::AscImporter::D2TModelInfo	mi_;

    uiFileInput*	filefld_;
    uiGenInput*		velfld_;
    uiGenInput*		tvdfld_;
    uiLabel*	        tvdsslbl_;
    uiGenInput*		unitfld_;
    uiGenInput*		twtfld_;
    uiGenInput*		csfld_;

    void 		fileFldChecked(CallBacker*);
};


#endif
