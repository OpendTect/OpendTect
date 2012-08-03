#ifndef attribdesc_h
#define attribdesc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdesc.h,v 1.57 2012-08-03 13:00:07 cvskris Exp $
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "refcount.h"
#include "bufstring.h"
#include "bufstringset.h"
#include "seistype.h"
#include "attribdescid.h"
#include "typeset.h"


namespace Attrib
{

class Desc;
class Param;
class DescSet;
class ValParam;

typedef void(*DescStatusUpdater)(Desc&);
typedef void(*DescDefaultsUpdater)(Desc&);

mClass(AttributeEngine) DescSetup
{
    public:
				    DescSetup();
	mDefSetupClssMemb(DescSetup,bool,is2d);
	mDefSetupClssMemb(DescSetup,bool,ps);
	mDefSetupClssMemb(DescSetup,bool,singletraceonly);
	mDefSetupClssMemb(DescSetup,bool,usingtrcpos);
	mDefSetupClssMemb(DescSetup,bool,depthonly);
	mDefSetupClssMemb(DescSetup,bool,timeonly);
	mDefSetupClssMemb(DescSetup,bool,hidden);
	mDefSetupClssMemb(DescSetup,bool,steering);
	mDefSetupClssMemb(DescSetup,bool,stored);
};


mClass(AttributeEngine) InputSpec
{
public:
    				InputSpec( const char* d, bool enabled )
				    : desc_(d), enabled_(enabled)
				    , issteering_(false)		{}

    const char*        		getDesc() const { return desc_; }

    BufferString		desc_;
    TypeSet<Seis::DataType>	forbiddenDts_;
    bool			enabled_;
    bool			issteering_;

    bool			operator==(const InputSpec&) const;
};


/*!Description of an attribute in an Attrib::DescSet. Each attribute has
   a name (e.g. "Similarity"), a user reference (e.g. "My similarity"), and at
   least one output. In addition, it may have parameters and inputs. If it has
   multiple outputs, only one of the outputs are selected.

   The attrib name, the parameters and the selected output number together form
   a definition string that define what the attribute calculates.

   Each Desc has DescID that is unique within it's DescSet.
 */

mClass(AttributeEngine) Desc
{ mRefCountImpl(Desc);
public:

    enum Locality		{ SingleTrace, PossiblyMultiTrace, MultiTrace };

				Desc(const Desc&);
				Desc(const char* attrname,
				     DescStatusUpdater updater=0,
				     DescDefaultsUpdater defupdater=0);

    const char*			attribName() const;

    void			setDescSet(DescSet*);
    DescSet*			descSet() const;
    DescID			id() const;
    bool			getParentID(DescID cid,DescID& pid,int&) const;
    void			getAncestorIDs(DescID cid,
				TypeSet<Attrib::DescID>&,TypeSet<int>&) const;
    				/*ordered from parent to oldest original*/

    bool			getDefStr(BufferString&) const;
    bool			parseDefStr(const char*);

    const char*			userRef() const;
    void			setUserRef(const char*);

    int				nrOutputs() const;
    void			selectOutput(int);
    int				selectedOutput() const;

    Seis::DataType		dataType(int output=-1) const;
				/*!<\param output specifies which output is 
				     required, or -1 for the selected output.*/
    Locality			locality() const	  { return locality_; }
    void			setLocality( Locality l ) { locality_ = l; }
    bool			usesTracePosition() const { return usestrcpos_;}
    void			setUsesTrcPos( bool yn )  { usestrcpos_ = yn; }
    bool			isSteering() const        { return issteering_;}
    void			setSteering( bool yn )    { issteering_ = yn; }
    bool			isHidden() const	{ return hidden_; }
    				/*!<If hidden, it won't show up in UI. */
    void			setHidden( bool yn )	{ hidden_ = yn; }
    				/*!<If hidden, it won't show up in UI. */

    bool			isStored() const;
    bool			isStoredInMem() const;
    BufferString		getStoredID(bool recursive=false) const;

    void			setNeedProvInit( bool yn=true )
    				{ needprovinit_ = yn; }
    bool			needProvInit() const
    				{ return needprovinit_;}

    int				nrInputs() const;
    InputSpec&			inputSpec(int);
    const InputSpec&		inputSpec(int) const;
    bool        		setInput(int,const Desc*);
    Desc*			getInput(int);
    const Desc*			getInput(int) const;
    void			getDependencies(TypeSet<Attrib::DescID>&) const;
    				/*!<Generates list of attributes this attribute
				    is dependant on. */

    bool			is2D() const		{ return is2d_; }
    void			set2D( bool yn )	{ is2d_ = yn; }
    bool			isPS() const		{ return isps_; }
    void			setPS( bool yn )	{ isps_ = yn; }

    enum SatisfyLevel		{ AllOk, Warning, Error };
    SatisfyLevel		isSatisfied() const;
				/*!< Checks whether all inputs are satisfied. */

    const char*			errMsg() const;
    void			setErrMsg( const char* str )	{ errmsg_=str; }

    bool			isIdenticalTo(const Desc&,
	    				      bool cmpoutput=true) const;
    bool			isIdentifiedBy(const char*) const;
    DescID			inputId(int idx) const;

    				/* Interface to factory */
    void			addParam(Param*);
    				/*!< Pointer becomes mine */
    const Param*		getParam(const char* key) const;
    Param*			getParam(const char* key);
    const ValParam*		getValParam(const char* key) const;
    ValParam*			getValParam(const char* key);
    void			setParamEnabled(const char* key,bool yn=true);
    bool			isParamEnabled(const char* key) const;
    void			setParamRequired(const char* key,bool yn=true);
    bool			isParamRequired(const char* key) const;

    void			updateParams();
    void			updateDefaultParams();
    void			changeStoredID(const char*);

    void			addInput(const InputSpec&);
    bool			removeInput(int idx);
    void			removeOutputs();
    void			addOutputDataType(Seis::DataType);
    void			setNrOutputs(Seis::DataType,int);
    void			addOutputDataTypeSameAs(int);
    void			changeOutputDataType(int,Seis::DataType);

    static bool			getAttribName(const char* defstr,BufferString&);
    static bool			getParamString(const char* defstr,
	    				       const char* key,BufferString&);

    Desc*			getStoredInput() const;
    
    static const char*		sKeyInlDipComp();
    static const char*		sKeyCrlDipComp();
    
protected:

    bool			setInput_(int,Desc*);
    Param*			findParam(const char* key);

    void			getKeysVals(const char* defstr,
	    			    BufferStringSet& keys,
				    BufferStringSet& vals);
				/*!<Fills \akeys and \avals with pairs of
				    parameters from the defstr. */

    TypeSet<Seis::DataType>	outputtypes_;
    TypeSet<int>		outputtypelinks_;
    bool			issteering_;
    bool			hidden_;
    bool			needprovinit_;
    bool 			is2d_;
    bool 			isps_;
    Locality			locality_;
    bool			usestrcpos_;

    TypeSet<InputSpec>		inputspecs_;
    ObjectSet<Desc>		inputs_;

    BufferString		attribname_;
    ObjectSet<Param>		params_;

    BufferString        	userref_;
    int				seloutput_;
    DescSet*			descset_;

    DescStatusUpdater		statusupdater_;
    DescDefaultsUpdater		defaultsupdater_;
    BufferString		errmsg_;
};

}; // namespace Attrib

