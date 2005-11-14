#ifndef attribdesc_h
#define attribdesc_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdesc.h,v 1.25 2005-11-14 16:10:23 cvskris Exp $
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
var = ((ValParam*)desc.getParam(varstring))->getIntValue(0); \

#define mGetFloat( var, varstring ) \
var = ((ValParam*)desc.getParam(varstring))->getfValue(0); \

#define mGetBool( var, varstring ) \
var = ((ValParam*)desc.getParam(varstring))->getBoolValue(0); \

#define mGetEnum( var, varstring ) \
var = ((ValParam*)desc.getParam(varstring))->getIntValue(0); \

#define mGetString( var, varstring ) \
var = ((ValParam*)desc.getParam(varstring))->getStringValue(0); \
    
#define mGetBinID( var, varstring ) \
{\
    var.inl = ((ValParam*)desc.getParam(varstring))->getIntValue(0); \
    var.crl = ((ValParam*)desc.getParam(varstring))->getIntValue(1); \
}

#define mGetFloatInterval( var, varstring ) \
{\
    var.start = ((ValParam*)desc.getParam(varstring))->getfValue(0); \
    var.stop = ((ValParam*)desc.getParam(varstring))->getfValue(1); \
}


#endif


