#pragma once
/*+
________________________________________________________________________

Copyright:	(C) 1995-2022 dGB Beheer B.V.
License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uigroup.h"
#include "uidialog.h"

#include "factory.h"

class PickSetInfo;

class uiButtonGroup;
class uiCheckBox;
class uiGenInput;
class uiTable;
class uiTextEdit;
class uiToolButton;
class uiZRangeInput;
namespace Pick { class Set; }

mExpClass(uiIo) uiFilterPicksGrp : public uiGroup
{ mODTextTranslationClass(uiFilterPicksGrp)
public:

				uiFilterPicksGrp(uiParent*,Pick::Set&);
				~uiFilterPicksGrp();

    void			getInfo(BufferString&) const;

    bool			applyFilter();

private:

    void			useXYCB(CallBacker*);
    void			finalizeCB(CallBacker*);

    bool			applyLocFilter(bool keepinside);
    bool			applySphericalFilter(bool keepinside);

    uiGenInput*			usexycb_;
    uiGenInput*			keepdiscardfld_;

    uiGroup*			crdgrp_;
    uiGroup*			inlcrlgrp_;

    uiGenInput*			xintvfld_;
    uiGenInput*			yintvfld_;
    uiGenInput*			inlintvfld_;
    uiGenInput*			crlintvfld_;
    uiZRangeInput*		zintvfld_;

    uiGenInput*			radiusfld_		= nullptr;
    uiGenInput*			dipfld_			= nullptr;
    uiGenInput*			azimfld_		= nullptr;

    Pick::Set&			ps_;
    PickSetInfo&		psinfo_;

};


mExpClass(uiIo) uiEditPolygonGroup : public uiGroup
{ mODTextTranslationClass(uiEditPolygonGroup)
public:

				uiEditPolygonGroup(uiParent*,Pick::Set&);
				~uiEditPolygonGroup();

    uiButtonGroup*		tblButGrp();

private:

    void			createTable();
    void			fillTable();
    void			fillRow(int row);

    void			valChgCB(CallBacker*);
    void			removeLocCB(CallBacker*);
    void			pickLocationChangedCB(CallBacker*);

    uiTable*			tbl_			    = nullptr;
    uiButtonGroup*		tblbutgrp_		    = nullptr;

    Pick::Set&			ps_;

};



mExpClass(uiIo) uiEditPicksDlg : public uiDialog
{ mODTextTranslationClass(uiEditPicksDlg)
public:

			    uiEditPicksDlg(uiParent*,Pick::Set&);
			    ~uiEditPicksDlg();

    mDefineFactory2ParamInClass(uiEditPicksDlg,uiParent*,Pick::Set&,factory);

    static uiEditPicksDlg*  create( uiParent* p, Pick::Set& ps )
			    { return new uiEditPicksDlg(p,ps); }
    static void		    initClass();

protected:

    bool		    acceptOK(CallBacker*) override;
    void		    applyCB(CallBacker*);

    uiTextEdit*		    infofld_		    = nullptr;
    uiEditPolygonGroup*     polygongrp_		    = nullptr;
    uiFilterPicksGrp*	    filtergrp_;

    Pick::Set&		    ps_;

};
