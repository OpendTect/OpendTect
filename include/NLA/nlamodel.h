#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "nlamod.h"
#include "nladesign.h"

/*!\brief Minimum Interface for NLA models */

mExpClass(NLA) NLAModel
{
public:

    virtual				~NLAModel();

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
					//!< 'serialize' - without the pars()

    virtual const char*			nlaType( bool compact=true ) const
					{ return compact ? "NN"
							 : "Neural Network"; }

protected:
					NLAModel();
};


mGlobal(NLA) bool isEmpty(const NLAModel* mdl);
