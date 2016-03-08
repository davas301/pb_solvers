#include "setup.h"

Setup::Setup(string infile)
:ompThreads_( 1 ),
saltConc_( 0.01 ),
nType_( 2 ),
PBCs_( 0 ),
blen_( 1.4e8 ),
maxtime_( 1000000 ),
ntraj_( 5 ),
idiel_( 4.0 ),
sdiel_( 78.0 ),
temp_( 298.0 ),
srand_( (unsigned)time(NULL) ),
nTypenCount_(2),
typeDef_(2),
typeDiff_(2),
molfnames_(2),
runSpecs_(2)
{
  nTypenCount_[0] = 1;
  nTypenCount_[1] = 1;
  
  for (int i = 0; i<nType_; i++)
  {
    typeDiff_[i] = vector<double> (2);
    typeDiff_[i][0] = 0.0;
    typeDiff_[i][1] = 0.0;
  }
  
  typeDef_[0] = "stat";
  typeDef_[1] = "stat";
  runSpecs_[0] = "pot";
  runSpecs_[1] = "test";
  
  // Initializing file locs to defaults
  // pqr fname, imat path, spol path, spol name
  vector<vector<string> > molfn = {{"../Config/test1.pqr", "../Imat/test1/",
    "../Selfpol/test1", "test1_p30.0", "../Config/test1.xyz"},
    {"../Config/test2.pqr", "../Imat/test2/",
      "../Selfpol/test2", "test2_p30.0", "../Config/test2.xyz"}};
  
  for (int i=0; i<nType_; i++)
  {
    molfnames_[i] = vector<string> ( 5);
    for (int j=0; j<molfnames_[i].size();j++)
    {
      molfnames_[i][j] = molfn[i][j];
    }
  }
  
}


void Setup::read_infile(string fname)
{
  cout << "Reading Input file " << fname << endl ;
  ifstream fin(fname);
  if (!fin.is_open())
  {
    cout << "Could not open file " << fname << endl;
    exit(0);
  }
  
  string inputLine;
  vector<vector <string> > keywordLines;
  getline(fin,inputLine);
  
  while (!fin.eof())
  {
    findLines(inputLine);
    getline(fin, inputLine);
  }
}

vector<string> Setup::split(string str, char delim)
{
  vector<string> internal;
  stringstream ss(str); // Turn the string into a stream.
  string tok;
  
  while(getline(ss, tok, delim))
  {
    internal.push_back(tok);
  }
  return internal;
}


void Setup::findLines(string fline)
{
    if (!fline.empty())
    {
      findKeyword( split(fline, ' '));
    }
}

void Setup::findKeyword(vector<string> fline)
{
  string keyword = fline[0];
  if (keyword == "runname")
  {
    cout << "Runname command found" << endl;
    setRunName( fline[1] );
  } else if (keyword == "runtype")
  {
    cout << "Runtype command found" << endl;
    setRunType( fline[1] );
    if (fline[1] == "sim")
    {
      if (fline.size() > 2)
      {
        setNTraj( atoi( fline[2].c_str() ) );
      }
      if (fline.size() > 3)
      {
        setMaxTime( atoi( fline[3].c_str() ));
      }
    }
  } else if (keyword == "omp")
  {
    cout << "OMP command found" << endl;
    setOMP(atoi(fline[1].c_str()));
  } else if (keyword == "pbc")
  {
    cout << "PBC command found" << endl;
    setPBCT( atoi(fline[1].c_str()) );
    if ( getPBCs() > 0 )
      setBoxl(atof(fline[2].c_str()));
  } else if (keyword == "salt")
  {
    cout << "Salt command found" << endl;
    setSaltCon( atof(fline[1].c_str()) );
  } else if (keyword == "temp")
  {
    cout << "Temperature command found" << endl;
    setTemp( atof(fline[1].c_str()) );
  } else if (keyword == "idiel")
  {
    cout << "Interior dielectric command found" << endl;
    setIDiel( atof(fline[1].c_str()) );
  } else if (keyword == "sdiel")
  {
    cout << "Solvent dielectric command found" << endl;
    setSDiel( atof(fline[1].c_str()) );
  } else if (keyword == "attypes")
  {
    cout << "Atom Types command found" << endl;
    setNType( atoi(fline[1].c_str()) );
    resizeVecs();
    cout << "done with attypes" << endl;
  } else if (keyword == "type")
  {
    cout << "Type def command found" << endl;
    int typeNo = atoi(fline[1].c_str())-1;
    if (typeNo > getType()-1)
      return;
    if (fline.size() > 2)
      setTypeNCount( typeNo, atoi(fline[2].c_str()) );
    if (fline.size() > 3)
    {
      setTypeNDef( typeNo, fline[3].c_str() );
      if (getTypeNDef(typeNo) == "move")
        setTypeNDtr( typeNo, atof(fline[4].c_str()));
    }
  } else if (keyword == "pqr")
  {
    cout << "PQR command found" << endl;
    int typeNo = atoi(fline[1].c_str())-1;
    if (typeNo > getType()-1)
      return;
    setTypeNPQR( typeNo, fline[2].c_str() );
  } else if (keyword == "xyz")
  {
    cout << "XYZ command found" << endl;
    int typeNo = atoi(fline[1].c_str())-1;
    if (typeNo > getType()-1)
      return;
    setTypeNXYZ( typeNo, fline[2] );
    
  } else if (keyword == "random")
  {
    cout << "RNG Seed command found" << endl;
    setRand( atoi(fline[1].c_str()) );
  } else
    cout << "Keyword not found, read in as " << fline[0] << endl;
}

void Setup::resizeVecs()
{
  nTypenCount_.resize(nType_);
  typeDef_.resize(nType_);

  typeDiff_.resize(nType_);
  for (int i = 0; i<nType_; i++)
  {
    typeDiff_[i].resize(2);
  }

  molfnames_.resize(nType_);
  for(int i = 0; i < nType_; i++)
  {
    molfnames_[i].resize(5);
  }
} // end resizeVecs
