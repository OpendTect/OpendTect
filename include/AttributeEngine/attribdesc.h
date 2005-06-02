#ifndef attribdesc_h
#define attribdesc_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdesc.h,v 1.12 2005-06-02 10:37:53 cvshelene Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "bufstring.h"
#include "seistype.h"

class DataInpSpec;

namespace Attrib
{

class Desc;
class DescSet;
class Param;

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

			Desc( const Desc& );
			Desc( DescSet* descset );
			Desc( const char* basename,DescStatusUpdater updater=0,
			      DescChecker=0);
//    void		erase() { deepErase( params );
//	    			deepUnRef( inputs ); }

    const char*		attribName() const;
    Desc*		clone() const;

    bool		init() { return true; }  ; //Needed?

    void		setDescSet( DescSet* );
    DescSet*		descSet() const;

    int			id() const;

    bool		getDefStr(BufferString&) const;
    bool		parseDefStr( const char* );
    const char*         userRef() const;
    void                setUserRef( const char* );

    int			nrOutputs() const;
    void	        selectOutput(int);
    int			selectedOutput() const;
    Seis::DataType	dataType() const;
    void		setSteering(bool yn)		{ issteering=yn; }
    bool		isSteering() const		{ return issteering; }
    void		setHidden(bool yn)		{ hidden_ = yn; }
    bool		isHidden() const		{ return hidden_; }
    bool		isStored() const;

    int			nrInputs() const;
    InputSpec&		inputSpec(int);
    const InputSpec&	inputSpec(int) const;
    bool        	setInput(int input,Desc*);
    Desc*		getInput(int);
    const Desc*		getInput(int) const;
    bool		is2D() const;

    int			isSatisfied() const;
			/*!< Checks wether all inputs are satisfied. 
			   \retval 0 Nothing to complain
			   \retval 1 Waring
			   \retval 2 Error
			*/

    bool		isIdenticalTo( const Desc&, bool cmpoutput=true ) const;


    			/* Interface to factory */
    void		addParam( Param* );
    const Param*	getParam( const char* key ) const;
    Param*		getParam( const char* key );
    void		setParamEnabled( const char* key, bool yn=true );
    bool		isParamEnabled( const char* key ) const;
    void		setParamRequired( const char* key, bool yn=true );
    bool		isParamRequired( const char* key ) const;

    bool		setParamVal( const char* key, const char* val );
    //bool		getParamVal( const char* key, BufferString& ) const;
    //const DataInpSpec*	getParamSpec( const char* key );

    void		addInput( const InputSpec& );
    bool		removeInput(int idx);
    void		removeOutputs();
    void		addOutputDataType( Seis::DataType );
    void		setNrOutputs( Seis::DataType, int );
    void		addOutputDataTypeSameAs( int );

    static bool		getAttribName( const char* defstr, BufferString& );
    static bool		getParamString( const char* defstr, const char* key,
					BufferString& );

protected:
    Param*			findParam( const char* key );
    TypeSet<Seis::DataType>	outputtypes;
    TypeSet<int>		outputtypelinks;
    bool			issteering;
    bool			hidden_;

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

}; //Namespace

#define mGetBool( var, varstring ) \
var = desc.getParam(varstring)->getBoolValue(0); \

#define mGetEnum( var, varstring ) \
var = desc.getParam(varstring)->getIntValue(0); \

#define mGetBinID( var, varstring ) \
{\
    var.inl = desc.getParam(varstring)->getIntValue(0); \
    var.crl = desc.getParam(varstring)->getIntValue(1); \
}

#define mGetFloatInterval( var, varstring ) \
{\
    var.start = desc.getParam(varstring)->getfValue(0); \
    var.stop = desc.getParam(varstring)->getfValue(1); \
}


#endif


