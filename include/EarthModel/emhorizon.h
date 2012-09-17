#ifndef emhorizon_h
#define emhorizon_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2007
 RCS:		$Id: emhorizon.h,v 1.7 2012/04/04 10:22:15 cvsbert Exp $
________________________________________________________________________


-*/

#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "keystrs.h"
#include "iopar.h"


namespace EM
{
class EMManager;

mClass HorizonGeometry : public RowColSurfaceGeometry
{
protected:
    				HorizonGeometry( Surface& surf )
				    : RowColSurfaceGeometry(surf)	{}
};


mClass Horizon : public Surface
{
public:
    virtual HorizonGeometry&		geometry()			= 0;
    virtual const HorizonGeometry&	geometry() const
					{ return const_cast<Horizon*>(this)
					    			->geometry(); }

    virtual float		getZValue(const Coord&,bool allow_udf=true,
	    				  int nr=0) const = 0;

    void			setStratLevelID( int lvlid )
				{ stratlevelid_ = lvlid; }
    int				stratLevelID() const
				{ return stratlevelid_; }

    virtual void		fillPar( IOPar& par ) const
				{
				    Surface::fillPar( par );
				    par.set( sKey::StratRef, stratlevelid_ );
				}

    virtual bool		usePar( const IOPar& par )
				{
				    par.get( sKey::StratRef, stratlevelid_ );
				    return Surface::usePar( par );
				}

protected:
    				Horizon( EMManager& emm )
				    : Surface(emm), stratlevelid_(-1)	{}

    virtual const IOObjContext&	getIOObjContext() const		= 0;

    int				stratlevelid_;
};

} // namespace EM

#endif
