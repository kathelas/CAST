// Copyright (C) 2009-2015 National ICT Australia (NICTA)
// 
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
// -------------------------------------------------------------------
// 
// Written by Conrad Sanderson - http://conradsanderson.id.au


//! \addtogroup fn_chol
//! @{



template<typename T1>
inline
typename enable_if2< is_supported_blas_type<typename T1::elem_type>::value, const Op<T1, op_chol> >::result
chol
  (
  const Base<typename T1::elem_type,T1>& X,
  const char* layout = "upper"
  )
  {
  arma_extra_debug_sigprint();
  
  const char sig = (layout != NULL) ? layout[0] : char(0);
  
  arma_debug_check( ((sig != 'u') && (sig != 'l')), "chol(): layout must be \"upper\" or \"lower\"" );
  
  return Op<T1, op_chol>(X.get_ref(), ((sig == 'u') ? 0 : 1), 0 );
  }



template<typename T1>
inline
typename enable_if2< is_supported_blas_type<typename T1::elem_type>::value, bool >::result
chol
  (
         Mat<typename T1::elem_type>&    out,
  const Base<typename T1::elem_type,T1>& X,
  const char* layout = "upper"
  )
  {
  arma_extra_debug_sigprint();
  
  const char sig = (layout != NULL) ? layout[0] : char(0);
  
  arma_debug_check( ((sig != 'u') && (sig != 'l')), "chol(): layout must be \"upper\" or \"lower\"" );
  
  const bool status = auxlib::chol(out, X.get_ref(), ((sig == 'u') ? 0 : 1));
  
  if(status == false)
    {
    out.reset();
    arma_debug_warn("chol(): decomposition failed");
    }
  
  return status;
  }



//! @}
