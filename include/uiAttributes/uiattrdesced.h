#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uigroup.h"
#include "uiattribfactory.h"
#include "changetracker.h"
#include "datapack.h"
#include "helpview.h"
#include "uistrings.h"

namespace Attrib { class Desc; class DescSet; }
namespace ZDomain { class Info; }

class uiAttrDescEd;
class uiAttrSel;
class uiAttrSelData;
class uiAttrTypeSel;
class uiImagAttrSel;
class uiSteeringSel;
class uiSteerAttrSel;


/*!\brief Description of attribute parameters to evaluate.  */

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


/*!\brief Attribute description editor creator.  */

mExpClass(uiAttributes) uiAttrDescEdCreater
{
public:
    virtual			~uiAttrDescEdCreater()		{}
    virtual uiAttrDescEd*	create(uiParent*) const		= 0;

};


/*!\brief Attribute description editor.

  Required functions are declared in the macro mDeclReqAttribUIFns. Two of
  those, attribName() and createInstance() are implemented by the mInitAttribUI
  macro.
*/

mExpClass(uiAttributes) uiAttrDescEd : public uiGroup
{ mODTextTranslationClass(uiAttrDescEd);
public:

    typedef Attrib::Desc	Desc;
    typedef Attrib::DescSet	DescSet;
    enum DomainType		{ TimeAndDepth, Time, Depth };
    enum DimensionType		{ AnyDim, Only3D, Only2D };
    enum SynthAttrType		{ Not4Synth, Usable4Synth };

    virtual		~uiAttrDescEd();
    HelpKey		helpKey()			{ return helpkey_; }
    void		setDesc(Desc*);
    void		setDescSet(DescSet*);
    DescSet*		descSet() const			{ return ads_; }
    Desc*		curDesc()			{ return desc_; }
    const Desc*		curDesc() const			{ return desc_; }

    void		setZDomainInfo(const ZDomain::Info*);
    const ZDomain::Info* getZDomainInfo() const;

    uiRetVal		errMsgs(Desc* desc);
    virtual uiRetVal	commit(Desc* desc=0);
			//!< returns null on success, error message otherwise
			//!< If attribdesc is non-zero, that desc will be
			//!< filled. If not, the internal desc will be filled.

    virtual int		getOutputIdx(float val) const	{ return (int)val; }
    virtual float	getOutputValue(int idx) const	{ return (float)idx; }
    virtual void	setOutputStep(float step)	{}

    virtual void	getEvalParams(TypeSet<EvalParam>&) const {}

    virtual const char*	attribName() const		= 0;
    virtual uiString	displayName() const		= 0;
    virtual uiString	groupName() const		= 0;
    virtual DomainType	domainType() const		= 0;
    virtual DimensionType dimensionType() const		= 0;

    bool		is2D() const			{ return is2d_; }

    virtual void	setDataPackInp(const TypeSet<DataPack::FullID>&);

    static const char*	getInputAttribName(uiAttrSel*,const Desc&);

    static const char*	timegatestr();
    static const char*	frequencystr();
    static const char*	stepoutstr();
    static const char*	filterszstr();

    Notifier<uiAttrDescEd>  descChanged;
    Notifier<uiAttrDescEd>  descSetChanged;

    void		setInitialDefaults(const DescSet&);

protected:

			uiAttrDescEd(uiParent*,bool is2d,const HelpKey&);

    virtual bool	setParameters(const Desc&)	{ return true; }
    virtual bool	getParameters(Desc&)		{ return true; }
    virtual bool	setInput(const Desc&)		{ return true; }
    virtual uiRetVal	getInput(Desc&)
						{ return uiRetVal::OK(); }
    virtual bool	setOutput(const Desc&)		{ return true; }
    virtual bool	getOutput(Desc&);

    virtual uiRetVal	areUIParsOK()		{ return uiRetVal::OK(); }

    void		fillOutput(Desc&,int selout);
    uiRetVal		fillInp(uiAttrSel*,Desc&,int);
    uiRetVal		fillInp(uiSteeringSel*,Desc&,int);
    uiRetVal		fillInp(uiSteerAttrSel*,Desc&,int);

    void		putInp(uiAttrSel*,const Desc&,int inpnr);
    void		putInp(uiSteerAttrSel*,const Desc&,int inpnr);
    void		putInp(uiSteeringSel*,const Desc&,int inpnr);

    uiString		zDepLabel(const uiString& pre,
				  const uiString& post) const;
    uiString		gateLabel() const
			{ return zDepLabel( uiString::empty(),
					    uiStrings::sGate() ); }
    uiString		shiftLabel() const
			{ return zDepLabel( uiString::empty(),
				    uiStrings::sShift().toLower());}
    bool		zIsTime() const;

    uiAttrSel*		createInpFld(bool is2d,const uiString& =sDefLabel());
    uiAttrSel*		createInpFld(const uiAttrSelData&,
				     const uiString& =sDefLabel());

    uiImagAttrSel*	createImagInpFld(bool is2d);
    bool		getInputDPID(uiAttrSel*,DataPack::FullID&) const;
    Desc*		getInputDescFromDP(uiAttrSel*) const;

    ChangeTracker	chtr_;
    HelpKey		helpkey_;
    DescSet*		ads_;
    bool		is2d_;
    const ZDomain::Info* zdomaininfo_;

    TypeSet<DataPack::FullID> dpfids_;

    static uiString	sOtherGrp();
    static uiString	sBasicGrp();
    static uiString	sFilterGrp();
    static uiString	sFreqGrp();
    static uiString	sPatternGrp();
    static uiString	sStatsGrp();
    static uiString	sPositionGrp();
    static uiString	sDipGrp();
    static uiString	sTraceMatchGrp();
    static uiString	sExperimentalGrp();
    static uiString	sInputTypeError(int input);
    static uiString	sDefLabel(); // uiAttrSel::sDefLabel()

private:

    Desc*		desc_;

    friend class	uiAttrTypeSel;

};


