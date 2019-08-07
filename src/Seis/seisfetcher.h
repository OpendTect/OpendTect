/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/

#include "seisprovider.h"
#include "datapack.h"
class GatherSetDataPack;
class RegularSeisDataPack;
class Seis2DDataSet;


namespace Seis
{

/*\brief Gets required traces from either DataPack or Storage

  The fetcher should transparently access the DataPack or the storage.
  There is no glue-ing going on in the Z direction. If the DataPack cannot
  satisfy the Z range requirements, then it will not be used.

  */

class Fetcher
{
public:

    mUseType( Provider,		idx_type );
    mUseType( Provider,		STTrl );

				Fetcher( Provider& p )
				    : prov_(p)		{}
    virtual			~Fetcher()		{}
    virtual bool		is2D() const		= 0;
    virtual bool		isPS() const		= 0;

    virtual void		reset();
    virtual void		getComponentInfo(BufferStringSet&,
				     DataType&) const	= 0;
    virtual void		getPossiblePositions()	= 0;
    virtual void		prepWork()		= 0;
    virtual const STTrl*	curTransl() const	{ return nullptr; }

    void			ensureDPIfAvailable(idx_type lidx);
    bool			haveDP() const		{ return dp_; }
    RegularSeisDataPack&	regSeisDP();
    const RegularSeisDataPack&	regSeisDP() const;
    GatherSetDataPack&		gathDP();
    const GatherSetDataPack&	gathDP() const;

    Provider&		prov_;
    mutable uiRetVal	uirv_;
    RefMan<DataPack>	dp_;
    idx_type		dplidx_			= -1;

};


/*\brief Fetcher for 3D data. */

class Fetcher3D : public Fetcher
{
public:

			Fetcher3D( Provider& p ) : Fetcher(p)	{}
    virtual bool	is2D() const		{ return false; }

    Provider3D&		prov3D()
			{ return static_cast<Provider3D&>( prov_ ); }
    const Provider3D&	prov3D() const
			{ return static_cast<const Provider3D&>( prov_ ); }

    virtual bool	setPosition(const BinID&)		= 0;

};


/*\brief Fetcher for 2D data. */

class Fetcher2D : public Fetcher
{ mODTextTranslationClass(Seis::Fetcher2D);
public:

			Fetcher2D( Provider& p ) : Fetcher(p)	{}
			~Fetcher2D();
    virtual bool	is2D() const		{ return true; }
    void		reset() override;

    Provider2D&		prov2D()
			{ return static_cast<Provider2D&>( prov_ ); }
    const Provider2D&	prov2D() const
			{ return static_cast<const Provider2D&>( prov_ ); }

    void		getComponentInfo(BufferStringSet&,
					 DataType&) const override;

    virtual bool	setPosition(const Bin2D&)		= 0;

    Bin2D		curb2d_;
    Seis2DDataSet*	dataset_	= nullptr;

protected:

    void		ensureDataSet() const;

};


} // namespace Seis
