/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/

#include "seisprovider.h"
#include "seistrc.h"
#include "datapack.h"
class GatherSetDataPack;
class RegularSeisDataPack;
class Seis2DDataSet;
namespace Pos { class ZSubSel; }


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
    mUseType( Pos,		GeomID );
    mUseType( Pos,		ZSubSel );

				Fetcher( Provider& p )
				    : prov_(p)		{}
    virtual			~Fetcher()		{}
    virtual bool		is2D() const		= 0;
    virtual bool		isPS() const		= 0;

    virtual void		reset();
    virtual void		getComponentInfo(BufferStringSet&,
				     DataType&) const	= 0;
    virtual void		getPossibleExtents()	= 0;
    virtual void		prepWork()		= 0;
    virtual const STTrl*	curTransl() const	{ return nullptr; }

    bool			haveDP() const		{ return dp_; }
    RegularSeisDataPack&	regSeisDP();
    const RegularSeisDataPack&	regSeisDP() const;
    GatherSetDataPack&		gathDP();
    const GatherSetDataPack&	gathDP() const;

    DataCharacteristics		dataChar() const	{ return datachar_; }

    Provider&		prov_;
    mutable uiRetVal	uirv_;

protected:

    void		fillFromDP(const BinID&,SeisTrcInfo&,TraceData&);
    void		fillFromDP(const Bin2D&,SeisTrcInfo&,TraceData&);

    void		handleGeomIDChange(idx_type iln=0);
    ZSampling		provZSamp() const;

    idx_type		curlidx_			= -1;
    RefMan<DataPack>	dp_;
    DataCharacteristics	datachar_;

private:

    SeisTrc		worktrc_;
    void		fillFromDP(const TrcKey&,SeisTrcInfo&,TraceData&);

};


/*\brief Fetcher for 3D data. */

class Fetcher3D : public Fetcher
{
public:

			Fetcher3D( Provider& p ) : Fetcher(p)	{}
    bool		is2D() const override		{ return false; }

    Provider3D&		prov3D()
			{ return static_cast<Provider3D&>( prov_ ); }
    const Provider3D&	prov3D() const
			{ return static_cast<const Provider3D&>( prov_ ); }

    virtual bool	setPosition(const BinID&)		= 0;

protected:

    bool		useDP(const BinID&) const;

};


/*\brief Fetcher for 2D data. */

class Fetcher2D : public Fetcher
{ mODTextTranslationClass(Seis::Fetcher2D);
public:

			Fetcher2D( Provider& p ) : Fetcher(p)	{}
			~Fetcher2D();
    bool		is2D() const override	{ return true; }
    void		reset() override;

    Provider2D&		prov2D()
			{ return static_cast<Provider2D&>( prov_ ); }
    const Provider2D&	prov2D() const
			{ return static_cast<const Provider2D&>( prov_ ); }

    void		getComponentInfo(BufferStringSet&,
					 DataType&) const override;
    void		getPossibleExtents() override;

    virtual bool	setPosition(const Bin2D&)		= 0;

    Bin2D		curb2d_;
    Seis2DDataSet*	dataset_	= nullptr;

protected:

    void		ensureDataSet() const;
    bool		useDP(const Bin2D&) const;

};


} // namespace Seis
