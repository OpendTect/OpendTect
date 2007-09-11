#ifndef emhorizon_h
#define emhorizon_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		May 2007
 RCS:		$Id: emhorizon.h,v 1.3 2007-09-11 16:39:28 cvsnanne Exp $
________________________________________________________________________


-*/

#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "keystrs.h"
#include "iopar.h"


namespace EM
{
class EMManager;

class HorizonGeometry : public RowColSurfaceGeometry
{
protected:
    				HorizonGeometry( Surface& surf )
				    : RowColSurfaceGeometry(surf)	{}
};


class Horizon : public Surface
{
public:
    virtual HorizonGeometry&		geometry()			= 0;
    virtual const HorizonGeometry&	geometry() const
					{ return const_cast<Horizon*>(this)
					    			->geometry(); }
								
    void			setTiedToLvl( const char* str )
    				{ stratlevelnm_ = str; }

    virtual void		fillPar( IOPar& par ) const
				{
				    Surface::fillPar( par );
				    par.set( sKey::StratRef, stratlevelnm_ );
				}

    virtual bool		usePar( const IOPar& par )
				{
				    par.get( sKey::StratRef, stratlevelnm_ );
				    return Surface::usePar( par );
				}

protected:
    				Horizon( EMManager& emm )
				    : Surface(emm)	{}

    virtual const IOObjContext&	getIOObjContext() const		= 0;

    BufferString		stratlevelnm_;

};

} // namespace EM

#endif
