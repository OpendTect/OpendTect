#ifndef uiwelllogextract_h
#define uiwelllogextract_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          July 2012
 RCS:           $Id$
________________________________________________________________________

-*/


#include "uiwellattribmod.h"
#include "uigroup.h"
class IOObj;
class uiListBox;
class uiGenInput;
class DataPointSet;
class DataPointSetDisplayMgr;
class BufferStringSet;
class uiDataPointSet;
class uiMultiWellLogSel;
class uiPosFilterSetSel;
namespace Attrib { class DescSet; }


mClass(uiWellAttrib) uiWellLogExtractGrp : public uiGroup
{
public:

    struct Setup
    {
			Setup(bool wa=true,bool singlog =false,
			      const char* prop =0)
			    : singlelog_(singlog)
			    , withattrib_(wa)
			    , prefpropnm_(prop)	{}
	mDefSetupMemb(bool,singlelog);
	mDefSetupMemb(bool,withattrib);
	mDefSetupMemb(BufferString,prefpropnm);
    };
				uiWellLogExtractGrp(uiParent*,
					const uiWellLogExtractGrp::Setup&,
					const Attrib::DescSet* ads=0);
						
				~uiWellLogExtractGrp();

    void			setDescSet(const Attrib::DescSet*);
    void			getWellNames(BufferStringSet&);
    void			getSelLogNames(BufferStringSet&);

    bool			extractDPS();
    const DataPointSet*		getDPS() const;
    void			releaseDPS();
    const Setup&		su() const		{ return setup_; }

protected:

    const Attrib::DescSet* ads_;
    ObjectSet<IOObj>	wellobjs_;

    Setup		setup_;
    uiListBox*		attrsfld_;
    uiGenInput*		radiusfld_;
    uiGenInput*		logresamplfld_;
    uiMultiWellLogSel*	welllogselfld_;
    uiPosFilterSetSel*	posfiltfld_;
    DataPointSet*	curdps_;

    void		adsChg();
    bool		extractWellData(const BufferStringSet&,
	    				const BufferStringSet&,
					ObjectSet<DataPointSet>&);
    bool		extractAttribData(DataPointSet&,int);
};


#endif

