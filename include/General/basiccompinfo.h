#ifndef basiccompinfo_h
#define basiccompinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Component information
 RCS:		$Id: basiccompinfo.h,v 1.1 2001-03-19 14:30:37 bert Exp $
________________________________________________________________________

-*/

#include <uidobj.h>
#include <datachar.h>
#include <samplingdata.h>


/*!\brief Info on one component */

class BasicComponentInfo : public UserIDObject
{
public:
			BasicComponentInfo( const char* nm=0 )
			: UserIDObject(nm)
			, datatype(0), sd(0,1), nrsamples(0)	{}

    bool		operator==( const BasicComponentInfo& ci ) const
			{ return datatype == ci.datatype
			      && datachar == ci.datachar
			      && sd == ci.sd
			      && nrsamples == ci.nrsamples; }

    int			datatype;
    DataCharacteristics	datachar;
    SamplingData<float>	sd;
    int			nrsamples;

};


#endif
