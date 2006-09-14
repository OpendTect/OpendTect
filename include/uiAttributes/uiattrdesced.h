#ifndef uiattrdesced_h
#define uiattrdesced_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: uiattrdesced.h,v 1.14 2006-09-14 20:12:29 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "changetracker.h"
#include "paramsetget.h"
#include "bufstring.h"

namespace Attrib { class Desc; class DescSet; class DescSetMan; };

class uiAttrDescEd;
class uiAttrSel;
class uiAttrSelData;
class uiImagAttrSel;
class uiSteeringSel;
class uiSteerCubeSel;

using namespace Attrib;

/*! \brief Description of attribute parameters to evaluate */

class EvalParam
{
public:
    			EvalParam( const char* lbl, const char* par1=0,
				   const char* par2=0 )
			    : label_(lbl), par1_(par1), par2_(par2)
			    , evaloutput_(false)	{}

    bool		operator==(const EvalParam& ep) const
			{
			    return label_==ep.label_ && par1_==ep.par1_ &&
				   par2_==ep.par2_;
			}

    BufferString	label_;
    BufferString	par1_;
    BufferString	par2_;
    bool		evaloutput_;

};


/*! \brief Attribute description editor creater */

class uiAttrDescEdCreater
{
public:
    virtual			~uiAttrDescEdCreater()		{}
    virtual uiAttrDescEd*	create(uiParent*) const		= 0;

};



/*! \brief Attribute description editor */

class uiAttrDescEd : public uiGroup
{
public:

    virtual		~uiAttrDescEd();

    void		setDesc(Desc*,DescSetMan*);
    void		setDescSet( DescSet* ds )	{ ads_ = ds; }
    Desc*		curDesc()			{ return desc_; }
    const Desc*		curDesc() const			{ return desc_; }
    virtual const char*	commit(Desc* desc=0);
			//!< returns null on success, error message otherwise
    			//!< If attribdesc is non-zero, that desc will be
    			//!< filled. If not, the internal desc will be filled.

    virtual void	set2D(bool yn)			{}

    virtual int		getOutputIdx(float val) const	{ return (int)val; }
    virtual float	getOutputValue(int idx) const	{ return (float)idx; }
    virtual void	setOutputStep(float step)	{}

    virtual void	getEvalParams(TypeSet<EvalParam>&) const {}

    virtual const char* getAttribName() const		= 0;
    virtual bool	isUIFor( const char* attrnm ) const
			{ return !strcmp(attrnm,getAttribName()); }
    void		setDisplayName(const char* nm ) { dispname_ = nm; }
    const char*		getDisplayName() const		{ return dispname_; }

    virtual bool	useIfZIsTime() const		{ return true; }
    virtual bool	useIfZIsDepth() const		{ return true; }

    static const char*	timegatestr;
    static const char*	frequencystr;
    static const char*	stepoutstr;
    static const char*	filterszstr;

protected:

			uiAttrDescEd(uiParent*);

    virtual bool	setParameters(const Desc&)	{ return true; }
    virtual bool	getParameters(Desc&)		{ return true; }
    virtual bool	setInput(const Desc&)		{ return true; }
    virtual bool	getInput(Desc&)			{ return true; }
    virtual bool	setOutput(const Desc&)		{ return true; }
    virtual bool	getOutput(Desc&);

    virtual bool        areUIParsOK()			{ return true; }

    void		fillOutput(Desc&,int selout);
    void		fillInp(uiAttrSel*,Desc&,int);
    void		fillInp(uiSteeringSel*,Desc&,int);
    void		fillInp(uiSteerCubeSel*,Desc&,int);
    
    void		putInp(uiAttrSel*,const Desc&,int inpnr);
    void		putInp(uiSteerCubeSel*,const Desc&,int inpnr);
    void		putInp(uiSteeringSel*,const Desc&,int inpnr);

    BufferString	gateLabel() const;
    BufferString	shiftLabel() const;
    bool		zIsTime() const;

    ChangeTracker	chtr_;
    uiAttrSel*		getInpFld(const char* txt=0,const uiAttrSelData* =0);
    uiImagAttrSel*	getImagInpFld();
    void		attrInpSel(CallBacker*);

    BufferString	errmsg_;
    DescSet*		ads_;

private:

    BufferString	dispname_;
    Desc*		desc_;
    DescSetMan*		adsman_;
};


#define mInitUI( clss, displaynm ) \
void clss::initClass() \
{ uiAF().add( displaynm, createInstance ); } \
\
uiAttrDescEd* clss::createInstance( uiParent* p ) \
{ return new clss( p ); }


#endif
