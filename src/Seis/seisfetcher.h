/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/

#include "seisprovider.h"
#include "datapack.h"
class Seis2DDataSet;


namespace Seis
{

/*\brief Gets required traces from either DataPack or Storage

  The fetcher should transparently access the DataPack or the storage.
  There is no glue-ing going on in the Z direction. If the DataPack cannot
  satisfy the Z range requirements, then it will not be used.

  */

class Fetcher
{ mODTextTranslationClass(Seis::Fetcher);
public:

			Fetcher( Provider& p ) : prov_(p)	{}
    virtual		~Fetcher()				{}

    Provider&		prov_;
    mutable uiRetVal	uirv_;
    RefMan<DataPack>	dp_;

    void		getDataPack();
    bool		haveDP() const				{ return dp_; }
    virtual void	reset();

};


/*\brief Fetcher for 3D data. */

class Fetcher3D : public Fetcher
{
public:

			Fetcher3D( Provider& p ) : Fetcher(p)	{}

    Provider3D&		prov3D();
    const Provider3D&	prov3D() const;
    BinID		curBid() const;

    bool		isSelectedBinID(const BinID&) const;

};


/*\brief Fetcher for 2D data. */

class Fetcher2D : public Fetcher
{ mODTextTranslationClass(Seis::Fetcher2D);
public:

    mUseType( Pos,		GeomID );
    mUseType( Provider2D,	trcnr_type );
    mUseType( Provider2D,	Line2DData );
    mUseType( Provider2D,	Line2DDataSet );

			Fetcher2D( Provider& p ) : Fetcher(p)	{}
			~Fetcher2D();

    Provider2D&		prov2D();
    const Provider2D&	prov2D() const;

    bool		isSelectedPosition(GeomID,trcnr_type) const;
    void		getComponentInfo(BufferStringSet&,DataType&) const;

    void		ensureDataSet() const;
    void		getLineData(Line2DDataSet&) const;
    bool		toNextTrace();
    bool		toNextLine();

    Seis2DDataSet*	dataset_	= nullptr;

    bool		selectPosition(GeomID,trcnr_type);

    void		reset() override;

};


#define mIsSingleLine(sd) (sd && sd->nrGeomIDs()<2)



} // namespace Seis
