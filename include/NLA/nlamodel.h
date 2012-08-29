#ifndef nlamodel_h
#define nlamodel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		June 2001
 RCS:		$Id: nlamodel.h,v 1.11 2012-08-29 07:56:39 cvskris Exp $
________________________________________________________________________

-*/

#include "nlamod.h"
#include "gendefs.h"
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
    virtual float			versionNr() const		= 0;

    virtual IOPar&			pars()				= 0;
    const IOPar&			pars() const
					{ return const_cast<NLAModel*>
						 (this)->pars(); }
    					//!< Attrib set in/out

    virtual void			dump(BufferString&) const	= 0;
    					//!< 'serialise' - without the pars()

    virtual const char*			nlaType( bool compact=true ) const
    					{ return compact ? "NN"
					    		 : "Neural Network"; }

};


/*!\mainpage Non-Linear Analysis interfaces

  In order to accommodate more advanced methods than Attribute evaluation
  only, an interface for non-linear analysis is defined. The current
  implementation in the dGB plugins use Neural networks to accomplish
  a training-applying scheme using Neural Networks.

  If you want to make your own NLA module, make it comply this interface
  and provide a user interface via the uiNLA interface. Then use the
  setNlaServer() method on the uiODApplMgr to make your UI active.

*/


#endif

