#ifndef welld2tmodel_h
#define welld2tmodel_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: welld2tmodel.h,v 1.3 2003-08-18 16:37:23 bert Exp $
________________________________________________________________________


-*/

#include "uidobj.h"
#include "color.h"

namespace Well
{

class D2TModel : public ::UserIDObject
{
public:

			D2TModel( const char* nm= 0 )
			: ::UserIDObject(nm)	{}

    float		getTime(float d_ah) const;

    BufferString	desc;
    BufferString	datasource;

    static const char*	sKeyTimeWell; //!< name of model for well that is only
    				      //!< known in time
    static const char*	sKeyDataSrc;

    TypeSet<float>	t;
    TypeSet<float>	dah;

};


}; // namespace Well

#endif
