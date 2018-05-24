#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2018
________________________________________________________________________


-*/

#include "wellreadaccess.h"
#include "uistring.h"
class IOObj;
namespace HDF5 { class Reader; }


namespace Well
{

class HDF5Writer;

/*!\brief Reads Well::Data from HDF5 file  */

mExpClass(Well) HDF5Reader : public Well::ReadAccess
{ mODTextTranslationClass(Well::HDF5Reader)
public:

			HDF5Reader(const IOObj&,Data&,uiString& errmsg);
			HDF5Reader(const char* fnm,Data&,uiString& errmsg);
			HDF5Reader(const HDF5Writer&,Data&,uiString& errmsg);
			~HDF5Reader();

    virtual bool	getInfo() const;
    virtual bool	getTrack() const;
    virtual bool	getLogs() const;
    virtual bool	getMarkers() const;
    virtual bool	getD2T() const;
    virtual bool	getCSMdl() const;
    virtual bool	getDispProps() const;
    virtual bool	getLog(const char* lognm) const;
    virtual void	getLogNames(BufferStringSet&) const;
    virtual void	getLogInfo(ObjectSet<IOPar>&) const;

    virtual const uiString& errMsg() const	{ return errmsg_; }

protected:

    uiString&		errmsg_;
    HDF5::Reader*	rdr_;

    void		init(const char*);

};

}; // namespace Well
