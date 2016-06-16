//
//  Solver.cpp
//  pbsam_xcode
//
//  Created by David Brookes on 5/17/16.
//  Copyright © 2016 David Brookes. All rights reserved.
//

#include "Solver.h"



vector<Pt> make_uniform_sph_grid(int m_grid, double r)
{
  
  vector<Pt> grid (m_grid);
  Pt gp;
  grid[0].set_r(r);
  grid[0].set_theta(0.0);
  grid[0].set_phi(0.0);
  double hk;
  for (int k = 0; k < m_grid; k++)
  {
    grid[k].set_r(r);
    hk = -1 + ((2 * (k-1)) / (m_grid-1));
    grid[k].set_theta(acos(hk));
    
    if (k==0 || k==m_grid-1) grid[k].set_phi(0);
    else
    {
      grid[k] = fmod(grid[k-1].phi() + (3.6/sqrt(m_grid) * (1/sqrt(1-(hk*hk)))),
                     2*M_PI);
    }
  }
  return grid;
}


ComplexMoleculeMatrix::ComplexMoleculeMatrix(int I, int ns, int p)
:p_(p), mat_(ns, MyMatrix<cmplx> (p, 2*p+1)), I_(I)
{
}

void ComplexMoleculeMatrix::reset_mat()
{
  for (int k = 0; k < mat_.size(); k++)
  {
    for (int i = 0; i < mat_[k].get_nrows(); i++)
    {
      for (int j = 0; j < mat_[k].get_ncols(); j++)
      {
        mat_[k].set_val(i, j, cmplx(0, 0));
      }
    }
  }
}

EMatrix::EMatrix(int I, int ns, int p)
:ComplexMoleculeMatrix(I, ns, p)
{
}

void EMatrix::calc_vals(Molecule mol, shared_ptr<SHCalc> _shcalc, double eps_in)
{
  cmplx val;
  Pt cen;
  double r_alpha, a_k, q_alpha;
  for (int k=0; k< mol.get_ns(); k++)
  {
    for (int alpha=0; alpha < mol.get_nc_k(k); alpha++)
    {
      cen = mol.get_posj(mol.get_ch_k_alpha(k, alpha));
      _shcalc->calc_sh(cen.theta(), cen.phi());
      for (int n = 0; n < p_; n++)
      {
        for (int m = -n; m < n+1; m++)
        {
          q_alpha = mol.get_qj(mol.get_ch_k_alpha(k, alpha));
          r_alpha = mol.get_radj(mol.get_ch_k_alpha(k, alpha));
          a_k = mol.get_ak(k);
          
          val = conj(_shcalc->get_result(n, m));
          val *= q_alpha / eps_in;
          val *= pow(r_alpha / a_k, n);
          val += get_mat_knm(k, n, m);
          set_mat_knm(k, n, m, val);
        }
      }
    }
  }
}

LEMatrix::LEMatrix(int I, int ns, int p)
:ComplexMoleculeMatrix(I, ns, p)
{
}

void LEMatrix::calc_vals(Molecule mol, shared_ptr<SHCalc> _shcalc,
                         double eps_in)
{
  cmplx val;
  Pt cen;
  
  double r_alpha, a_k, q_alpha;
  for (int k=0; k< mol.get_ns(); k++)
  {
    for (int alpha=0; alpha < mol.get_nc_k(k); alpha++)
    {
      if (mol.get_cg_of_ch(alpha) == k) continue;
      cen = mol.get_posj_realspace(alpha) - mol.get_centerk(k);
      cen = mol.get_posj(mol.get_ch_k_alpha(k, alpha));
      _shcalc->calc_sh(cen.theta(), cen.phi());
      for (int n = 0; n < p_; n++)
      {
        for (int m = -n; m < n+1; m++)
        {
          q_alpha = mol.get_qj(mol.get_ch_k_alpha(k, alpha));
          r_alpha = mol.get_radj(mol.get_ch_k_alpha(k, alpha));
          a_k = mol.get_ak(k);
          
          val = conj(_shcalc->get_result(n, m));
          val *= q_alpha / eps_in;
          val *= 1 / r_alpha;
          val *= pow(a_k / r_alpha, n);
          val += get_mat_knm(k, n, m);
          set_mat_knm(k, n, m, val);
        }
      }
    }
  }
}

IEMatrix::IEMatrix(int I, int ns, int p)
:p_(p), I_(I),
IE_(ns, MatOfMats<cmplx>::type(p, 2*p+1, MyMatrix<cmplx>(p, 2*p+1)))
{
}

