//
//  ASolverTest.h
//  pb_solvers_code
//
//  Created by Lisa Felberg on 10/8/15.
//  Copyright © 2015 Lisa Felberg. All rights reserved.
//

#ifndef ASolverTest_h
#define ASolverTest_h

#include "ASolver.h"

class ASolverTest
{
  
public :
  void RunASolverTest()
  {
    Constants const_;
    vector< Molecule > mol_;
    vector< Molecule > mol_sing_;
    
    mol_.clear( );
    Pt pos[2]     = { Pt( 0.0, 0.0, -5.0 ), Pt( 10.0, 7.8, 25.0 ) };
    Pt cgPos[2]   = { Pt( 0.0, 0.0, -5.5 ), Pt( 11.0, 6.9, 24.3 ) };
    double cg[2] = { 5.0, -0.4};
    double rd[2] = { 5.6, 10.4};
    
    Pt cgPosSi[2] = { Pt( 0.0, 0.0, -35.0 ), Pt( 0.0, 0.0, 0.0 ) };
    
    for (int molInd = 0; molInd < 2; molInd ++ )
    {
      int M = 1;
      vector<double> charges(M);
      vector<double> vdW(M); vector<Pt> posCharges(M);
      charges[0] = cg[molInd]; posCharges[0] = cgPos[molInd]; vdW[0] = 0.0;
      
      Molecule molNew("stat",rd[molInd],charges,posCharges,vdW,pos[molInd]);
      mol_.push_back( molNew );
      
      charges[0]    = 2.0; posCharges[0] = cgPosSi[molInd];
      
      Molecule molSing( "stat", 10.0, charges, posCharges, vdW);
      mol_sing_.push_back( molSing );
    }
    
    const int vals           = 10;
    BesselConstants bConsta( 2*vals );
    BesselCalc bCalcu( 2*vals, bConsta);
    SHCalcConstants SHConsta( 2*vals );
    SHCalc SHCalcu( 2*vals, SHConsta );
    System sys( const_, mol_ );
    ReExpCoeffsConstants re_exp_consts (sys.get_consts().get_kappa(),
                                        sys.get_lambda(), nvals);
    
    ASolver ASolvTest( 2, vals, bCalcu, SHCalcu, sys);
    ASolvTest.solve_A( 1E-4 );
    ASolvTest.solve_gradA(1E-12);
  }
};



#endif /* ASolverTest_h */
