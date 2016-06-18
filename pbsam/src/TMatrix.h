//
//  TMatrix.h
//  pbsam_xcode
//
//  Created by David Brookes on 6/15/16.
//  Copyright © 2016 David Brookes. All rights reserved.
//

#ifndef TMatrix_h
#define TMatrix_h

#include <map>
#include <memory>
#include <vector>
#include <stdio.h>
#include "SHCalc.h"
#include "ReExpCalc.h"

using namespace std;

/*
 Re-expansion coefficients
 */
class TMatrix
{
protected:
  
  /*
   enum for telling the ReExpCoeffs which values to retrieve
   in the below methods
   */
  enum WhichReEx { BASE, DDR, DDPHI, DDTHETA };
  
  int     p_;
  double  kappa_;
  
  vector<shared_ptr<ReExpCoeffs> > T_;
  
  // maps (I,k), (J,l) indices to a single index in T. If this returns -1
  // then the spheres are overlapping or less than 5A away and numerical
  // re-expansion is required
  map<vector<int>, int>   idxMap_;
  shared_ptr<SHCalc>      _shCalc_;
  shared_ptr<BesselCalc>  _besselCalc_;
  shared_ptr<System>      _system_;
  
  int         Nmol_;
  vector<int> Nsi_; // number of spheres in each molecule
  
  
  // inner functions for re-expansion
  MyMatrix<cmplx> expand_RX(MyMatrix<cmplx> X,
                            int I, int k, int J, int l,
                            WhichReEx whichR);
  
  MyMatrix<cmplx> expand_SX(MyMatrix<cmplx> x1,
                            int I, int k, int J, int l,
                            WhichReEx whichS);
  
  MyMatrix<cmplx> expand_RHX(MyMatrix<cmplx> x2,
                             int I, int k, int J, int l,
                             WhichReEx whichRH);
  
  MyMatrix<cmplx> expand_dRdtheta_sing(MyMatrix<cmplx> mat,
                                      int I, int k, int J, int l,
                                      double theta, bool ham);
  
  MyMatrix<cmplx> expand_dRdphi_sing(MyMatrix<cmplx> mat,
                                       int I, int k, int J, int l,
                                       double theta, bool ham);
  
  // returns True if (J,l) > (I,k)  (i.e. J > I or (I==J and k<l))
  bool is_Jl_greater(int I, int k, int J, int l);
  
  /*
   Convert derivatives from spherical to cartesian coords
   */
  VecOfMats<cmplx>::type conv_to_cart(VecOfMats<cmplx>::type dZ,
                                      int I, int k, int J, int l);
  
  
public:
  
  TMatrix() { }
  
  TMatrix(int p, shared_ptr<System> _sys, shared_ptr<SHCalc> _shcalc,
          shared_ptr<Constants> _consts, shared_ptr<BesselCalc> _besselcalc,
          shared_ptr<ReExpCoeffsConstants> _reexpconsts);
  
  // if these spheres can be re-expanded analytically, return true
  bool is_analytic(int I, int k, int J, int l)
  {
    if (idxMap_[{I, k, J, l}] == -1 ) return false;
    else return true;
  }
  
  // get re-expansion of sphere (I, k) with respect to (J, l)
  shared_ptr<ReExpCoeffs> get_T_Ik_Jl(int I, int k, int J, int l)
  {
    return T_[idxMap_[{I, k, J, l}]];
  }
  
  void update_vals(shared_ptr<System> _sys, shared_ptr<SHCalc> _shcalc,
                   shared_ptr<BesselCalc> _besselcalc,
                   shared_ptr<ReExpCoeffsConstants> _reexpconsts);
  
  /*
   Re-expand a matrix X with respect to T(I,k)(J,l)
   */
  MyMatrix<cmplx> re_expandX(MyMatrix<cmplx> X, int I, int k, int J, int l);
  
  /*
   re-expand element j of grad(X) with element (I,k,J l) of T.
   */
  MyMatrix<Ptx> re_expand_gradX(MyMatrix<Ptx> dX,
                                int I, int k, int J, int l);
  
  
  
  /*
   Re-expand X with element (I, k, J, l) of grad(T) and return
   a matrix of Point objects containing each element of the gradient
   */
  MyMatrix<Ptx> re_expandX_gradT(MyMatrix<cmplx> X,
                                 int I, int k,
                                 int J, int l);
  

  int get_nmol() const { return Nmol_; }
  
  int get_nsi(int i)   { return Nsi_[i]; }
  
  
  // convert a matrix of Pts into a vector of 3 matrices
  VecOfMats<cmplx>::type convert_from_ptx(MyMatrix<Ptx> X);
  //and do the opposite of the above
  MyMatrix<Ptx> convert_to_ptx(VecOfMats<cmplx>::type X);
  
};


#endif /* TMatrix_h */