IEMatrix::IEMatrix(int I, Molecule mol, shared_ptr<SHCalc> _shcalc, int p)
: p_(p), I_(I),
IE_(mol.get_ns(), MatOfMats<cmplx>::type(p, 2*p+1, MyMatrix<cmplx>(p, 2*p+1)))
{
  calc_vals(mol, _shcalc);
}


void IEMatrix::calc_vals(Molecule mol, shared_ptr<SHCalc> _shcalc)
{
  int m_grid = 20 * pow(p_, 2); //suggested in Yap 2010
  cmplx val;
  vector<Pt> grid_pts;
  Pt real_grid;
  for (int k = 0; k < mol.get_ns(); k++)
  {
    grid_pts = make_uniform_sph_grid(m_grid, mol.get_ak(k));
    // loop through other spheres in this molecule to find exposed grid points:
    for (int k2 = 0; k2< mol.get_ns(); k2++)
    {
      if (k2 == k) continue;
      for (int g = 0; g < m_grid; g++)
      {
        real_grid = grid_pts[g] - mol.get_centerk(k);
        // check if sphere is overlapping another
        if (real_grid.dist(mol.get_centerk(k2)) < mol.get_ak(k2)) continue;
        
        _shcalc->calc_sh(grid_pts[g].theta(), grid_pts[g].phi());
        for (int n = 0; n < p_; n++)
        {
          for (int m = -n; m < n+1; m++)
          {
            for (int l = 0; l < p_; l++)
            {
              for (int s = -l; s < l+1; s++)
              {
                val = get_IE_k_nm_ls(k, n, m, l, s);
                val += _shcalc->get_result(l, s) *
                conj(_shcalc->get_result(n, m));
                set_IE_k_nm_ls(k, n, m, l, s, val);
              }
            }
          }
        }
      }
    }
  }
}

void IEMatrix::reset_mat()
{
  for (int k = 0; k < IE_.size(); k++)
  {
    for (int n = 0; n < p_; n++)
    {
      for (int m = -n; m < n+1; m++)
      {
        for (int l = 0; l < p_; l++)
        {
          for (int s = -l; s < l+1; s++)
          {
            set_IE_k_nm_ls(k, n, m, l, s, cmplx(0, 0));
          }
        }
      }
    }
  }
}

LFMatrix::LFMatrix(int I, int ns, int p)
:ComplexMoleculeMatrix(I, ns, p)
{
}

void LFMatrix::calc_vals(shared_ptr<TMatrix> T, shared_ptr<FMatrix> F,
                         shared_ptr<SHCalc> shcalc, shared_ptr<System> sys)
{
  reset_mat();
  MyMatrix<cmplx> reex;
  for (int k = 0; k < mat_.size(); k++)
  {
    for (int j = 0; j < T->get_nsi(I_); j++)
    {
      if (j==k) continue;
      if (! T->is_analytic(I_, k, I_, j))
      {
        reex = T->re_expandX(F->get_mat_k(k), I_, k, I_, j);
      }
      else
      {
        reex = analytic_reex(I_, k, j, F, shcalc, sys);
      }
      mat_[k] += reex;
    }
  }
}

MyMatrix<cmplx> LFMatrix::analytic_reex(int I, int k, int j,
                                        shared_ptr<FMatrix> F,
                                        shared_ptr<SHCalc> shcalc,
                                        shared_ptr<System> sys, int Mp)
{
  if (Mp == -1) Mp = 2.5 * p_*p_;
  vector<Pt> sph_grid = make_uniform_sph_grid(Mp, sys->get_aik(I_, j));
  cmplx inner, fbj;
  Pt rb_k;
  MyMatrix<cmplx> LF_Ik (p_, 2 * p_ + 1);
  for (int n = 0; n < p_; n++)
  {
    for (int m = -n; m < n+1; m++)
    {
      for (int b = 0; b < Mp; b++)
      {
        fbj = make_fb_Ij(I, j, sph_grid[b], F, shcalc);
        fbj *= (4 * M_PI) / sph_grid.size();
        
        rb_k = sph_grid[b]-(sys->get_centerik(I, k)-sys->get_centerik(I, j));
        shcalc->calc_sh(rb_k.theta(), rb_k.phi());
        
        inner = fbj / rb_k.r();
        inner *= pow(sys->get_aik(I, k) / rb_k.r(), n);
        inner *= conj(shcalc->get_result(n, m));
        LF_Ik.set_val(n, m, inner);
      }
    }
  }
  return LF_Ik;
}