#include "paramsetget.h"

#define mDeclReqAttribUIFns \
protected: \
    static uiAttrDescEd* createInstance(uiParent*,bool); \
    static int		factoryid_; \
    static BufferString	attrnm_; \
    static uiString	groupname_; \
    static uiString	dispname_; \
    static DomainType	domtyp_; \
    static DimensionType dimtyp_; \
public: \
    static void		initClass(); \
    static void		enableClass(bool yn); \
    static void		removeClass(); \
    static int		factoryID()		{ return factoryid_; } \
    const char*		attribName() const	{ return attrnm_; } \
    uiString		displayName() const	{ return dispname_; } \
    uiString		groupName() const	{ return groupname_; } \
    DomainType		domainType() const	{ return domtyp_; } \
    DimensionType	dimensionType() const	{ return dimtyp_; }


#define mGenInitAttribUIPars( clss, attr, dispnm, grp, domtyp, dimtyp, \
			      synthtyp, isgrpdef) \
\
int clss::factoryid_ = -1; \
BufferString clss::attrnm_; \
uiString clss::groupname_; \
uiString clss::dispname_; \
uiAttrDescEd::DomainType clss::domtyp_ = uiAttrDescEd::domtyp; \
uiAttrDescEd::DimensionType clss::dimtyp_ = uiAttrDescEd::dimtyp; \
\
void clss::initClass() \
{ \
    if ( factoryid_ < 0 ) \
	factoryid_ = uiAF().add( dispnm, attr::attribName(), grp, \
		clss::createInstance, sCast(int,domtyp), sCast(int,dimtyp), \
		synthtyp == Usable4Synth, isgrpdef ); \
    dispname_ = dispnm; domtyp_ = domtyp; dimtyp_ = dimtyp; \
    attrnm_ = attr::attribName(); \
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
    return new clss( p, is2d ); \
} \

#define mInitAttribUIPars(clss,attr,dispnm,grp,domtyp,dimtyp,synthtype) \
mGenInitAttribUIPars(clss,attr,dispnm,grp,domtyp,dimtyp,synthtype,false)

#define mInitAttribUI( clss, attr, dispnm, grp ) \
mInitAttribUIPars(clss,attr,dispnm,grp,TimeAndDepth,AnyDim,Usable4Synth)

#define mInitAttribUINoSynth( clss, attr, dispnm, grp ) \
mInitAttribUIPars(clss,attr,dispnm,grp,TimeAndDepth,AnyDim,Not4Synth)

#define mInitGrpDefAttribUI( clss, attr, dispnm, grp ) \
mGenInitAttribUIPars(clss,attr,dispnm,grp,TimeAndDepth,AnyDim,Usable4Synth,true)

#define mInitGrpDefAttribUINoSynth( clss, attr, dispnm, grp ) \
mGenInitAttribUIPars(clss,attr,dispnm,grp,TimeAndDepth,AnyDim,Not4Synth,true)
