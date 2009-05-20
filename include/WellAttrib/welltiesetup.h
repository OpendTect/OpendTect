#ifndef welltiesetup_h
#define welltiesetup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bruno
 Date:          Jan 2009
 RCS:           $Id: welltiesetup.h,v 1.4 2009-05-20 14:27:29 cvsbruno Exp $
________________________________________________________________________

-*/

#include "namedobj.h"

#include "attribdescid.h"
#include "multiid.h"

#include "wellio.h"
#include <iosfwd>

class IOPar;
class MultiID;


mClass WellTieSetup
{
public:
			WellTieSetup()
			    : wellid_(*new MultiID())
			    , attrid_(*new Attrib::DescID())
			    , wvltid_(*new MultiID())
			    , issonic_(true)
			    , corrvellognm_("CS Corrected ")
			    {}


			WellTieSetup( const WellTieSetup& setup ) 
			    : wellid_(setup.wellid_)
			    , attrid_(setup.attrid_)
			    , wvltid_(setup.wvltid_)
			    , issonic_(setup.issonic_)
			    , vellognm_(setup.vellognm_)
			    , denlognm_(setup.denlognm_)
			    , corrvellognm_(setup.corrvellognm_)
			    {}	
	
    MultiID			wellid_;
    Attrib::DescID         	attrid_;
    BufferString        	vellognm_;
    BufferString          	denlognm_;
    BufferString          	corrvellognm_;
    MultiID               	wvltid_;
    bool                	issonic_;
    
    void    	      		usePar(const IOPar&);
    void          	 	fillPar(IOPar&) const;

    static WellTieSetup&	defaults();
    static void                 commitDefaults();

protected:
    
};


mClass WellTieIO : public Well::IO
{
public:
    				WellTieIO(const char* f,bool isrd)
				: Well::IO(f,isrd)
				{}

    static const char*  	sKeyWellTieSetup();
    static const char*  	sExtWellTieSetup();
};



mClass WellTieWriter : public WellTieIO
{
public:
				WellTieWriter(const char* f,const WellTieSetup&);

    bool          	        putWellTieSetup() const;  
    bool 			putWellTieSetup(std::ostream&) const;

protected:
   
    const WellTieSetup& 	wts_;
    bool                	wrHdr(std::ostream&,const char*) const;

};


mClass WellTieReader : public WellTieIO
{
public:
				WellTieReader(const char* f,WellTieSetup&);
  
    bool               		getWellTieSetup() const;	
    bool                	getWellTieSetup(std::istream&) const;


protected:
   
    WellTieSetup& 	wts_;
};

#endif