cmplx LFMatrix::make_fb_Ij(int I, int j, Pt rb,
                                  shared_ptr<FMatrix> F,
                                  shared_ptr<SHCalc> shcalc)
{
  cmplx fbj, fb_inner;
  shcalc->calc_sh(rb.theta(),rb.phi());
  
  //create fbj function
  fbj = 0;
  for (int n2 = 0; n2 < p_; n2++)
  {
    for (int m2 = -n2; m2 < n2+1; m2++)
    {
      fb_inner = ((2 * n2 + 1) / (4 * M_PI)) * F->get_mat_knm(j, n2, m2);
      fb_inner *= shcalc->get_result(n2, m2);
      fbj += fb_inner;
    }
  }
  fbj /= pow(rb.r(), 2); // make f_hat into f
  return fbj;
}

LHMatrix::LHMatrix(int I, int ns, int p, double kappa)
:ComplexMoleculeMatrix(I, ns, p), kappa_(kappa)
{
}

void LHMatrix::calc_vals(shared_ptr<TMatrix> T, shared_ptr<HMatrix> H,
                         shared_ptr<SHCalc> shcalc, shared_ptr<System> sys,
                         shared_ptr<BesselCalc> bcalc, int Mp)
{
  reset_mat();
  MyMatrix<cmplx> reex;
  for (int k = 0; k < mat_.size(); k++)
  {
    for (int j = 0; j < T->get_nsi(I_); j++)
    {
      if (j==k) continue;
      if (! T->is_analytic(I_, k, I_, j))
      {
        reex = T->re_expandX(H->get_mat_k(k), I_, k, I_, j );
      }
      else
      {
        reex = analytic_reex(I_, k, j, H, shcalc, sys, bcalc, Mp);
      }
      mat_[k] += reex;
    }
  }
}

MyMatrix<cmplx> LHMatrix::analytic_reex(int I, int k, int j,
                                        shared_ptr<HMatrix> H,
                                        shared_ptr<SHCalc> shcalc,
                                        shared_ptr<System> sys,
                                        shared_ptr<BesselCalc> bcalc,
                                        int Mp)
{
  if (Mp == -1) Mp = 2.5 * p_*p_;
  vector<Pt> sph_grid = make_uniform_sph_grid(Mp, sys->get_aik(I_, j));
  cmplx inner, hbj;
  Pt rb_k;
  MyMatrix<cmplx> LH_Ik (p_, 2 * p_ + 1);
  vector<double> besselk, besseli;

  for (int n = 0; n < p_; n++)
  {
    for (int m = -n; m < n+1; m++)
    {
      for (int b = 0; b < Mp; b++)
      {
        
        hbj = make_hb_Ij(I, j, sph_grid[b], H, shcalc, bcalc);
        hbj *= (4 * M_PI) / (Mp);
        
        rb_k = sph_grid[b]-(sys->get_centerik(I, k)-sys->get_centerik(I, j));
        shcalc->calc_sh(rb_k.theta(), rb_k.phi());
        besselk = bcalc->calc_mbfK(p_+1, kappa_ * rb_k.r());
        
        inner = hbj / rb_k.r();
        inner *= pow(sys->get_aik(I, k) / rb_k.r(), n);
        inner *= exp(-kappa_*rb_k.r());
        inner *= besselk[n];
        inner *= conj(shcalc->get_result(n, m));
        LH_Ik.set_val(n, m, inner);
      }
    }
  }
  return LH_Ik;
}

cmplx LHMatrix::make_hb_Ij(int I, int j, Pt rb,
                           shared_ptr<HMatrix> H,
                           shared_ptr<SHCalc> shcalc,
                           shared_ptr<BesselCalc> bcalc)
{
  vector<double> besseli;
  cmplx hb_inner, hbj;
  vector<cmplx> h;

  shcalc->calc_sh(rb.theta(), rb.phi());
  besseli = bcalc->calc_mbfI(p_+1,
                             kappa_*rb.r());
  hbj = 0;
  for (int n = 0; n < p_; n++)
  {
    for (int m = -n; m < n+1; m++)
    {
      hb_inner = ((2*n+1)/ (4*M_PI)) * H->get_mat_knm(j, n, m) ;
      hb_inner /= besseli[n];
      hb_inner *= shcalc->get_result(n, m);
      hbj += hb_inner;
    }
  }
  hbj /= pow(rb.r(), 2);  // make h_hat into h
  return hbj;
}

