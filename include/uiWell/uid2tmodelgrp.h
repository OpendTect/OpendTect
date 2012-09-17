#ifndef uid2tmodelgrp_h
#define uid2tmodelgrp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2006
 RCS:		$Id: uid2tmodelgrp.h,v 1.9 2009/07/22 16:01:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class uiFileInput;
class uiGenInput;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }
namespace Well { class Data; }

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

    const char*		getD2T(Well::Data&,bool cksh = true) const;

    bool		wantAsCSModel() const;

protected:

    Setup		setup_;
    Table::FormatDesc&  fd_;


    uiFileInput*	filefld_;
    uiGenInput*		velfld_;
    uiGenInput*		csfld_;
    uiTableImpDataSel*  dataselfld_;

    void 		fileFldChecked(CallBacker*);
};


#endif
