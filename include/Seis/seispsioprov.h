#ifndef seispsioprov_h
#define seispsioprov_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id: seispsioprov.h,v 1.11 2008-01-21 17:56:13 cvsbert Exp $
________________________________________________________________________

-*/

#include "transl.h"
#include "bufstring.h"
#include "seispsread.h"
#include "seispswrite.h"
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

  For 2D pre-stack data stores, you have to pass a line name to get the
  relevant reader. This can return null if the line name is not found.

 */

class SeisPSIOProvider
{
public:

    virtual			~SeisPSIOProvider()	{}

    virtual SeisPS3DReader*	make3DReader(const char*,int i=mUdf(int)) const
				{ return 0; }
    virtual SeisPS2DReader*	make2DReader(const char*,const char* lnm) const
				{ return 0; }
    virtual SeisPSWriter*	make3DWriter(const char*) const
				{ return 0; }
    virtual SeisPSWriter*	make2DWriter(const char*,const char* lnm) const
				{ return 0; }

    const char*			type() const		{ return type_.buf(); }

    static const char*		sKeyCubeID;


protected:

				SeisPSIOProvider( const char* t )
				    : type_(t)			{}

    BufferString		type_;

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
    SeisPS3DReader*		get3DReader(const IOObj&,int i=mUdf(int)) const;
    SeisPS2DReader*		get2DReader(const IOObj&,const char* lnm) const;
    SeisPSWriter*		get3DWriter(const IOObj&) const;
    SeisPSWriter*		get2DWriter(const IOObj&,const char* lnm) const;

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
