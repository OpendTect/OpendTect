#ifndef uiattribsetbuild_h
#define uiattribsetbuild_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id: uiattribsetbuild.h,v 1.1 2011-01-06 15:24:38 cvsbert Exp $
________________________________________________________________________

-*/


#include "uigroup.h"
#include "bufstringset.h"

namespace Attrib { class DescSet; }

class uiListBox;


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

protected:

    Attrib::DescSet&		descset_;
    BufferStringSet		availattrnms_;

    uiListBox*			availattrfld_;
    uiListBox*			defattrfld_;

    void			mkAvailAttrFld(const Setup&);

    void			addReq(CallBacker*);
    void			edReq(CallBacker*);
    void			rmReq(CallBacker*);

};


#endif
