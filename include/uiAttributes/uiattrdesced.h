#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uigroup.h"
#include "uiattribfactory.h"
#include "changetracker.h"
#include "paramsetget.h"
#include "datapack.h"
#include "helpview.h"

namespace Attrib { class Desc; class DescSet; class DescSetMan; };
namespace ZDomain { class Info; }

class uiAttrDescEd;
class uiAttrSel;
class uiAttrSelData;
class uiImagAttrSel;
class uiSteeringSel;
class uiSteerAttrSel;

using namespace Attrib;

/*!
\brief Description of attribute parameters to evaluate.
*/

mExpClass(uiAttributes) EvalParam
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


/*!
\brief Attribute description editor creator.
*/

mExpClass(uiAttributes) uiAttrDescEdCreater
{
public:
    virtual			~uiAttrDescEdCreater()		{}
    virtual uiAttrDescEd*	create(uiParent*) const		= 0;

};


/*!
\brief Attribute description editor.

  Required functions are declared in the macro mDeclReqAttribUIFns. Two of
  those, attribName() and createInstance() are implemented by the mInitAttribUI
  macro.
*/

mExpClass(uiAttributes) uiAttrDescEd : public uiGroup
{ mODTextTranslationClass(uiAttrDescEd);
public:

    virtual		~uiAttrDescEd();

    HelpKey		helpKey() const			{ return helpkey_; }
    HelpKey		videoKey() const		{ return videokey_; }
    void		setVideoKey( const HelpKey& k )	{ videokey_ = k; }

    void		setDesc(Desc*,DescSetMan*);
    void		setDescSet( DescSet* ds )	{ ads_ = ds; }
    Desc*		curDesc()			{ return desc_; }
    const Desc*		curDesc() const			{ return desc_; }

    void		setZDomainInfo(const ZDomain::Info*);
    const ZDomain::Info* getZDomainInfo() const;

    uiString		errMsgStr(Desc* desc);
    virtual uiString	commit(Desc* desc=0);
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

			uiAttrDescEd(uiParent*,bool is2d,const HelpKey&);

    virtual bool	setParameters(const Desc&)	{ return true; }
    virtual bool	getParameters(Desc&)		{ return true; }
    virtual bool	setInput(const Desc&)		{ return true; }
    virtual bool	getInput(Desc&)			{ return true; }
    virtual bool	setOutput(const Desc&)		{ return true; }
    virtual bool	getOutput(Desc&);

    virtual bool	areUIParsOK()			{ return true; }

    void		fillOutput(Desc&,int selout);
    void		fillInp(uiAttrSel*,Desc&,int);
    void		fillInp(uiSteeringSel*,Desc&,int);
    void		fillInp(uiSteerAttrSel*,Desc&,int);

    void		putInp(uiAttrSel*,const Desc&,int inpnr);
    void		putInp(uiSteerAttrSel*,const Desc&,int inpnr);
    void		putInp(uiSteeringSel*,const Desc&,int inpnr);

    bool		needInputUpdate() const		{ return needinpupd_; }

    uiString		zDepLabel(const uiString& pre,
				  const uiString& post) const;
    uiString		gateLabel() const
			{ return zDepLabel( uiString::emptyString(),
					    tr("gate")); }
    uiString		shiftLabel() const
			{ return zDepLabel( uiString::emptyString(),
					    tr("shift"));}
    bool		zIsTime() const;

    ChangeTracker	chtr_;
    uiAttrSel*		createInpFld(bool is2d,const char* txt=0);
    uiAttrSel*		createInpFld(const uiAttrSelData&,const char* txt=0);
    uiImagAttrSel*	createImagInpFld(bool is2d);
    bool		getInputDPID(uiAttrSel*,DataPack::FullID&) const;
    Desc*		getInputDescFromDP(uiAttrSel*) const;

    HelpKey		helpkey_;
    HelpKey		videokey_;
    BufferString	attrnm_;
    DomainType		domtyp_;
    DimensionType	dimtyp_;
    uiString		errmsg_;
    DescSet*		ads_;
    bool		is2d_;
    bool		needinpupd_;
    const ZDomain::Info* zdomaininfo_;

    TypeSet<DataPack::FullID> dpfids_;

    static const char*	sKeyOtherGrp();
    static const char*	sKeyBasicGrp();
    static const char*	sKeyFilterGrp();
    static const char*	sKeyFreqGrp();
    static const char*	sKeyPatternGrp();
    static const char*	sKeyStatsGrp();
    static const char*	sKeyPositionGrp();
    static const char*	sKeyDipGrp();

    static uiString	sInputTypeError(int input);



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
    static void enableClass(bool yn); \
    static void removeClass(); \
    const char* attribName() const override; \
    static int factoryID() { return factoryid_; }


#define mInitAttribUIPars( clss, attr, displaynm, grp, domtyp, dimtyp ) \
\
int clss::factoryid_ = -1; \
\
void clss::initClass() \
{ \
    factoryid_ = uiAF().add( displaynm, attr::attribName(), grp, \
		 clss::createInstance, (int)domtyp, (int)dimtyp ); \
} \
\
void clss::enableClass( bool yn ) \
{ uiAF().enable( attr::attribName(), yn ); } \
\
void clss::removeClass() \
{ uiAF().remove( attr::attribName() ); } \
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
