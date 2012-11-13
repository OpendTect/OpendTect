#ifndef seispsioprov_h
#define seispsioprov_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"
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

mClass(Seis) SeisPSIOProvider
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

    FixedString			type() const		{ return type_.buf(); }
    virtual bool		getLineNames(const char*,BufferStringSet&) const
    				{ return false; }

    static const char*		sKeyCubeID;


protected:

				SeisPSIOProvider( const char* t )
				    : type_(t)			{}

    BufferString		type_;

};


mClass(Seis) SeisPSIOProviderFactory
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

    bool			getLineNames(const IOObj&,
	    				     BufferStringSet&) const; // For 2D
    void			mk3DPostStackProxy(IOObj&);
    				//!< Adds entry to omf for post-stack access

protected:

    ObjectSet<SeisPSIOProvider>	provs_;

};

mGlobal(Seis) SeisPSIOProviderFactory& SPSIOPF();


//------
//! Translator mechanism is only used for selection etc.

mClass(Seis) SeisPS3DTranslatorGroup : public TranslatorGroup
{				isTranslatorGroup(SeisPS3D)
public:
    			mDefEmptyTranslatorGroupConstructor(SeisPS3D)
};


mClass(Seis) SeisPS3DTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(SeisPS3D)
};


mClass(Seis) CBVSSeisPS3DTranslator : public SeisPS3DTranslator
{			       isTranslator(CBVS,SeisPS3D)
public:
    			mDefEmptyTranslatorConstructor(CBVS,SeisPS3D)

    bool		implRemove(const IOObj*) const;
};


mClass(Seis) SeisPS2DTranslatorGroup : public TranslatorGroup
{				isTranslatorGroup(SeisPS2D)
public:
    			mDefEmptyTranslatorGroupConstructor(SeisPS2D)
};


mClass(Seis) SeisPS2DTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(SeisPS2D)
};


mClass(Seis) CBVSSeisPS2DTranslator : public SeisPS2DTranslator
{			       isTranslator(CBVS,SeisPS2D)
public:
    			mDefEmptyTranslatorConstructor(CBVS,SeisPS2D)

    bool		implRemove(const IOObj*) const;
};


#endif

