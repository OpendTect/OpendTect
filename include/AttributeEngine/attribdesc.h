#ifndef attribdesc_h
#define attribdesc_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribdesc.h,v 1.1 2005-01-26 09:15:22 kristofer Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "bufstring.h"

namespace Attrib
{

class DescSet;

class Desc
{ mRefCountImpl(Desc);
public:

			Desc();
    void		setDescSet( DescSet* );
    DescSet*		descSet() const;

    int			id() const;

    virtual const char* defStr() const                          = 0;
    const char*         userRef() const;
    void                setUserRef( const char* );

    virtual int		nrOutputs() const;
    virtual void        selectOutput(int);
    virtual int         selectedOutput() const;

    virtual int		nrInputs() const;
    virtual void        setInput( int input, Desc* );
    Desc*		getInput( int );

    virtual int         isSatisfied() const;
			/*!< Checks wether all inputs are satisfied. 
			   \retval 0 Nothing to complain
			   \retval 1 Waring
			   \retval 2 Error
			*/

    virtual bool	isIdenticalTo( const Desc&, bool cmpoutput=true ) const;

    virtual Desc*	clone()                                 = 0;
	
protected:
    BufferString        userref;
    DescSet*		ds;
};

}; //Namespace


#endif


