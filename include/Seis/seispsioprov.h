#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Dec 2004
________________________________________________________________________

-*/

#include "seismod.h"

#include "bufstring.h"
#include "manobjectset.h"
#include "posgeomid.h"
#include "seispsread.h"
#include "seispswrite.h"
#include "transl.h"
#include "uistring.h"

class IOObj;


/*!\brief Prestack Seismics objects provider

  It is not mandatory to provide both reader and writer.
  Null returns must be expected.

  The class has 2 sets of interface functions, both can be implemented. The
  OpendTect application will always use the first interface, which is the
  IOObj-based interface with the getXXX and fetchXXX functions. The second
  is if you do not want to make an IOObj but do know the IOObj.fullUserExpr.
  That can be used as the key for the makeXXX and (2nd bunch of )
  getXXX functions.

  The idea is to override the public virtual functions. Alternatively,
  if you know ioobj.fullUserExpr(), then you can override the makeReader and
  makeWriter functions. This was the only thing available in od5.0 and earlier.
  That key can be a file or directory, but also some kind of data store access
  code.  The IOObj's tranlator() is used for the type. OpendTect's simple
  CBVS PS data store has type 'CBVS' (who'd have thought that!), and the data
  store key is a directory name.

  If you pass an inline number to the getReader, you will get a Reader that
  should be able to read that inline, but be aware that this reader may not
  be able to provide the full geometry of the entire data store. If the
  inline is present in the data store, at least the segments for that inline
  should be filled (i.e. the geometry for that inline will be filled correctly).
  Construction time can be much faster when you pass an inline number.
  Pass:
  * negative number for no scanning
  * positive number for single inline usage
  * mUdf(int) (=default) for scanning the entire datastore

  For 2D prestack data stores, you have to pass a line name or GeomID to get
  the relevant reader. This can return null if the line id or name is not found.

 */

mExpClass(Seis) SeisPSIOProvider
{ mODTextTranslationClass(SeisPSIOProvider);
public:

    virtual			~SeisPSIOProvider()	{}

    virtual bool		canHandle( bool forread, bool for2d ) const
				{ return false; }

				// IOObj-based interface. Implementation
				// defaults to the string-based using
				// IOObj's fullUsrExpr.
    virtual SeisPS3DReader*	get3DReader(const IOObj&,
					     int i=mUdf(int)) const;
    virtual SeisPS2DReader*	get2DReader(const IOObj&,Pos::GeomID) const;
    virtual SeisPS2DReader*	get2DReader(const IOObj&,const char*) const;

    virtual SeisPSWriter*	get3DWriter(const IOObj&) const;
    virtual SeisPSWriter*	get2DWriter(const IOObj&,Pos::GeomID) const;
    virtual SeisPSWriter*	get2DWriter(const IOObj&,const char*) const;

    StringView			type() const		{ return type_.buf(); }
    virtual bool		fetchGeomIDs(const IOObj&,
					     TypeSet<Pos::GeomID>&) const;
    virtual bool		fetchLineNames(const IOObj&,
						BufferStringSet&) const;

				// string-based interface, The key passed must
				// be what IOObj::fullUsrExpr would return.
    virtual SeisPS3DReader*	make3DReader(const char*,int i=mUdf(int)) const
				{ return 0; }
    virtual SeisPS2DReader*	make2DReader(const char*,Pos::GeomID) const
				{ return 0; }
    virtual SeisPS2DReader*	make2DReader(const char*,const char* lnm) const
				{ return 0; }

    virtual SeisPSWriter*	make3DWriter(const char*) const
				{ return 0; }
    virtual SeisPSWriter*	make2DWriter(const char*,Pos::GeomID) const
				{ return 0; }
    virtual SeisPSWriter*	make2DWriter(const char*,const char* lnm) const
				{ return 0; }

    virtual bool		getGeomIDs(const char*,
					   TypeSet<Pos::GeomID>&) const
				{ return false; }
    virtual bool		getLineNames(const char*,
					     BufferStringSet&) const
				{ return false; }

    static const char*		sKeyCubeID;


protected:

				SeisPSIOProvider( const char* t )
				    : type_(t)			{}

    BufferString		type_;

};


mExpClass(Seis) SeisPSIOProviderFactory
{ mODTextTranslationClass(SeisPSIOProviderFactory);
public:

    int				add( SeisPSIOProvider* prov )
				{ return (provs_ += prov).size(); }
    const ObjectSet<SeisPSIOProvider>&	providers() const
				{ return provs_; }

    // Convenience functions
    const SeisPSIOProvider*	provider(const char* typ) const;
    SeisPSReader*		getReader(const MultiID&,const TrcKey&) const;
    SeisPSReader*		getReader(const IOObj&,const TrcKey&) const;
    SeisPSWriter*		getWriter(const MultiID&,const TrcKey&) const;
    SeisPSWriter*		getWriter(const IOObj&,const TrcKey&) const;

				//! For 3D
    SeisPS3DReader*		get3DReader(const IOObj&,int i=mUdf(int)) const;
    SeisPSWriter*		get3DWriter(const IOObj&) const;
    void			mk3DPostStackProxy(IOObj&);
				//!< Adds entry to omf for post-stack access

				//! For 2D
    SeisPS2DReader*		get2DReader(const IOObj&,Pos::GeomID) const;
    SeisPS2DReader*		get2DReader(const IOObj&,const char* lnm) const;
    SeisPSWriter*		get2DWriter(const IOObj&,Pos::GeomID) const;
    SeisPSWriter*		get2DWriter(const IOObj&,const char* lnm) const;
    bool			getGeomIDs(const IOObj&,
					   TypeSet<Pos::GeomID>&) const;
    bool			getLineNames(const IOObj&,
					     BufferStringSet&) const;

protected:

    ManagedObjectSet<SeisPSIOProvider>	provs_;

};

mGlobal(Seis) SeisPSIOProviderFactory& SPSIOPF();


//------
//! Translator mechanism is only used for selection etc.

mExpClass(Seis) SeisPS3DTranslatorGroup : public TranslatorGroup
{				isTranslatorGroup(SeisPS3D)
public:
			mDefEmptyTranslatorGroupConstructor(SeisPS3D)
};


mExpClass(Seis) SeisPS3DTranslator : public Translator
{ mODTextTranslationClass(SeisPS3DTranslator);
public:
			mDefEmptyTranslatorBaseConstructor(SeisPS3D)

    bool		implRemove(const IOObj*) const override;
    bool		implRename(const IOObj*,const char*,
				   const CallBack* cb=0) const override;
};


mExpClass(Seis) CBVSSeisPS3DTranslator : public SeisPS3DTranslator
{			       isTranslator(CBVS,SeisPS3D)
public:
			mDefEmptyTranslatorConstructor(CBVS,SeisPS3D)

    bool		implRemove(const IOObj*) const override;
};


mExpClass(Seis) SeisPS2DTranslatorGroup : public TranslatorGroup
{				isTranslatorGroup(SeisPS2D)
public:
			mDefEmptyTranslatorGroupConstructor(SeisPS2D)
};


mExpClass(Seis) SeisPS2DTranslator : public Translator
{ mODTextTranslationClass(SeisPS2DTranslator);
public:
			mDefEmptyTranslatorBaseConstructor(SeisPS2D)
};


mExpClass(Seis) CBVSSeisPS2DTranslator : public SeisPS2DTranslator
{			       isTranslator(CBVS,SeisPS2D)
public:
			mDefEmptyTranslatorConstructor(CBVS,SeisPS2D)

    bool		implRemove(const IOObj*) const override;
};