LHNMatrix::LHNMatrix(int I, int ns, int p)
:ComplexMoleculeMatrix(I, ns, p)
{
}

void LHNMatrix::calc_vals(shared_ptr<TMatrix> T,
                          vector<shared_ptr<HMatrix> > H)
{
  reset_mat();
  MyMatrix<cmplx> reex;
  for (int k = 0; k < mat_.size(); k++)
  {
    for (int J = 0; J < T->get_nmol(); J++)
    {
      if (J == I_) continue;
      for (int l =0 ; l < T->get_nsi(J); l++)
      {
        reex = T->re_expandX(H[J]->get_mat_k(l), I_, k, J, l);
        mat_[k] += reex;
      }
    }
  }
}

XHMatrix::XHMatrix(int I, int ns, int p)
:ComplexMoleculeMatrix(I, ns, p)
{
}

void XHMatrix::calc_vals(Molecule mol, shared_ptr<BesselCalc> bcalc,
                         shared_ptr<EMatrix> E, shared_ptr<LEMatrix> LE,
                         shared_ptr<LHMatrix> LH, shared_ptr<LFMatrix> LF,
                         shared_ptr<LHNMatrix> LHN)
{
  cmplx inner;
  double ak;
  vector<double> in_k;
  for (int k = 0; k < mol.get_ns(); k++)
  {
    ak = mol.get_ak(k);
    in_k = bcalc->calc_mbfI(p_, k * ak);
    for (int n = 0; n < p_; n++)
    {
      for (int m = - n; m < n+1; m++)
      {
        inner = E->get_mat_knm(k, n, m);
        inner += ak*(LE->get_mat_knm(k, n, m)+LF->get_mat_knm(k, n, m));
        inner -= ak*in_k[n]*(LH->get_mat_knm(k, n, m)+LHN->get_mat_knm(k, n, m));
        set_mat_knm(k, n, m, inner);
      }
    }
  }
}


XFMatrix::XFMatrix(int I, int ns, int p, double eps_in, double eps_out)
:ComplexMoleculeMatrix(I, ns, p), eps_(eps_in/eps_out)
{
}

void XFMatrix::calc_vals(Molecule mol, shared_ptr<BesselCalc> bcalc,
                         shared_ptr<EMatrix> E, shared_ptr<LEMatrix> LE,
                         shared_ptr<LHMatrix> LH, shared_ptr<LFMatrix> LF,
                         shared_ptr<LHNMatrix> LHN, double kappa)
{
  cmplx inner;
  double ak;
  vector<double> in_k;
  for (int k = 0; k < mol.get_ns(); k++)
  {
    ak = mol.get_ak(k);
    in_k = bcalc->calc_mbfI(p_+1, k * ak);
    for (int n = 0; n < p_; n++)
    {
      for (int m = - n; m < n+1; m++)
      {
        inner = (pow(kappa * ak, 2) * in_k[n+1])/(2*n+3);
        inner += n * in_k[n];
        inner *= ak * (LH->get_mat_knm(k, n, m) + LHN->get_mat_knm(k, n, m));
        inner += (n+1) * eps_ * E->get_mat_knm(k, n, m);
        inner -= n*eps_*ak*(LE->get_mat_knm(k, n, m)+LF->get_mat_knm(k, n, m));
        set_mat_knm(k, n, m, inner);
      }
    }
  }
}

HMatrix::HMatrix(int I, int ns, int p, double kappa)
:ComplexMoleculeMatrix(I, ns, p), kappa_(kappa)
{
}


void HMatrix::calc_vals(Molecule mol, shared_ptr<HMatrix> prev,
                        shared_ptr<XHMatrix> XH,
                        shared_ptr<FMatrix> F,
                        shared_ptr<IEMatrix> IE,
                        shared_ptr<BesselCalc> bcalc)
{
  cmplx val, inner, inner2;
  double ak;
  vector<double> bessel_i, bessel_k;
  for (int k = 0; k < mol.get_ns(); k++)  // loop over each sphere
  {
    ak = mol.get_ak(k);
    bessel_i = bcalc->calc_mbfI(p_+1, kappa_*ak);
    bessel_k = bcalc->calc_mbfK(p_+1, kappa_*ak);
    for (int n = 0; n < p_; n++)  // rows in new matrix
    {
      for (int m = -n; m < n+1; m++)  // columns in new matrix
      {
        val = 0;
        for (int l = 0; l < p_; l++)  // rows in old matrix
        {
          for (int s = -l; s < l+1; s++)  //columns in old matrix
          {
            inner = (2 * l + 1) / bessel_i[l];
            inner -= exp(-kappa_*ak) * bessel_k[l];
            inner *= prev->get_mat_knm(k, l, s);
            inner += F->get_mat_knm(k, l, s);
            inner += XH->get_mat_knm(k, l, s);
            inner *= IE->get_IE_k_nm_ls(k, n, m, l, s);
            val += inner;
          }
        }
        inner *= bessel_i[n];
        set_mat_knm(k, n, m, val);
      }
    }
  }
}

