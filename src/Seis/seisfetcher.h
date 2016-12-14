/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/

#include "seistrc.h"
#include "trckeyzsampling.h"
class IOObj;
class Seis2DDataSet;


namespace Seis
{

class Provider;
class Provider2D;
class Provider3D;

/*\brief Gets required traces from either DataPack or Storage

  The fetcher should transparently access the DataPack or the storage.
  There is no glue-ing going on in the Z direction. If the DataPack cannot
  satisfy the Z range requirements, then it will not be used.

  */

class Fetcher
{ mODTextTranslationClass(Seis::Fetcher);
public:

			Fetcher(Provider&);
    virtual		~Fetcher();

    virtual void	reset();
    bool		fillIOObj();
    IOObj*		getIOObj() const;

    Provider&		prov_;
    IOObj*		ioobj_;
    uiRetVal		uirv_;

};


/*\brief Fetcher for 3D data. */

class Fetcher3D : public Fetcher
{ mODTextTranslationClass(Seis::Fetcher3D);
public:

			Fetcher3D( Provider& p ) : Fetcher(p)	{}

    void		getReqCS();

    Provider3D&		prov3D();
    const Provider3D&	prov3D() const;

    bool		isSelectedBinID(const BinID&) const;
    bool		moveNextBinID();

    TrcKeyZSampling	reqcs_;
    BinID		nextbid_;

    virtual void		reset();
    virtual TrcKeyZSampling	getDefaultCS() const;

};


/*\brief Fetcher for 2D data. */

class Fetcher2D : public Fetcher
{ mODTextTranslationClass(Seis::Fetcher2D);
public:

			Fetcher2D( Provider& p )
			    : Fetcher(p)
			    , dataset_(0)	    {}
			~Fetcher2D();

    Provider2D&		prov2D();
    const Provider2D&	prov2D() const;
    void		openDataSet();

    Seis2DDataSet*	dataset_;
    TrcKey		nexttrcky_;

    virtual void	reset();

};



} // namespace Seis
