#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
________________________________________________________________________

-*/

#include "attribdescid.h"
#include "refcount.h"
#include "bufstring.h"
#include "bufstringset.h"
#include "seistype.h"
#include "typeset.h"
#include "notify.h"


namespace Attrib
{

class Param;
class ValParam;

typedef void		(*DescUpdater)(Desc&);


/*!\brief A setup class for desc */

mExpClass(AttributeEngine) DescSetup
{
    public:
					DescSetup();

    mDefSetupClssMemb(DescSetup, bool,	is2d)
    mDefSetupClssMemb(DescSetup, bool,	ps)
    mDefSetupClssMemb(DescSetup, bool,	singletraceonly)
    mDefSetupClssMemb(DescSetup, bool,	usingtrcpos)
    mDefSetupClssMemb(DescSetup, bool,	depthonly)
    mDefSetupClssMemb(DescSetup, bool,	timeonly)
    mDefSetupClssMemb(DescSetup, bool,	hidden)
    mDefSetupClssMemb(DescSetup, bool,	steering)
    mDefSetupClssMemb(DescSetup, bool,	stored)

};


/*!
\brief Specification of input data of an attribute.
*/

mExpClass(AttributeEngine) InputSpec
{
public:
			InputSpec( const char* d, bool enabled )
				    : desc_(d), enabled_(enabled)
				    , issteering_(false)		{}

    const char*		getDesc() const { return desc_; }

    BufferString	desc_;
    TypeSet<DataType>	forbiddenDts_;
    bool		enabled_;
    bool		issteering_;

    bool		operator==(const InputSpec&) const;
};


/*!
\brief Description of an attribute in an Attrib::DescSet. Each attribute has
a name (e.g. "Similarity"), a user reference (e.g. "My similarity"), and at
least one output. In addition, it may have parameters and inputs. If it has
multiple outputs, only one of the outputs is selected.

The attrib name, the parameters and the selected output number together form
a definition string that defines what the attribute calculates.

Each Desc has DescID that is unique within it's DescSet.
*/

mExpClass(AttributeEngine) Desc : public RefCount::Referenced
				, public CallBacker
{ mODTextTranslationClass(Desc)
public:

    explicit		Desc(const char* attrname,DescUpdater updater=0,
				     DescUpdater defupdater=0);
			Desc(const Desc&);

    const OD::String&	attribName() const	{ return attribname_; }

    void		setDescSet(DescSet*);
    DescSet*		descSet() const		{ return descset_; }
    DescID		id() const;
    bool		getParentID(DescID cid,DescID& pid,int&) const;
    void		getAncestorIDs(DescID cid,
					TypeSet<DescID>&,TypeSet<int>&) const;
				    /*ordered from parent to oldest original*/

    bool		getDefStr(BufferString&) const;
    bool		parseDefStr(const char*);

    const char*		userRef() const		{ return userref_; }
    void		setUserRef(const char*);

    int			nrOutputs() const	{ return outputtypes_.size(); }
    void		selectOutput( int nr )	{ seloutput_ = nr; }
    int			selectedOutput() const	{ return seloutput_; }

    DataType		dataType(int output=-1) const;
			    /*!<\param -1 for the selected output.*/

    bool		needProvInit() const	{ return needprovinit_; }

    bool		is2D() const		{ return is2d_; }
    bool		isPS() const		{ return isps_; }
    bool		isSingleTrace() const	{ return issingtrc_; }
    bool		isSteering() const	{ return issteering_;}
    bool		isHidden() const	{ return ishidden_; }
    bool		usesTracePosition() const { return usestrcpos_;}

    bool		isStored() const;
    bool		isStoredInMem() const;
    DBKey		getStoredID(bool recursive=false) const;
    BufferString	getStoredType(bool recursive=false) const;

    int			nrInputs() const	{ return inputs_.size(); }
    InputSpec&		inputSpec( int nr )	{ return inputspecs_[nr]; }
    const InputSpec&	inputSpec( int nr ) const { return inputspecs_[nr]; }
    bool		setInput(int,const Desc*);
    Desc*		getInput(int);
    const Desc*		getInput(int) const;
    void		getInputs(TypeSet<Attrib::DescID>&) const;
    void		getDependencies(TypeSet<Attrib::DescID>&) const;
					//!< the attributes this one is dep on

    enum SatisfyLevel	{ AllOk, Warning, StorNotFound, GenError };
    SatisfyLevel	satisfyLevel() const; //!< Mainly checks inputs
    static bool		isError( SatisfyLevel lvl )	{ return lvl>Warning; }
    const uiString	errMsg() const;
    void		setErrMsg( const uiString& msg ) { errmsg_ = msg; }

    bool		isIdenticalTo(const Desc&,bool cmpoutput=true) const;
    bool		isIdentifiedBy(const char*) const;
    DescID		inputId(int idx) const;

    void		addParam( Param* p )		{ params_ += p; }
    const Param*	getParam( const char* ky) const { return findParam(ky);}
    Param*		getParam( const char* ky)	{ return findParam(ky);}
    const ValParam*	getValParam(const char* ky) const;
    ValParam*		getValParam(const char* ky);
    void		setParamEnabled(const char* ky,bool yn=true);
    bool		isParamEnabled(const char* ky) const;
    void		setParamRequired(const char* ky,bool yn=true);
    bool		isParamRequired(const char* ky) const;

    void		updateParams();
    void		updateDefaultParams();
    void		changeStoredID(const DBKey&);

    void		addInput(const InputSpec&);
    bool		removeInput(int idx);
    void		removeOutputs();
    void		addOutputDataType(DataType);
    void		setNrOutputs(DataType,int);
    void		addOutputDataTypeSameAs(int);
    void		changeOutputDataType(int,DataType);

    static bool		getAttribName(const char* defstr,BufferString&);
    static bool		getParamString(const char* defstr,const char* key,
				       BufferString&);

    Desc*		getStoredInput() const;
    DescID		getMultiOutputInputID() const;

    //Used to clone an attribute chain and apply it on multiple components
    //of the same input cube (different offsets for instance)
    Desc*		cloneDescAndPropagateInput(const DescID&,
						   BufferString) const;

    static void		getKeysVals(const char* defstr,
				    BufferStringSet& keys,
				    BufferStringSet& vals,
				    const char* onlyneedkey=0);

    Notifier<Desc>	userRefChanged;

protected:

			~Desc();

    bool		setInput_(int,Desc*);
    Param*		findParam(const char*) const;

    bool		is2d_;
    bool		isps_;
    bool		issingtrc_;
    bool		issteering_;
    bool		ishidden_;
    bool		usestrcpos_;

    bool		needprovinit_;

    TypeSet<DataType>	outputtypes_;
    TypeSet<int>	outputtypeinpidxs_;

    BufferString	attribname_;
    BufferString	userref_;
    DescSet*		descset_;
    ObjectSet<Param>	params_;
    TypeSet<InputSpec>	inputspecs_;
    ObjectSet<Desc>	inputs_;
    int			seloutput_;

    DescUpdater		statusupdater_;
    DescUpdater		defaultsupdater_;
    mutable uiString	errmsg_;

public:

			// maintained by statusupdater_
    void		setIs2D( bool yn )		{ is2d_ = yn; }
    void		setIsPS( bool yn )		{ isps_ = yn; }
    void		setIsSingleTrace( bool yn )	{ issingtrc_ = yn; }
    void		setIsSteering( bool yn )	{ issteering_ = yn; }
    void		setUsesTrcPos( bool yn )	{ usestrcpos_ = yn; }
    void		setIsHidden( bool yn )		{ ishidden_ = yn; }
    void		setNeedProvInit( bool yn=true )	{ needprovinit_ = yn; }

    enum Locality	{ SingleTrace, PossiblyMultiTrace, MultiTrace };
    mDeprecated void	setLocality( Locality loc )
			{ issingtrc_ = loc == SingleTrace; }
    mDeprecated Locality locality() const
			{ return issingtrc_ ? SingleTrace : MultiTrace; }
    mDeprecated void	setHidden( bool yn )		{ setIsHidden(yn); }

};

mGlobal(AttributeEngine) void getIntFromDescStr(Desc&,int&,const char*);

} // namespace Attrib


