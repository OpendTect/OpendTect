#ifndef segyfiledata_h
#define segyfiledata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2008
 RCS:		$Id: segyfiledata.h,v 1.1 2008-10-04 10:04:04 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "position.h"
#include "seistype.h"
#include "samplingdata.h"
class IOPar;
class DataPointSet;
 

namespace SEGY
{

/*\brief Data usually obtained by scanning a SEG-Y file.

  The data is stored in a DataPointSet, and we have to take measures against
  the SI() not being well defined. Thus, we add 2 extra columns for residual
  X and Y offset.
 
 */

class FileData
{
public:
    			FileData(const char* fnm,Seis::GeomType);
    			FileData(const FileData&);
			~FileData();

    BufferString	fname_;
    Seis::GeomType	geom_;
    int			trcsz_;
    SamplingData<float>	sampling_;
    int			segyfmt_;
    bool		isrev1_;
    int			nrstanzas_;

    int			nrTraces() const;
    BinID		binID(int) const;
    Coord		coord(int) const;
    int			trcNr(int) const;
    float		offset(int) const;
    bool		isNull(int) const;
    bool		isUsable(int) const;

    void		add(const BinID&,const Coord&,int,float,bool,bool);
    			//!< params are as in access functions above
    void		addEnded(); //!< causes dataChange() for DataPointSet

    void		getReport(IOPar&) const;

protected:

    DataPointSet&	data_;
};

} // namespace


#endif