double HMatrix::calc_converge(shared_ptr<HMatrix> curr,
                              shared_ptr<HMatrix> prev)
{
  double mu=0, num, den;
  cmplx hnm_curr, hnm_prev;
  for (int k = 0; curr->mat_.size(); k++)
  {
    num = 0;
    den = 0;
    for (int n = 0; n < curr->get_p(); n++)
    {
      for (int m = -n; m < n+1; m++)
      {
        hnm_curr = curr->get_mat_knm(k, n, m);
        hnm_prev = prev->get_mat_knm(k, n, m);
        num += abs(hnm_curr - hnm_prev);
        den += abs(hnm_curr) + abs(hnm_prev);
      }
    }
    mu += num / (0.5 * den);
  }
  
  return mu;
}

FMatrix::FMatrix(int I, int ns, int p, double kappa)
:ComplexMoleculeMatrix(I, ns, p), kappa_(kappa)
{
}


void FMatrix::calc_vals(Molecule mol, shared_ptr<FMatrix> prev,
                        shared_ptr<XFMatrix> XF,
                        shared_ptr<HMatrix> H,
                        shared_ptr<IEMatrix> IE,
                        shared_ptr<BesselCalc> bcalc)
{
  cmplx val, inner, inner2;
  double ak;
  vector<double> bessel_i, bessel_k;
  for (int k = 0; k < mol.get_ns(); k++)  // loop over each sphere
  {
    ak = mol.get_ak(k);
    bessel_i = bcalc->calc_mbfI(p_+1, kappa_*ak);
    bessel_k = bcalc->calc_mbfK(p_+1, kappa_*ak);
    for (int n = 0; n < p_; n++)  // rows in new matrix
    {
      for (int m = -n; m < n+1; m++)  // columns in new matrix
      {
        val = 0;
        for (int l = 0; l < p_; l++)  // rows in old matrix
        {
          for (int s = -l; s < l+1; s++)  //columns in old matrix
          {
            inner = (l * bessel_k[l]) - ((2 * l +1) * bessel_k[l+1]);
            inner *= exp(-kappa_*ak) * H->get_mat_knm(k, l, s);
            inner += XF->get_mat_knm(k, l, s);
            inner += (2*l + 1 - l * XF->get_eps()) * prev->get_mat_knm(k, l, s);
            inner *= IE->get_IE_k_nm_ls(k, n, m, l, s);
            val += inner;
          }
        }
        set_mat_knm(k, n, m, val);
      }
    }
  }
}


Solver::Solver(shared_ptr<System> _sys, shared_ptr<Constants> _consts,
               shared_ptr<SHCalc> _shCalc, shared_ptr<BesselCalc> _bCalc,
               int p)
