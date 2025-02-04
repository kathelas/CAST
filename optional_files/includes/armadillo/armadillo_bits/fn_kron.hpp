// Copyright (C) 2009-2016 National ICT Australia (NICTA)
// 
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
// -------------------------------------------------------------------
// 
// Written by Conrad Sanderson - http://conradsanderson.id.au
// Written by Dimitrios Bouzas


//! \addtogroup fn_kron
//! @{



//! \brief
//! kronecker product of two matrices,
//! with the matrices having the same element type
template<typename T1, typename T2>
arma_inline
const Glue<T1,T2,glue_kron>
kron(const Base<typename T1::elem_type,T1>& A, const Base<typename T1::elem_type,T2>& B)
  {
  arma_extra_debug_sigprint();

  return Glue<T1, T2, glue_kron>(A.get_ref(), B.get_ref());
  }



//! \brief
//! kronecker product of two matrices,
//! with the matrices having different element types
template<typename T, typename T1, typename T2>
inline
Mat<typename eT_promoter<T1,T2>::eT>
kron(const Base<std::complex<T>,T1>& X, const Base<T,T2>& Y)
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<T> eT1;

  promote_type<eT1,T>::check();
  
  const unwrap<T1> tmp1(X.get_ref());
  const unwrap<T2> tmp2(Y.get_ref());
  
  const Mat<eT1>& A = tmp1.M;
  const Mat<T  >& B = tmp2.M;

  Mat<eT1> out;
  
  glue_kron::direct_kron(out, A, B);
  
  return out;
  }



//! \brief
//! kronecker product of two matrices,
//! with the matrices having different element types
template<typename T, typename T1, typename T2>
inline
Mat<typename eT_promoter<T1,T2>::eT>
kron(const Base<T,T1>& X, const Base<std::complex<T>,T2>& Y)
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<T> eT2;  

  promote_type<T,eT2>::check();
  
  const unwrap<T1> tmp1(X.get_ref());
  const unwrap<T2> tmp2(Y.get_ref());
  
  const Mat<T  >& A = tmp1.M;
  const Mat<eT2>& B = tmp2.M;

  Mat<eT2> out;
  
  glue_kron::direct_kron(out, A, B);
  
  return out;
  }



// template<typename T1, typename T2>
// inline
// SpMat<typename T1::elem_type>
// kron(const SpBase<typename T1::elem_type, T1>& A_expr, const SpBase<typename T1::elem_type, T2>& B_expr)
//   {
//   arma_extra_debug_sigprint();
//   
//   typedef typename T1::elem_type eT;
//   
//   const unwrap_spmat<T1> UA(A_expr.get_ref());
//   const unwrap_spmat<T2> UB(B_expr.get_ref());
//   
//   const SpMat<eT>& A = UA.M;
//   const SpMat<eT>& B = UB.M;
//   
//   const uword A_rows = A.n_rows;
//   const uword A_cols = A.n_cols;
//   const uword B_rows = B.n_rows;
//   const uword B_cols = B.n_cols;
//   
//   SpMat<eT> out(A_rows*B_rows, A_cols*B_cols);
//   
//   if(out.is_empty() == false)
//     {
//     typename SpMat<eT>::const_iterator it     = A.begin();
//     typename SpMat<eT>::const_iterator it_end = A.end();
//     
//     for(; it != it_end; ++it)
//       {
//       const uword i = it.row();
//       const uword j = it.col();
//       
//       const eT A_val = (*it);
//       
//       out.submat(i*B_rows, j*B_cols, (i+1)*B_rows-1, (j+1)*B_cols-1) = A_val * B;
//       }
//     }
//   
//   return out;
//   }



//! @}
