#ifndef seispsioprov_h
#define seispsioprov_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id: seispsioprov.h,v 1.1 2004-12-30 17:29:35 bert Exp $
________________________________________________________________________

-*/

#include "transl.h"
#include "bufstring.h"
class SeisPSReader;
class SeisPSWriter;
class IOObj;



/*!\brief Pre-Stack Seismics objects provider

  It is not mandatory to provide both reader and writer.
  Null returns must be expected.

  The key provided to makeReader and makeReader must be a file or directory
  name. This name is stored in IOObjs refering to PS data stores.

  IOObjs should always have a sKey::Type -> the type.

 */

class SeisPSIOProvider
{
public:

    virtual		~SeisPSIOProvider()		{}

    virtual SeisPSReader* makeReader(const char*) const	= 0;
    virtual SeisPSWriter* makeWriter(const char*) const	= 0;

    const char*		type() const			{ return type_.buf(); }


protected:

    			SeisPSIOProvider( const char* t )
			    : type_(t)			{}

    BufferString	type_;

};


class SeisPSIOProviderFactory
{
public:

    int				add( SeisPSIOProvider* prov )
				{ return (provs_ += prov).size(); }
    const ObjectSet<SeisPSIOProvider>&	providers() const
				{ return provs_; }

    // Convenience functions
    const SeisPSIOProvider*	provider(const char* typ) const;
    SeisPSReader*		getReader(const IOObj&) const;
    SeisPSWriter*		getWriter(const IOObj&) const;

protected:

    ObjectSet<SeisPSIOProvider>	provs_;

};

SeisPSIOProviderFactory& SPSIOPF();


//------
//! Translator mechanism is only used for selection etc.

class SeisPSTranslatorGroup : public TranslatorGroup
{				isTranslatorGroup(SeisTrc)
public:
    			mDefEmptyTranslatorGroupConstructor(SeisPS)
};


class SeisPSTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(SeisPS)
};


class ODSeisPSTranslator : public SeisPSTranslator
{			 isTranslator(OD,SeisPS)
public:
    			mDefEmptyTranslatorConstructor(OD,SeisPS)

    bool		implRemove(const IOObj*) const;
};


#endif