:_E_(_sys->get_n()),
_LE_(_sys->get_n()),
_IE_(_sys->get_n()),
_LF_(_sys->get_n()),
_LH_(_sys->get_n()),
_LHN_(_sys->get_n()),
_XF_(_sys->get_n()),
_XH_(_sys->get_n()),
_H_(_sys->get_n()),
_F_(_sys->get_n()),
_shCalc_(_shCalc),
_bCalc_(_bCalc),
_consts_(_consts),
_sys_(_sys),
p_(p),
kappa_(_consts->get_kappa())
{
  _reExConsts_ = make_shared<ReExpCoeffsConstants> (kappa_,
                                                    _sys_->get_lambda(), p_);
  _T_ = make_shared<TMatrix> (p_, _sys_, _shCalc_, _consts_,
                             _bCalc_, _reExConsts_);
  Molecule mol;
  // intialize all matrices
  for (int I = 0; I < _sys_->get_n(); I++)
  {
    mol = _sys_->get_molecule(I);
    
    _E_[I] = make_shared<EMatrix> (I, _sys_->get_Ns_i(I), p_);
    _E_[I]->calc_vals(mol, _shCalc_, _consts_->get_dielectric_prot());
    
    _LE_[I] = make_shared<LEMatrix> (I, _sys_->get_Ns_i(I), p_);
    _LE_[I]->calc_vals(mol, _shCalc_, _consts_->get_dielectric_prot());
    
    _IE_[I] = make_shared<IEMatrix>(I, _sys_->get_Ns_i(I), p_);
    _IE_[I]->calc_vals(_sys_->get_molecule(I), _shCalc_);
    
    _LF_[I] = make_shared<LFMatrix> (I, _sys_->get_Ns_i(I), p_);
    _LH_[I] = make_shared<LHMatrix> (I, _sys_->get_Ns_i(I), p_, kappa_);
    _LHN_[I] = make_shared<LHNMatrix> (I, _sys_->get_Ns_i(I), p_);
    
    _XF_[I] = make_shared<XFMatrix> (I, _sys_->get_Ns_i(I), p_,
                                     _consts_->get_dielectric_prot(),
                                     _consts_->get_dielectric_water());
    _XH_[I] = make_shared<XHMatrix> (I, _sys_->get_Ns_i(I), p_);
    
    _H_[I] = make_shared<HMatrix>(I, _sys_->get_Ns_i(I), p_, kappa_);
    _F_[I] = make_shared<FMatrix>(I, _sys_->get_Ns_i(I), p_, kappa_);
    _prevH_[I] = make_shared<HMatrix>(I, _sys_->get_Ns_i(I), p_, kappa_);
    _prevF_[I] = make_shared<FMatrix>(I, _sys_->get_Ns_i(I), p_, kappa_);
  }
}

double Solver::iter()
{
  double mu = 0;
  Molecule mol;
  // start an iteration by calculating XF and XH (is this right?)
  for (int I = 0; I < _sys_->get_n(); I++)
  {
    mol = _sys_->get_molecule(I);
    _XF_[I]->calc_vals(_sys_->get_molecule(I), _bCalc_, _E_[I], _LE_[I],
                      _LH_[I], _LF_[I], _LHN_[I], kappa_);
    _XH_[I]->calc_vals(_sys_->get_molecule(I), _bCalc_, _E_[I], _LE_[I],
                      _LH_[I], _LF_[I], _LHN_[I]);
    
    _H_[I]->calc_vals(mol, _prevH_[I], _XH_[I], _prevF_[I], _IE_[I], _bCalc_);
    _F_[I]->calc_vals(mol, _prevF_[I], _XF_[I], _prevH_[I], _IE_[I], _bCalc_);
    
    _LF_[I]->calc_vals(_T_, _F_[I], _shCalc_, _sys_);
    _LH_[I]->calc_vals(_T_, _H_[I], _shCalc_, _sys_, _bCalc_);
    
    mu += HMatrix::calc_converge(_H_[I], _prevH_[I]);
  }
  
  for (int I = 0; I < _sys_->get_n(); I++)
  {
    _LHN_[I]->calc_vals(_T_, _H_); // requires all Hs so do it after they are
                                    // calculated
  }
  update_prev();
  return mu;
}


void Solver::update_prev()
{
  for (int I = 0; I < _H_.size(); I++)
  {
    for (int k = 0; k < _sys_->get_Ns_i(I); k++)
    {
      for (int n = 0; n < p_; n++)
      {
        for (int m = 0; m < p_; m++)
        {
          _prevH_[I]->set_mat_knm(k, n, m, _H_[I]->get_mat_knm(k, n, m));
          _prevF_[I]->set_mat_knm(k, n, m, _F_[I]->get_mat_knm(k, n, m));
        }
      }
    }
  }
}

void Solver::solve(double tol, int maxiter)
{
  double mu;
  for (int t = 0; t < maxiter; t++)
  {
    mu = iter();
    if (mu < tol) break;
  }
}


void Solver::reset_all()
{
  for (int I = 0; I < _sys_->get_n(); I++)
  {
    _E_[I]->reset_mat();
    _LE_[I]->reset_mat();
    _IE_[I]->reset_mat();
    _LF_[I]->reset_mat();
    _LH_[I]->reset_mat();
    _LHN_[I]->reset_mat();
    _XF_[I]->reset_mat();
    _XH_[I]->reset_mat();
    _H_[I]->reset_mat();
    _F_[I]->reset_mat();
    _prevH_[I]->reset_mat();
    _prevF_[I]->reset_mat();
    
  }
}


