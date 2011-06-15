#ifndef uiattribsetbuild_h
#define uiattribsetbuild_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id: uiattribsetbuild.h,v 1.10 2011-06-15 10:12:46 cvsbert Exp $
________________________________________________________________________

-*/


#include "uibuildlistfromlist.h"
#include "datapack.h"

class CtxtIOObj;
namespace Attrib { class DescSet; }


mClass uiAttribDescSetBuild : public uiBuildListFromList
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

    const Attrib::DescSet& descSet() const	{ return descset_; }

    void		setDataPackInp(const TypeSet<DataPack::FullID>&);

protected:

    Attrib::DescSet&	descset_;
    BufferStringSet	availattrnms_;
    const Setup		attrsetup_;
    CtxtIOObj&		ctio_;
    TypeSet<DataPack::FullID> dpfids_;

    uiToolButton*	savebut_;

    void		fillAvailable();
    bool		doAttrSetIO(bool);

    virtual void	defSelChg();
    virtual void	editReq(bool);
    virtual void	removeReq();
    virtual bool	ioReq(bool);
    virtual const char*	avFromDef(const char*) const;

};


#endif
