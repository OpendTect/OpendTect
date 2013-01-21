#ifndef mmprogspec_h
#define mmprogspec_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jul 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "mmprocmod.h"
#include "datainpspec.h"
#include "namedobj.h"
class IOPar;

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

    class Option : public NamedObject
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


}; // namespace MMProc


#endif

