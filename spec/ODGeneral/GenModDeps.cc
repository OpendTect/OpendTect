/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2002
 * FUNCTION : Generate file to include in make.Vars
-*/

static const char* rcsID = "$Id: GenModDeps.cc,v 1.1 2002-11-14 11:36:00 bert Exp $";

#include "prog.h"
#include "strmprov.h"
#include <stdlib.h>
#include <iostream.h>

class Dep
{
public:
    				Dep( const char* m )
				    : name(m)	{}
    bool			operator ==( const char* s ) const
				    { return name == s; }

    BufferString		name;
    ObjectSet<BufferString>	mods;
};


int main( int argc, char** argv )
{
    if ( argc != 3 )
    {
	cerr << "Usage: " << argv[0] << " input_spec_file out_makevars" << endl;
	return 1;
    }
    StreamProvider spin( argv[1] );
    StreamData sdin = spin.makeIStream();
    if ( !sdin.usable() )
    {
	cerr << argv[0] << ": Cannot open input stream" << endl;
	return 1;
    }
    else if ( sdin.istrm == &cin )
	cout << "Using standard input." << endl;
    istream& instrm = *sdin.istrm;

    if ( !instrm )
    {
	cerr << "Bad dep spec file" << endl;
	return 1;
    }
    StreamProvider spout( argv[2] );
    StreamData sdout = spout.makeOStream();
    if ( !sdout.usable() )
    {
	cerr << argv[0] << ": Cannot open output stream" << endl;
	return 1;
    }
    ostream& outstrm = *sdout.ostrm;

    char linebuf[1024];
    char wordbuf[256];
    ObjectSet<Dep> deps;
    while ( instrm )
    {
	instrm.getline( linebuf, 1024 );
	char* bufptr = linebuf; 
	skipLeadingBlanks(bufptr);
	removeTrailingBlanks(bufptr);
	if ( ! *bufptr || *bufptr == '#' )
	    continue;

	char* nextptr = (char*)getNextWord(bufptr,wordbuf);
	if ( ! wordbuf[0] ) continue;
	int l = strlen( wordbuf );
	if ( wordbuf[l-1] == ':' ) wordbuf[l-1] = '\0';
	if ( ! wordbuf[0] ) continue;

	*nextptr++ = '\0';
	skipLeadingBlanks(nextptr);

	Dep* newdep = new Dep( wordbuf ) ;
	ObjectSet<BufferString> filedeps;
	while ( nextptr && *nextptr )
	{
	    skipLeadingBlanks(nextptr);
	    nextptr = (char*)getNextWord(nextptr,wordbuf);
	    if ( !wordbuf[0] ) break;

	    if ( wordbuf[1] != '.' || (wordbuf[0] != 'S' && wordbuf[0] != 'D') )
		{ cerr << "Cannot handle dep=" << wordbuf << endl; return 1; }

	    filedeps += new BufferString( wordbuf );
	}


	ObjectSet<BufferString> depmods;
	for ( int idx=filedeps.size()-1; idx>=0; idx-- )
	{
	    const char* filedep = (const char*)(*filedeps[idx]);
	    const char* modnm = filedep + 2;
	    if ( *filedep == 'S' )
	    {
		depmods += new BufferString( modnm );
	        continue;
	    }

	    Dep* depdep = find( deps, modnm );
	    if ( !depdep )
		{ cerr << "Cannot find dep=" << modnm << endl; return 1; }

	    for ( int idep=depdep->mods.size()-1; idep>=0; idep-- )
	    {
		const char* depdepmod = (const char*)(*depdep->mods[idep]);
		if ( !find(depmods,depdepmod) )
		    depmods += new BufferString( depdepmod );
	    }
	}
	if ( depmods.size() < 1 )
	    { delete newdep; continue; }
	deps += newdep;

	for ( int idx=depmods.size()-1; idx>=0; idx-- )
	    newdep->mods += depmods[idx];
    }
    sdin.close();

    for ( int idx=0; idx<deps.size(); idx++ )
    {
	Dep& dep = *deps[idx];
	outstrm << 'L' << dep.name << " :=";
	for ( int idep=0; idep<dep.mods.size(); idep++ )
	    outstrm << " -l" << (const char*)(*dep.mods[idep]);
	outstrm << endl;
	outstrm << 'I' << dep.name << " :=";
	for ( int idep=0; idep<dep.mods.size(); idep++ )
	    outstrm << ' ' << (const char*)(*dep.mods[idep]);
	outstrm << endl;
    }

    sdout.close();
    return 0;
}
