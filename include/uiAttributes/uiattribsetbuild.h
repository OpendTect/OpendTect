#ifndef uiattribsetbuild_h
#define uiattribsetbuild_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id: uiattribsetbuild.h,v 1.3 2011-01-10 13:30:13 cvsbert Exp $
________________________________________________________________________

-*/


#include "uigroup.h"
#include "datapack.h"
#include "bufstringset.h"

namespace Attrib { class Desc; class DescSet; }

class uiListBox;
class uiToolButton;


mClass uiAttribDescSetBuild : public uiGroup
{
public:

    mClass Setup
    {
    public:
				Setup(bool for2d);
	mDefSetupMemb(bool,is2d);
	mDefSetupMemb(bool,showps);
	mDefSetupMemb(bool,singletraceonly);
	mDefSetupMemb(bool,showusingtrcpos);
	mDefSetupMemb(bool,showdepthonlyattrs);
	mDefSetupMemb(bool,showtimeonlyattrs);
	mDefSetupMemb(bool,showhidden);
	mDefSetupMemb(bool,showsteering);
    };
				uiAttribDescSetBuild(uiParent*,const Setup&);
				~uiAttribDescSetBuild();

    const Attrib::DescSet&	descSet() const		{ return descset_; }
    bool			haveUserChange() const	{ return usrchg_; }

    void			setDataPackInp(const DataPack::FullID&);

protected:

    Attrib::DescSet&		descset_;
    BufferStringSet		availattrnms_;
    const Setup			setup_;
    bool			usrchg_;

    uiListBox*			availattrfld_;
    uiListBox*			defattrfld_;
    uiToolButton*		edbut_;
    uiToolButton*		rmbut_;
    uiToolButton*		savebut_;

    void			fillAvailAttrFld();
    void			fillDefAttribFld();
    bool			doAttrEd(Attrib::Desc& desc,bool);

    void			updButStates(CallBacker* cb=0);
    void			addReq(CallBacker*);
    void			edReq(CallBacker*);
    void			rmReq(CallBacker*);
    void			openReq(CallBacker*);
    void			saveReq(CallBacker*);

};


#endif
