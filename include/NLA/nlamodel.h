#ifndef nlamodel_h
#define nlamodel_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		June 2001
 RCS:		$Id: nlamodel.h,v 1.3 2003-11-07 12:21:52 bert Exp $
________________________________________________________________________

-*/

#include <gendefs.h>
class IOPar;
class NLADesign;
class BufferString;


/*\brief Minimum Interface for NLA models */


class NLAModel
{
public:

    virtual				~NLAModel()			{}

    virtual const char*			name() const			= 0;
    virtual const NLADesign&		design() const			= 0;
    virtual NLAModel*			clone()	const			= 0;
    virtual IOPar&			pars()				= 0;
    					//!< Attrib set in/out
    virtual void			dump(BufferString&) const	= 0;
    					//!< 'serialise' - without the pars()

    virtual const char*			nlaType( bool compact=true ) const
    					{ return compact ? "NN"
					    		 : "Neural Network"; }

};


#endif
