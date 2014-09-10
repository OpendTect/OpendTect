#ifndef prestacktrimstatics_h
#define prestacktrimstatics_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "prestackprocessingmod.h"
#include "prestackprocessor.h"
#include "multiid.h"

class Muter;

namespace PreStack
{

class MuteDef;

/*!
\brief Processor for PreStack Trim Statics.
*/

mExpClass(PreStackProcessing) TrimStatics : public Processor
{ mODTextTranslationClass(TrimStatics)
public:
			mDefaultFactoryInstantiation(
				Processor, TrimStatics,
				"Trim Statics", sFactoryKeyword() )

 				TrimStatics();
    				~TrimStatics();

    bool			prepareWork();

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    uiString			errMsg() const		{ return errmsg_; }

protected:

    uiString			errmsg_;

    od_int64			nrIterations() const { return outidx_.size(); }
    bool			doWork(od_int64,od_int64,int);

};

} // namespace PreStack

#endif
