#ifndef attribdesc_h
#define attribdesc_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdesc.h,v 1.2 2005-02-01 14:05:34 kristofer Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "bufstring.h"
#include "seistype.h"

namespace Attrib
{

class Desc;
class DescSet;
class Param;

typedef void(*DescStatusUpdater)(Desc&);

class InputSpec
{
public:
    				InputSpec( const char* d, bool enabled_ )
				    : desc( d ) , enabled( enabled_ )
				    , issteering(false) {}

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
			Desc( const char* basename,DescStatusUpdater updater=0);

    const char*		attribName() const;
    Desc*		clone() const;

    bool		init(); //Needed?

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
    bool		isSteering() const { return issteering; }

    int			nrInputs() const;
    InputSpec&		inputSpec( int );
    const InputSpec&	inputSpec( int ) const;
    bool        	setInput( int input, Desc* );
    Desc*		getInput( int );

    int			isSatisfied() const;
			/*!< Checks wether all inputs are satisfied. 
			   \retval 0 Nothing to complain
			   \retval 1 Waring
			   \retval 2 Error
			*/

    bool		isIdenticalTo( const Desc&, bool cmpoutput=true ) const;

    void		addParam( Param* );
    const Param*	getParam( const char* key ) const;
    Param*		getParam( const char* key );
    void		setParamEnabled( const char* key, bool yn=true );
    bool		isParamEnabled( const char* key ) const;
    void		setParamRequired( const char* key, bool yn=true );
    bool		isParamRequired( const char* key ) const;

    void		addInput( const InputSpec& );
    void		addOutputDataType( Seis::DataType );
    void		addOutputDataTypeSameAs( int );
    void		setSteering(bool yn) { issteering=yn; }

    static bool		getAttribName( const char* defstr, BufferString& );
    static bool		getParamString( const char* defstr, const char* key,
					BufferString& );

protected:
    TypeSet<Seis::DataType>	outputtypes;
    TypeSet<int>		outputtypelinks;
    bool			issteering;

    TypeSet<InputSpec>		inputspecs;
    ObjectSet<Desc>		inputs;

    BufferString		attribname;
    ObjectSet<Param>		params;

    BufferString        	userref;
    int				seloutput;
    DescSet*			ds;

    DescStatusUpdater		statusupdater;
};

}; //Namespace

#define mGetBool( var, varstring ) \
var = desc.getParam(varstring)->getSpec()->getBoolValue(0); \

#define mGetEnum( var, varstring ) \
var = desc.getParam(varstring)->getSpec()->getIntValue(0); \

#define mGetBinID( var, varstring ) \
{\
    var.inl = desc.getParam(varstring)->getSpec()->getIntValue(0); \
    var.crl = desc.getParam(varstring)->getSpec()->getIntValue(1); \
}

#define mGetFloatInterval( var, varstring ) \
{\
    var.start = desc.getParam(varstring)->getSpec()->getfValue(0); \
    var.stop = desc.getParam(varstring)->getSpec()->getfValue(1); \
}


#endif


