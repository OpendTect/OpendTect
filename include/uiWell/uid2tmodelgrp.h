#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uigroup.h"

class uiFileInput;
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
	virtual		~Setup()	{}

	mDefSetupMemb(bool,fileoptional)
	mDefSetupMemb(bool,withunitfld)
	mDefSetupMemb(bool,asksetcsmdl)
	mDefSetupMemb(BufferString,filefldlbl)

    };

			uiD2TModelGroup(uiParent*,const Setup&);
			~uiD2TModelGroup();

    bool		getD2T(Well::Data&,bool cksh = true) const;
    uiString		errMsg() const		{ return errmsg_; }
    uiString		warnMsg() const		{ return warnmsg_; }

    bool		wantAsCSModel() const;
    BufferString	dataSourceName() const;

    static const char*	sKeyTemporaryVel();
			/* Will be removed after 6.0. */

    static uiString	sKeyTemporaryVelUiStr();
    static float	getDefaultTemporaryVelocity();

protected:

    Setup		setup_;
    Table::FormatDesc&  fd_;
    mutable uiString	errmsg_;
    mutable uiString	warnmsg_;

    uiFileInput*	filefld_;
    uiGenInput*		velfld_;
    uiGenInput*		csfld_;
    uiTableImpDataSel*  dataselfld_;

    void		fileFldChecked(CallBacker*);
};


mGlobal(uiWell)	float	getGUIDefaultVelocity();
			/* Will be removed after 6.0.
			   Replaced by Vel::getGUIDefaultVelocity() */
