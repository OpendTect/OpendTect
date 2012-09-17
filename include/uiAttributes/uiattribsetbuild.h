#ifndef uiattribsetbuild_h
#define uiattribsetbuild_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
 RCS:           $Id: uiattribsetbuild.h,v 1.12 2011/06/27 08:41:16 cvsbruno Exp $
________________________________________________________________________

-*/


#include "uibuildlistfromlist.h"
#include "datapack.h"

class CtxtIOObj;
namespace Attrib { class DescSet; }
class uiPreStackAttrib;


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

    void		setDataPackInp(const TypeSet<DataPack::FullID>&,
	    				bool isprestack=false);

protected:

    Attrib::DescSet&	descset_;
    const Setup		attrsetup_;
    CtxtIOObj&		ctio_;
    TypeSet<DataPack::FullID> dpfids_;
    TypeSet<DataPack::FullID> psdpfids_;

    uiToolButton*	savebut_;
    uiPreStackAttrib*	uipsattrdesced_;

    void		fillAvailable();
    bool		doAttrSetIO(bool);

    virtual void	defSelChg();
    virtual void	editReq(bool);
    virtual void	removeReq();
    virtual bool	ioReq(bool);
    virtual const char*	avFromDef(const char*) const;

};


#endif
