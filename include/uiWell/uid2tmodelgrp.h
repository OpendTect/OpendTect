#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2006
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uigroup.h"

class uiConstantVel;
class uiFileSel;
class uiGenInput;
class uiTableImpDataSel;
namespace Table { class FormatDesc; }
namespace Well { class Data; }

mExpClass(uiWell) uiD2TModelGroup : public uiGroup
{ mODTextTranslationClass(uiD2TModelGroup);
public:

    mExpClass(uiWell) Setup
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

    bool		getD2T(Well::Data&,bool cksh = true) const;
    uiString		errMsg() const		{ return errmsg_; }
    uiString		warnMsg() const		{ return warnmsg_; }

    bool		wantAsCSModel() const;
    BufferString	dataSourceName() const;

    static uiString	sKeyTemporaryVel();
    static float	getDefaultTemporaryVelocity();

protected:

    Setup		setup_;
    Table::FormatDesc&  fd_;
    mutable uiString	errmsg_;
    mutable uiString	warnmsg_;

    uiFileSel*		filefld_;
    uiConstantVel*	velfld_;
    uiGenInput*		csfld_;
    uiTableImpDataSel*  dataselfld_;

    void		fileFldChecked(CallBacker*);
};
