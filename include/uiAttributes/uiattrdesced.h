#ifndef uiattrdesced_h
#define uiattrdesced_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: uiattrdesced.h,v 1.37 2012/03/21 13:49:45 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "uiattribfactory.h"
#include "changetracker.h"
#include "paramsetget.h"
#include "datapack.h"

namespace Attrib { class Desc; class DescSet; class DescSetMan; };

class uiAttrDescEd;
class uiAttrSel;
class uiAttrSelData;
class uiImagAttrSel;
class uiSteeringSel;
class uiSteerCubeSel;

using namespace Attrib;

/*! \brief Description of attribute parameters to evaluate */

mClass EvalParam
{
public:
    			EvalParam( const char* lbl, const char* par1=0,
				   const char* par2=0, int idx=mUdf(int) )
			    : label_(lbl), par1_(par1), par2_(par2), pgidx_(idx)
			    , evaloutput_(false)	{}

    bool		operator==(const EvalParam& ep) const
			{
			    return label_==ep.label_ && par1_==ep.par1_ &&
				   par2_==ep.par2_ && pgidx_==ep.pgidx_;
			}

    BufferString	label_;
    BufferString	par1_;
    BufferString	par2_;
    int			pgidx_;
    bool		evaloutput_;

};


/*! \brief Attribute description editor creater */

mClass uiAttrDescEdCreater
{
public:
    virtual			~uiAttrDescEdCreater()		{}
    virtual uiAttrDescEd*	create(uiParent*) const		= 0;

};



/*! \brief Attribute description editor
 
 Required functions are declared in the macro mDeclReqAttribUIFns. Two of
 those, attribName() and createInstance() are implemented by the mInitAttribUI
 macro.

 */

mClass uiAttrDescEd : public uiGroup
{
public:

    virtual		~uiAttrDescEd();
    const char*		helpID()			{ return helpid_; }
    void		setDesc(Desc*,DescSetMan*);
    void		setDescSet( DescSet* ds )	{ ads_ = ds; }
    Desc*		curDesc()			{ return desc_; }
    const Desc*		curDesc() const			{ return desc_; }

    virtual const char*	commit(Desc* desc=0);
			//!< returns null on success, error message otherwise
    			//!< If attribdesc is non-zero, that desc will be
    			//!< filled. If not, the internal desc will be filled.

    virtual int		getOutputIdx(float val) const	{ return (int)val; }
    virtual float	getOutputValue(int idx) const	{ return (float)idx; }
    virtual void	setOutputStep(float step)	{}

    virtual void	getEvalParams(TypeSet<EvalParam>&) const {}

    virtual const char* attribName() const		= 0;
    const char*		displayName() const		{ return dispname_; }
    void		setDisplayName(const char* nm ) { dispname_ = nm; }

    enum DomainType	{ Both, Time, Depth };
    DomainType		domainType() const		{ return domtyp_; }
    void		setDomainType( DomainType t )	{ domtyp_ = t; }

    enum DimensionType	{ AnyDim, Only3D, Only2D };
    DimensionType	dimensionType() const		{ return dimtyp_; }
    void		setDimensionType( DimensionType t ) { dimtyp_ = t; }

    bool		is2D() const			{ return is2d_; }

    void		setNeedInputUpdate()		{ needinpupd_ = true; }

    virtual void	setDataPackInp(const TypeSet<DataPack::FullID>&);

    static const char*	getInputAttribName(uiAttrSel*,const Desc&);

    static const char*	timegatestr();
    static const char*	frequencystr();
    static const char*	stepoutstr();
    static const char*	filterszstr();

protected:

			uiAttrDescEd(uiParent*,bool is2d,const char* helpid=0);

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

    bool                needInputUpdate() const		{ return needinpupd_; }

    BufferString	zDepLabel(const char* pre,const char* post) const;
    BufferString	gateLabel() const
			{ return zDepLabel( 0, "gate" ); }
    BufferString	shiftLabel() const
			{ return zDepLabel( 0, "shift" ); }
    bool		zIsTime() const;

    ChangeTracker	chtr_;
    uiAttrSel*		createInpFld(bool is2d,const char* txt=0);
    uiAttrSel*		createInpFld(const uiAttrSelData&,const char* txt=0);
    uiImagAttrSel*	createImagInpFld(bool is2d);

    BufferString        helpid_;
    BufferString	attrnm_;
    DomainType		domtyp_;
    DimensionType	dimtyp_;
    BufferString	errmsg_;
    DescSet*		ads_;
    bool		is2d_;
    bool		needinpupd_;

    TypeSet<DataPack::FullID> dpfids_;

    static const char*	sKeyOtherGrp();
    static const char*	sKeyBasicGrp();
    static const char*	sKeyFilterGrp();
    static const char*	sKeyFreqGrp();
    static const char*	sKeyPatternGrp();
    static const char*	sKeyStatsGrp();
    static const char*	sKeyPositionGrp();
    static const char*	sKeyDipGrp();

  

private:

    BufferString	dispname_;
    Desc*		desc_;
    DescSetMan*		adsman_;

};


#define mDeclReqAttribUIFns \
protected: \
    static uiAttrDescEd* createInstance(uiParent*,bool); \
    static int factoryid_; \
public: \
    static void initClass(); \
    virtual const char* attribName() const; \
    static int factoryID() { return factoryid_; }


#define mInitAttribUIPars( clss, attr, displaynm, grp, domtyp, dimtyp ) \
\
int clss::factoryid_ = -1; \
\
void clss::initClass() \
{ \
    if ( factoryid_ < 0 ) \
	factoryid_ = uiAF().add( displaynm, attr::attribName(), grp, \
		     clss::createInstance, (int)domtyp, (int)dimtyp ); \
} \
\
uiAttrDescEd* clss::createInstance( uiParent* p, bool is2d ) \
{ \
    uiAttrDescEd* de = new clss( p, is2d ); \
    de->setDisplayName( displaynm ); \
    de->setDomainType( domtyp ); \
    de->setDimensionType( dimtyp ); \
    return de; \
} \
\
const char* clss::attribName() const \
{ \
    return attr::attribName(); \
}

#define mInitAttribUI( clss, attr, displaynm, grp ) \
    mInitAttribUIPars(clss,attr,displaynm,grp,uiAttrDescEd::Both, \
	    	      uiAttrDescEd::AnyDim)

#endif
