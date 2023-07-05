#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uigroup.h"

#include "datapointset.h"

class BufferStringSet;
class DataPointSetDisplayMgr;
class IOObj;

class uiDataPointSet;
class uiGenInput;
class uiListBox;
class uiMultiWellLogSel;
class uiPosFilterSetSel;

namespace Attrib { class DescSet; }


mExpClass(uiWellAttrib) uiWellLogExtractGrp : public uiGroup
{ mODTextTranslationClass(uiWellLogExtractGrp);
public:

    mExpClass(uiWellAttrib) Setup
    {
    public:
				Setup(bool wa =true,bool singlog =false,
				      const char* prop =0);
				~Setup();

	mDefSetupMemb(bool,singlelog);
	mDefSetupMemb(bool,withattrib);
	mDefSetupMemb(BufferString,prefpropnm);
    };
				uiWellLogExtractGrp(uiParent*,
					const uiWellLogExtractGrp::Setup&,
					const Attrib::DescSet* =nullptr);
				~uiWellLogExtractGrp();

    void			setDescSet(const Attrib::DescSet*);
    const Attrib::DescSet*	getDescSet() const	{ return ads_; }

    void			getWellNames(BufferStringSet&);
    void			getSelLogNames(BufferStringSet&);

    bool			extractDPS();
    ConstRefMan<DataPointSet>	getDPS() const;
    void			releaseDPS();
    const Setup&		su() const		{ return setup_; }

protected:

    const Attrib::DescSet* ads_;
    ObjectSet<IOObj>	wellobjs_;

    Setup		setup_;
    uiListBox*		attrsfld_ = nullptr;
    uiGenInput*		radiusfld_ = nullptr;
    uiGenInput*		logresamplfld_;
    uiMultiWellLogSel*	welllogselfld_;
    uiPosFilterSetSel*	posfiltfld_ = nullptr;
    RefMan<DataPointSet>	curdps_;

    void		adsChg();
    bool		extractWellData(const TypeSet<MultiID>&,
					const BufferStringSet&,
					ObjectSet<DataPointSet>&);
    bool		extractAttribData(DataPointSet&,int);
};
