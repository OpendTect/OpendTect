#ifndef attribdesc_h
#define attribdesc_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdesc.h,v 1.29 2006-04-18 07:30:00 cvshelene Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "bufstring.h"
#include "bufstringset.h"
#include "seistype.h"
#include "attribdescid.h"

class DataInpSpec;
class MultiID;

namespace Attrib
{

class Desc;
class DescSet;
class Param;
class ValParam;

typedef void(*DescStatusUpdater)(Desc&);
typedef bool(*DescChecker)(const Desc&);

class InputSpec
{
public:
    				InputSpec( const char* d, bool enabled_ )
				    : desc(d), enabled(enabled_)
				    , issteering(false)	{}

    const char*        		getDesc() const { return desc; }

    BufferString		desc;
    TypeSet<Seis::DataType>	forbiddenDts;
    bool			enabled;
    bool			issteering;

    bool			operator==(const InputSpec&) const;
};



class Desc
{ mRefCountImpl(Desc);
public:

			Desc(const Desc&);
			Desc(const char* basename,DescStatusUpdater updater=0,
			     DescChecker=0);
//    void		erase() { deepErase( params );
//	    			deepUnRef( inputs ); }

    const char*		attribName() const;
    Desc*		clone() const;
    bool		init()				{ return true; }

    void		setDescSet(DescSet*);
    DescSet*		descSet() const;

    DescID		id() const;

    bool		getDefStr(BufferString&) const;
    bool		parseDefStr(const char*);
    void		getKeysVals(const char*,BufferStringSet&,
	    			    BufferStringSet&);
    const char*         userRef() const;
    void                setUserRef(const char*);

    int			nrOutputs() const;
    void	        selectOutput(int);
    int			selectedOutput() const;
    Seis::DataType	dataType() const;
    void		setStoredDataType(Seis::DataType);
    void		setSteering(bool yn)		{ issteering=yn; }
    bool		isSteering() const		{ return issteering; }
    void		setHidden(bool yn)		{ hidden_ = yn; }
    bool		isHidden() const		{ return hidden_; }
    bool		isStored() const;
    bool		getMultiID(MultiID&) const;
    void		setNeedProvInit( bool yn=true )	{ needprovinit_ = yn; }
    bool		needProvInit() const		{ return needprovinit_;}

    int			nrInputs() const;
    int                 nrSpecs() const         { return inputspecs.size(); }
    InputSpec&		inputSpec(int);
    const InputSpec&	inputSpec(int) const;
    bool        	setInput(int input,Desc*);
    Desc*		getInput(int);
    const Desc*		getInput(int) const;
    bool		is2D() const;
    void		set2d(bool);

    enum SatisfyLevel	{ AllOk, Warning, Error };
    SatisfyLevel	isSatisfied() const;
			/*!< Checks wether all inputs are satisfied. */

    bool		isIdenticalTo(const Desc&,bool cmpoutput=true) const;
    bool		isIdentifiedBy(const char*) const;
    DescID		inputId(int idx) const;

    			/* Interface to factory */
    void		addParam(Param*);
    const Param*	getParam(const char* key) const;
    Param*		getParam(const char* key);
    const ValParam*	getValParam(const char* key) const;
    ValParam*		getValParam(const char* key);
    void		setParamEnabled(const char* key,bool yn=true);
    bool		isParamEnabled(const char* key) const;
    void		setParamRequired(const char* key,bool yn=true);
    bool		isParamRequired(const char* key) const;

    void		updateParams();
    void		changeStoredID(const char*);

    void		addInput(const InputSpec&);
    bool		removeInput(int idx);
    void		removeOutputs();
    void		addOutputDataType(Seis::DataType);
    void		setNrOutputs(Seis::DataType,int);
    void		addOutputDataTypeSameAs(int);

    static bool		getAttribName(const char* defstr,BufferString&);
    static bool		getParamString(const char* defstr,const char* key,
				       BufferString&);

    Desc*		getStoredInput() const;
    
    static const char*  sKeyInlDipComp();
    static const char*  sKeyCrlDipComp();
    
protected:
    Param*			findParam(const char* key);
    TypeSet<Seis::DataType>	outputtypes;
    TypeSet<int>		outputtypelinks;
    bool			issteering;
    bool			hidden_;
    bool			needprovinit_;
    bool 			is2d;
    bool 			is2dset;

    TypeSet<InputSpec>		inputspecs;
    ObjectSet<Desc>		inputs;

    BufferString		attribname;
    ObjectSet<Param>		params;

    BufferString        	userref;
    int				seloutput;
    DescSet*			ds;

    DescStatusUpdater		statusupdater;
    DescChecker			descchecker;

//    IOObj*              	getDefCubeIOObj(bool,bool) const;
};

}; // namespace Attrib

#define mGetInt( var, varstring ) \
{\
    var = desc.getValParam(varstring)->getIntValue(0); \
    if ( mIsUdf(var) )\
        var = desc.getValParam(varstring)->getDefaultIntValue(0);\
}

#define mGetFloat( var, varstring ) \
{\
    var = desc.getValParam(varstring)->getfValue(0); \
    if ( mIsUdf(var) )\
        var = desc.getValParam(varstring)->getDefaultfValue(0);\
}

#define mGetBool( var, varstring ) \
{\
    Attrib::ValParam* valparam##var = \
            const_cast<Attrib::ValParam*>(desc.getValParam(varstring));\
    mDynamicCastGet(Attrib::BoolParam*,boolparam##var,valparam##var);\
    if ( boolparam##var ) \
        var = boolparam##var->isSet() ? boolparam##var->getBoolValue(0)\
				      : boolparam##var->getDefaultBoolValue(0);\
}

#define mGetEnum( var, varstring ) \
{\
    Attrib::ValParam* valparam##var = \
            const_cast<Attrib::ValParam*>(desc.getValParam(varstring));\
    mDynamicCastGet(Attrib::EnumParam*,enumparam##var,valparam##var);\
    if ( enumparam##var ) \
        var = enumparam##var->isSet() ? enumparam##var->getIntValue(0)\
				      : enumparam##var->getDefaultIntValue(0);\
}

#define mGetString( var, varstring ) \
{\
    var = desc.getValParam(varstring)->getStringValue(0); \
    if ( !strcmp( var, "" ) )\
        var = desc.getValParam(varstring)->getDefaultStringValue(0); \
}
    
#define mGetBinID( var, varstring ) \
{\
    var.inl = desc.getValParam(varstring)->getIntValue(0); \
    var.crl = desc.getValParam(varstring)->getIntValue(1); \
    if ( mIsUdf(var.inl) || mIsUdf(var.crl) )\
    {\
	Attrib::ValParam* valparam##var = \
	      const_cast<Attrib::ValParam*>(desc.getValParam(varstring));\
	mDynamicCastGet(Attrib::BinIDParam*,binidparam##var,valparam##var);\
	if ( binidparam##var ) \
	    var = binidparam##var->getDefaultBinIDValue();\
    }\
}

#define mGetFloatInterval( var, varstring ) \
{\
    var.start = desc.getValParam(varstring)->getfValue(0); \
    var.stop = desc.getValParam(varstring)->getfValue(1); \
    if ( mIsUdf(var.start) || mIsUdf(var.stop) )\
    {\
	Attrib::ValParam* valparam##var = \
	      const_cast<Attrib::ValParam*>(desc.getValParam(varstring));\
	mDynamicCastGet(Attrib::ZGateParam*,gateparam##var,valparam##var);\
	if ( gateparam##var ) \
	    var = gateparam##var->getDefaultZGateValue();\
    }\
}

#endif