#define mGetIntFromDesc( __desc, var, varstring ) \
    getIntFromDescStr( __desc, var, varstring )

#define mGetFloatFromDesc( __desc, var, varstring ) \
{\
    var = __desc.getValParam(varstring)->getFValue(0); \
    if ( mIsUdf(var) )\
        var = __desc.getValParam(varstring)->getDefaultFValue(0);\
}


#define mGetDoubleFromDesc( __desc, var, varstring ) \
{\
    var = __desc.getValParam(varstring)->getDValue(0); \
    if ( mIsUdf(var) )\
        var = __desc.getValParam(varstring)->getDefaultDValue(0);\
}


#define mGetBoolFromDesc( __desc, var, varstring ) \
{\
    Attrib::ValParam* valparam##var = \
            const_cast<Attrib::ValParam*>(__desc.getValParam(varstring));\
    mDynamicCastGet(Attrib::BoolParam*,boolparam##var,valparam##var);\
    if ( boolparam##var ) \
        var = boolparam##var->isSet() ? boolparam##var->getBoolValue(0)\
				      : boolparam##var->getDefaultBoolValue(0);\
}

#define mGetEnumFromDesc( __desc, var, varstring ) \
{\
    Attrib::ValParam* valparam##var = \
            const_cast<Attrib::ValParam*>(__desc.getValParam(varstring));\
    mDynamicCastGet(Attrib::EnumParam*,enumparam##var,valparam##var);\
    if ( enumparam##var ) \
        var = enumparam##var->isSet() ? enumparam##var->getIntValue(0)\
				      : enumparam##var->getDefaultIntValue(0);\
}

#define mGetStringFromDesc( __desc, var, varstring ) \
{\
    var = __desc.getValParam(varstring)->getStringValue(0); \
    if ( FixedString(var).isEmpty() )\
        var = __desc.getValParam(varstring)->getDefaultStringValue(0); \
}

#define mGetBinIDFromDesc( __desc, var, varstring ) \
{\
    var.inl() = __desc.getValParam(varstring)->getIntValue(0); \
    var.crl() = __desc.getValParam(varstring)->getIntValue(1); \
    if ( mIsUdf(var.inl()) || mIsUdf(var.crl()) )\
    {\
	Attrib::ValParam* valparam##var = \
	      const_cast<Attrib::ValParam*>(__desc.getValParam(varstring));\
	mDynamicCastGet(Attrib::BinIDParam*,binidparam##var,valparam##var);\
	if ( binidparam##var ) \
	    var = binidparam##var->getDefaultBinIDValue();\
    }\
    if ( __desc.descSet()->is2D() ) \
	var.inl() = 0; \
}

#define mGetFloatIntervalFromDesc( __desc, var, varstring ) \
{\
    var.start = __desc.getValParam(varstring)->getFValue(0); \
    var.stop = __desc.getValParam(varstring)->getFValue(1); \
    if ( mIsUdf(var.start) || mIsUdf(var.stop) )\
    {\
	Attrib::ValParam* valparam##var = \
	      const_cast<Attrib::ValParam*>(__desc.getValParam(varstring));\
	mDynamicCastGet(Attrib::ZGateParam*,gateparam##var,valparam##var);\
	if ( gateparam##var ) \
	    var = gateparam##var->getDefaultGateValue();\
    }\
}

#define mGetFloat( var, varstring ) \
    mGetFloatFromDesc( desc_, var, varstring )
#define mGetInt( var, varstring ) \
    mGetIntFromDesc( desc_, var, varstring )
#define mGetBool( var, varstring ) \
    mGetBoolFromDesc( desc_, var, varstring )
#define mGetEnum( var, varstring ) \
    mGetEnumFromDesc( desc_, var, varstring )
#define mGetString( var, varstring ) \
    mGetStringFromDesc( desc_, var, varstring )
#define mGetBinID( var, varstring ) \
    mGetBinIDFromDesc( desc_, var, varstring )
#define mGetFloatInterval( var, varstring ) \
    mGetFloatIntervalFromDesc( desc_, var, varstring )
#define mGetDouble( var, varstring ) \
    mGetDoubleFromDesc( desc_, var, varstring )
