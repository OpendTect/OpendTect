#ifndef uid2tmodelgrp_h
#define uid2tmodelgrp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2006
 RCS:		$Id$
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
{
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
			~uiD2TModelGroup();

    const char*		getD2T(Well::Data&,bool cksh = true) const;
    bool		getD2TBool(Well::Data&,bool cksh = true) const;
    const char*		errMsg() const;
    const char*		warnMsg() const;

    bool		wantAsCSModel() const;
    BufferString	dataSourceName() const;

protected:

    Setup		setup_;
    Table::FormatDesc&  fd_;


    uiFileInput*	filefld_;
    uiGenInput*		velfld_;
    uiGenInput*		csfld_;
    uiTableImpDataSel*  dataselfld_;

    void		fileFldChecked(CallBacker*);

			//For ABI compatibility, will be removed
    void		setMsg(const BufferString,bool warning) const;
    void		setMsgNonconst(const BufferString,bool warning);
};


mGlobal(uiWell)	float	getGUIDefaultVelocity();
                        //!< If survey display unit is feet, it returns 8000
                        //!< otherwise 2000. Its purpose is to get nice values
                        //!< of velocity when initializing velocity fields


#endif