#define mGetIntFromDesc( __desc, var, varstring ) \
{\
    var = __desc.getValParam(varstring)->getIntValue(0); \
    if ( mIsUdf(var) )\
        var = __desc.getValParam(varstring)->getDefaultIntValue(0);\
}

#define mGetFloatFromDesc( __desc, var, varstring ) \
{\
    var = __desc.getValParam(varstring)->getfValue(0); \
    if ( mIsUdf(var) )\
        var = __desc.getValParam(varstring)->getDefaultfValue(0);\
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
    if ( !strcmp( var, "" ) )\
        var = __desc.getValParam(varstring)->getDefaultStringValue(0); \
}
    
#define mGetBinIDFromDesc( __desc, var, varstring ) \
{\
    var.inl = __desc.getValParam(varstring)->getIntValue(0); \
    var.crl = __desc.getValParam(varstring)->getIntValue(1); \
    if ( mIsUdf(var.inl) || mIsUdf(var.crl) )\
    {\
	Attrib::ValParam* valparam##var = \
	      const_cast<Attrib::ValParam*>(__desc.getValParam(varstring));\
	mDynamicCastGet(Attrib::BinIDParam*,binidparam##var,valparam##var);\
	if ( binidparam##var ) \
	    var = binidparam##var->getDefaultBinIDValue();\
    }\
    if ( __desc.descSet()->is2D() ) \
    	var.inl = 0; \
}

#define mGetFloatIntervalFromDesc( __desc, var, varstring ) \
{\
    var.start = __desc.getValParam(varstring)->getfValue(0); \
    var.stop = __desc.getValParam(varstring)->getfValue(1); \
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

#endif



