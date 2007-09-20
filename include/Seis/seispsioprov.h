#ifndef seispsioprov_h
#define seispsioprov_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id: seispsioprov.h,v 1.7 2007-09-20 13:56:26 cvskris Exp $
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

  The key provided to makeReader and makeWriter is the key to the data store.
  It can be a file or directory, but also some kind of data store access code.
  This name is stored in IOObjs refering to PS data stores. The IOObj's
  tranlator() is used for the type. OpendTect's simple CBVS PS data store
  has type 'CBVS' (who'd have thought that!), and the data store key is a
  directory name.

  If you pass an inline number to the makeReader, you will get a Reader that
  should be able to read that inline, but be aware that this reader may not
  be able to provide the full geometry of the entire data store. If the
  inline is present in the data store, at least the segments for that inline
  should be filled (i.e. the geometry for that inline will be filled correctly).
  Construction time can be much faster when you pass an inline number.
  Pass:
  * negative number for no scanning
  * positive number for single inline usage
  * mUdf(int) (=default) for scanning the entire datastore

 */

class SeisPSIOProvider
{
public:

    virtual		~SeisPSIOProvider()		{}

    virtual SeisPSReader* makeReader(const char*,int inl=mUdf(int)) const = 0;
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
    SeisPSReader*		getReader(const IOObj&,int inl=mUdf(int)) const;
    SeisPSWriter*		getWriter(const IOObj&) const;

protected:

    ObjectSet<SeisPSIOProvider>	provs_;

};

SeisPSIOProviderFactory& SPSIOPF();


//------
//! Translator mechanism is only used for selection etc.

class SeisPSTranslatorGroup : public TranslatorGroup
{				isTranslatorGroup(SeisPS)
public:
    			mDefEmptyTranslatorGroupConstructor(SeisPS)
};


class SeisPSTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(SeisPS)
};


class CBVSSeisPSTranslator : public SeisPSTranslator
{			 isTranslator(CBVS,SeisPS)
public:
    			mDefEmptyTranslatorConstructor(CBVS,SeisPS)

    bool		implRemove(const IOObj*) const;
};


#endif
