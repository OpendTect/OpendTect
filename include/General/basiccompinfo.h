#ifndef basiccompinfo_h
#define basiccompinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Component information
 RCS:		$Id: basiccompinfo.h,v 1.5 2003-11-07 12:21:51 bert Exp $
________________________________________________________________________

-*/

#include <uidobj.h>
#include <datachar.h>
#include <samplingdata.h>
#include <scaler.h>


/*!\brief Info on one component */

class BasicComponentInfo : public UserIDObject
{
public:
			BasicComponentInfo( const char* nm=0 )
			: UserIDObject(nm)
			, datatype(0), sd(0,1), nrsamples(0)
			, scaler(0)			{}
			BasicComponentInfo( const BasicComponentInfo& ci )
			: UserIDObject((const char*)ci.name())
			, scaler(0)		{ *this = ci; }
    BasicComponentInfo&	operator=( const BasicComponentInfo& ci )
			{
			    if ( this == &ci ) return *this;
			    setName( ci.name() );
			    datatype = ci.datatype;
			    datachar = ci.datachar;
			    sd = ci.sd; nrsamples= ci.nrsamples;
			    delete scaler;
			    scaler = ci.scaler ? ci.scaler->mDuplicate() : 0;
			    return *this;
			}

    bool		operator==( const BasicComponentInfo& ci ) const
			{
			    if ( (bool)scaler != (bool)ci.scaler )
				return false;

			    return name() == ci.name()
				&& datatype == ci.datatype
				&& datachar == ci.datachar
				&& sd == ci.sd
				&& nrsamples == ci.nrsamples
				&& ( (!scaler && !ci.scaler)
				  || (*scaler == *ci.scaler) );
			}

    int			datatype;
    DataCharacteristics	datachar;
    SamplingData<float>	sd;
    int			nrsamples;
    LinScaler*		scaler;

};


#endif
