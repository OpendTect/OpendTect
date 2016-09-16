#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		October 2011
 RCS:		$Id: uibodyregiondlg.h 35719 2014-07-22 06:30:11Z bert.bril@dgbes.com $
________________________________________________________________________


-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "dbkey.h"

class IOObj;
class uiIOObjSel;
class uiGenInput;
class uiPosSubSel;
class uiPushButton;
class uiTable;

namespace EM { class Region3D; }


mExpClass(uiEarthModel) uiBodyRegionGrp : public uiGroup
{ mODTextTranslationClass(uiBodyRegionGrp)
public:
	mExpClass(uiEarthModel) Setup
	{
	public:
	    Setup(bool _is2d=false)
		: is2d_(_is2d)
		, withinlcrlz_(true)
		, withfault_(true)
		, withareasel_(false)
		, withsinglehor_(false)
	    {
	    }

	    mDefSetupMemb(bool,is2d)
	    mDefSetupMemb(bool,withinlcrlz)
	    mDefSetupMemb(bool,withfault)
	    mDefSetupMemb(bool,withareasel)
	    mDefSetupMemb(bool,withsinglehor)

	};

				uiBodyRegionGrp(uiParent*,const Setup&);
				~uiBodyRegionGrp();

    void			setRegion(const EM::Region3D&);
    const EM::Region3D&		region() const;
    bool			accept();

protected:
    void			addInlCrlZCB(CallBacker*);
    void			addSurfaceCB(CallBacker*);
    void			removeSurfaceCB(CallBacker*);
    void			horModChg(CallBacker*);
    void			updateTable();

    uiPosSubSel*		subvolfld_;

    uiTable*			table_;
    uiPushButton*		addhorbutton_;
    uiPushButton*		addfltbutton_;
    uiPushButton*		addinlbutton_;
    uiPushButton*		addcrlbutton_;
    uiPushButton*		addzbutton_;
    uiPushButton*		removebutton_;

    uiGenInput*			singlehorfld_;
    bool			singlehoradded_;

    EM::Region3D&		region3d_;
};


mExpClass(uiEarthModel) uiBodyRegionDlg : public uiDialog
{ mODTextTranslationClass(uiBodyRegionDlg)
public:
				uiBodyRegionDlg(uiParent*,bool is2d);
				~uiBodyRegionDlg();

    DBKey			getBodyMid() const;

protected:
    bool			acceptOK();
    bool			createImplicitBody();

    uiBodyRegionGrp*		grp_;
    uiIOObjSel*			outputfld_;
};
