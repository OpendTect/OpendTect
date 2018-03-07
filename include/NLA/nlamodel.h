#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2001
________________________________________________________________________

-*/

#include "nlamod.h"
#include "gendefs.h"

class NLADesign;

/*!
\brief Minimum Interface for NLA models
*/

mClass(NLA) NLAModel
{
public:

    virtual				~NLAModel()			{}

    virtual bool			isEmpty() const			= 0;
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

};


mGlobal(NLA) bool isEmpty(const NLAModel*);
