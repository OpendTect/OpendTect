#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mmprocmod.h"
#include "datainpspec.h"
#include "namedobj.h"


namespace MMProc
{

/*!
\brief Distributed computing specification.
*/

mExpClass(MMProc) ProgSpec : public NamedObject
{
public:

    				ProgSpec( const char* nm, const char* pp,
					  const char* pnm )
				    : NamedObject(nm)
				    , purpose_(pp)
				    , programname_(pnm)	{}

    const char*			purpose() const	// only offer for this purpose
				{ return purpose_; }
    const char*			programName() const
				{ return programname_; }

    mClass(MMProc) Option : public NamedObject
    {
				Option(const char* usrtxt)
				    : NamedObject(usrtxt)
				    , inpspec_(0)	{}
				~Option()		{ delete inpspec_; }

	DataInpSpec*		inpspec_; //!< null=text
    };

    virtual void		getSplitOptions(ObjectSet<Option>&) const {}
    virtual void		split(const IOPar& inp,const IOPar& opts,
	    				ObjectSet<IOPar>&) const = 0;

    virtual void		getExecuteOptions(ObjectSet<Option>&) const {}
    				//!< Will be set in each IOPar

    static int			add(ProgSpec*);

protected:

    const BufferString		purpose_;
    const BufferString		programname_;

};

mGlobal(MMProc) ObjectSet<ProgSpec>& PRSPS();


} // namespace MMProc
