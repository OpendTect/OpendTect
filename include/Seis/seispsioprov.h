#ifndef seispsioprov_h
#define seispsioprov_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id: seispsioprov.h,v 1.2 2005-01-07 16:35:51 bert Exp $
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

  The key provided to makeReader and makeReader is the key to the data store.
  It can be a file or directory, but also some kind of data store access code.
  This name is stored in IOObjs refering to PS data stores. The IOObj's
  tranlator() is used for the type.

  If you pass an inline number to the makeReader, you will get a Reader that
  may not be able to provide the full geometry of the data store. If the
  inline is present in the data store, at least the segments for that inline
  should be filled.  Construction time can be much faster when you pass an
  inline number.
  Pass:
  * negative number for no scanning
  * positive number for single inline usage
  * mUndefIntVal for scanning the entire datastore

 */

class SeisPSIOProvider
{
public:

    virtual		~SeisPSIOProvider()		{}

    virtual SeisPSReader* makeReader(const char*,
	    			     int inl=mUndefIntVal) const = 0;
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
    SeisPSReader*		getReader(const IOObj&,
	    				  int inl=mUndefIntVal) const;
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
    Translator*		make(const char*,bool un=true) const;
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


inline Translator* SeisPSTranslatorGroup::make( const char*, bool ) const
{
    return new ODSeisPSTranslator("OD","OD");
}


#endif
