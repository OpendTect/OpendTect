#ifndef stratlayermodel_h
#define stratlayermodel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Sep 2010
 RCS:		$Id: stratlayermodel.h,v 1.1 2010-09-06 13:57:50 cvsbert Exp $
________________________________________________________________________


-*/

#include "stratlayer.h"
#include "manobjectset.h"

namespace Strat
{
class Layer;

/*!\brief A model consisting of layers */

mClass LayerModel
{
public:

    				LayerModel()
				    : layers_(false)	{}

    ObjectSet<Layer>&		layers()		{ return layers_; }
    const ObjectSet<Layer>&	layers() const		{ return layers_; }

protected:

    ManagedObjectSet<Layer>	layers_;

};


}; // namespace Strat

#endif
