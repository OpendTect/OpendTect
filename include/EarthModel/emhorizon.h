#ifndef emhorizon_h
#define emhorizon_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		May 2007
 RCS:		$Id: emhorizon.h,v 1.1 2007-05-22 03:23:22 cvsnanne Exp $
________________________________________________________________________


-*/

#include "emsurface.h"
#include "emsurfacegeometry.h"


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

protected:
    				Horizon( EMManager& emm )
				    : Surface(emm)	{}

    virtual const IOObjContext&	getIOObjContext() const		= 0;
};

} // namespace EM

#endif
