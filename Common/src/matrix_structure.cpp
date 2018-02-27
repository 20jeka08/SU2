/*!
 * \file matrix_structure.cpp
 * \brief Main subroutines for doing the sparse structures
 * \author F. Palacios, A. Bueno, T. Economon
 * \version 5.0.0 "Raven"
 *
 * SU2 Original Developers: Dr. Francisco D. Palacios.
 *                          Dr. Thomas D. Economon.
 *
 * SU2 Developers: Prof. Juan J. Alonso's group at Stanford University.
 *                 Prof. Piero Colonna's group at Delft University of Technology.
 *                 Prof. Nicolas R. Gauger's group at Kaiserslautern University of Technology.
 *                 Prof. Alberto Guardone's group at Polytechnic University of Milan.
 *                 Prof. Rafael Palacios' group at Imperial College London.
 *                 Prof. Edwin van der Weide's group at the University of Twente.
 *                 Prof. Vincent Terrapon's group at the University of Liege.
 *
 * Copyright (C) 2012-2017 SU2, the open-source CFD code.
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../include/matrix_structure.hpp"

template <class CalcType>
TCSysMatrix<CalcType>::TCSysMatrix(void) {
  
  size = SU2_MPI::GetSize();
  rank = SU2_MPI::GetRank();
  
  /*--- Array initialization ---*/

  matrix            = NULL;
  ILU_matrix        = NULL;
  row_ptr           = NULL;
  col_ind           = NULL;
  row_ptr_ilu       = NULL;
  col_ind_ilu       = NULL;
  block             = NULL;
  prod_block_vector = NULL;
  prod_row_vector   = NULL;
  aux_vector        = NULL;
  sum_vector        = NULL;
  invM              = NULL;
  block_weight      = NULL;
  block_inverse     = NULL;

  /*--- Linelet preconditioner ---*/
  
  LineletBool     = NULL;
  LineletPoint    = NULL;
  UBlock          = NULL;
  invUBlock       = NULL;
  LBlock          = NULL;
  yVector         = NULL;
  zVector         = NULL;
  rVector         = NULL;
  LFBlock         = NULL;
  LyVector        = NULL;
  FzVector        = NULL;
  max_nElem       = 0;
  ilu_fill_in = 0;
  
}

template <class CalcType>
TCSysMatrix<CalcType>::~TCSysMatrix(void) {
  
  unsigned long iElem;

  /*--- Memory deallocation ---*/
  
  if (matrix != NULL)             delete [] matrix;
  if (ILU_matrix != NULL)         delete [] ILU_matrix;
  if (row_ptr != NULL)            delete [] row_ptr;
  if (col_ind != NULL)            delete [] col_ind;

  if (ilu_fill_in != 0) {
    if (row_ptr_ilu != NULL) delete [] row_ptr_ilu;
    if (col_ind_ilu != NULL) delete [] col_ind_ilu;
  }
  
  if (block != NULL)              delete [] block;
  if (block_weight != NULL)       delete [] block_weight;
  if (block_inverse != NULL)      delete [] block_inverse;
  
  if (prod_block_vector != NULL)  delete [] prod_block_vector;
  if (prod_row_vector != NULL)    delete [] prod_row_vector;
  if (aux_vector != NULL)         delete [] aux_vector;
  if (sum_vector != NULL)         delete [] sum_vector;
  if (invM != NULL)               delete [] invM;
  if (LineletBool != NULL)        delete [] LineletBool;
  if (LineletPoint != NULL)       delete [] LineletPoint;
  
  for (iElem = 0; iElem < max_nElem; iElem++) {
    if (UBlock[iElem] != NULL)      delete [] UBlock[iElem];
    if (invUBlock[iElem] != NULL)   delete [] invUBlock[iElem];
    if (LBlock[iElem] != NULL)      delete [] LBlock[iElem];
    if (yVector[iElem] != NULL)     delete [] yVector[iElem];
    if (zVector[iElem] != NULL)     delete [] zVector[iElem];
    if (rVector[iElem] != NULL)     delete [] rVector[iElem];
  }
  if (UBlock != NULL)     delete [] UBlock;
  if (invUBlock != NULL)  delete [] invUBlock;
  if (LBlock != NULL)     delete [] LBlock;
  if (yVector != NULL)    delete [] yVector;
  if (zVector != NULL)    delete [] zVector;
  if (rVector != NULL)    delete [] rVector;

  if (LFBlock != NULL)    delete [] LFBlock;
  if (LyVector != NULL)   delete [] LyVector;
  if (FzVector != NULL)   delete [] FzVector;
  
}

template <class CalcType>
void TCSysMatrix<CalcType>::Initialize(unsigned long nPoint, unsigned long nPointDomain,
                            unsigned short nVar, unsigned short nEqn,
                            bool EdgeConnect, CGeometry *geometry, CConfig *config) {

  /*--- Don't delete *row_ptr, *col_ind because they are
   asigned to the Jacobian structure. ---*/

  unsigned long iPoint, *row_ptr, *col_ind, index, nnz, Elem, iVar;
  unsigned short iNeigh, iElem, iNode, *nNeigh, *nNeigh_ilu;
  vector<unsigned long>::iterator it;
  vector<unsigned long> vneighs, vneighs_ilu;
  
  /*--- Set the ILU fill in level --*/
   
  ilu_fill_in = config->GetLinear_Solver_ILU_n();
  
  /*--- Compute the number of neighbors ---*/
  
  nNeigh = new unsigned short [nPoint];
  for (iPoint = 0; iPoint < nPoint; iPoint++) {
    
    if (EdgeConnect) {
      nNeigh[iPoint] = (geometry->node[iPoint]->GetnPoint()+1);  // +1 -> to include diagonal element
    }
    else {
      vneighs.clear();
      for (iElem = 0; iElem < geometry->node[iPoint]->GetnElem(); iElem++) {
        Elem =  geometry->node[iPoint]->GetElem(iElem);
        for (iNode = 0; iNode < geometry->elem[Elem]->GetnNodes(); iNode++)
          vneighs.push_back(geometry->elem[Elem]->GetNode(iNode));
      }
      vneighs.push_back(iPoint);
      
      sort(vneighs.begin(), vneighs.end());
      it = unique(vneighs.begin(), vneighs.end());
      vneighs.resize(it - vneighs.begin());
      nNeigh[iPoint] = vneighs.size();
    }
    
  }
  
  /*--- Create row_ptr structure, using the number of neighbors ---*/
  
  row_ptr = new unsigned long [nPoint+1];
  row_ptr[0] = 0;
  for (iPoint = 0; iPoint < nPoint; iPoint++)
    row_ptr[iPoint+1] = row_ptr[iPoint] + nNeigh[iPoint];
  nnz = row_ptr[nPoint];
  
  /*--- Create col_ind structure ---*/
  
  col_ind = new unsigned long [nnz];
  for (iPoint = 0; iPoint < nPoint; iPoint++) {
    
    vneighs.clear();
    
    if (EdgeConnect) {
      for (iNeigh = 0; iNeigh < geometry->node[iPoint]->GetnPoint(); iNeigh++)
        vneighs.push_back(geometry->node[iPoint]->GetPoint(iNeigh));
      vneighs.push_back(iPoint);
    }
    else {
      for (iElem = 0; iElem < geometry->node[iPoint]->GetnElem(); iElem++) {
        Elem =  geometry->node[iPoint]->GetElem(iElem);
        for (iNode = 0; iNode < geometry->elem[Elem]->GetnNodes(); iNode++)
          vneighs.push_back(geometry->elem[Elem]->GetNode(iNode));
      }
      vneighs.push_back(iPoint);
    }
    
    sort(vneighs.begin(), vneighs.end());
    it = unique(vneighs.begin(), vneighs.end());
    vneighs.resize( it - vneighs.begin() );
    
    index = row_ptr[iPoint];
    for (iNeigh = 0; iNeigh < vneighs.size(); iNeigh++) {
      col_ind[index] = vneighs[iNeigh];
      index++;
    }
    
  }
  
  /*--- Set the indices in the in the sparce matrix structure, and memory allocation ---*/
  
  SetIndexes(nPoint, nPointDomain, nVar, nEqn, row_ptr, col_ind, nnz, config);
  
  /*--- Initialization matrix to zero ---*/
  
  SetValZero();
  
  delete [] nNeigh;
  
  /*--- ILU(n) preconditioner with a specific sparse structure ---*/
  
  if (ilu_fill_in != 0) {
    
    nNeigh_ilu = new unsigned short [nPoint];
    for (iPoint = 0; iPoint < nPoint; iPoint++) {
      
      vneighs_ilu.clear();
      SetNeighbours(geometry, iPoint, 0, ilu_fill_in, EdgeConnect, vneighs_ilu);
      sort(vneighs_ilu.begin(), vneighs_ilu.end());
      it = unique(vneighs_ilu.begin(), vneighs_ilu.end());
      vneighs_ilu.resize(it - vneighs_ilu.begin());
      nNeigh_ilu[iPoint] = vneighs_ilu.size();
      
    }
    
    row_ptr_ilu = new unsigned long [nPoint+1];
    row_ptr_ilu[0] = 0;
    for (iPoint = 0; iPoint < nPoint; iPoint++)
      row_ptr_ilu[iPoint+1] = row_ptr_ilu[iPoint] + nNeigh_ilu[iPoint];
    nnz_ilu = row_ptr_ilu[nPoint];
    
    /*--- Create col_ind structure ---*/
    
    col_ind_ilu = new unsigned long [nnz_ilu];
    for (iPoint = 0; iPoint < nPoint; iPoint++) {
      
      vneighs_ilu.clear();
      SetNeighbours(geometry, iPoint, 0, ilu_fill_in, EdgeConnect, vneighs_ilu);
      sort(vneighs_ilu.begin(), vneighs_ilu.end());
      it = unique(vneighs_ilu.begin(), vneighs_ilu.end());
      vneighs_ilu.resize( it - vneighs_ilu.begin() );
      
      index = row_ptr_ilu[iPoint];
      for (iNeigh = 0; iNeigh < vneighs_ilu.size(); iNeigh++) {
        col_ind_ilu[index] = vneighs_ilu[iNeigh];
        index++;
      }
      
    }
    
    ILU_matrix = new CalcType [nnz_ilu*nVar*nEqn];
    for (iVar = 0; iVar < nnz_ilu*nVar*nEqn; iVar++) ILU_matrix[iVar] = 0.0;
    
    delete [] nNeigh_ilu;
    
  }
  
}

template <class CalcType>
void TCSysMatrix<CalcType>::SetNeighbours(CGeometry *geometry, unsigned long iPoint, unsigned short deep_level, unsigned short fill_level,
                               bool EdgeConnect, vector<unsigned long> & vneighs) {
  unsigned long Point, iElem, Elem;
  unsigned short iNode;


  if (EdgeConnect) {
    vneighs.push_back(iPoint);
    for (iNode = 0; iNode < geometry->node[iPoint]->GetnPoint(); iNode++) {
      Point = geometry->node[iPoint]->GetPoint(iNode);
      vneighs.push_back(Point);
      if (deep_level < fill_level) SetNeighbours(geometry, Point, deep_level+1, fill_level, EdgeConnect, vneighs);
    }
  }
  else {
    for (iElem = 0; iElem < geometry->node[iPoint]->GetnElem(); iElem++) {
      Elem =  geometry->node[iPoint]->GetElem(iElem);
      for (iNode = 0; iNode < geometry->elem[Elem]->GetnNodes(); iNode++) {
        Point = geometry->elem[Elem]->GetNode(iNode);
        vneighs.push_back(Point);
        if (deep_level < fill_level) SetNeighbours(geometry, Point, deep_level+1, fill_level, EdgeConnect, vneighs);
      }
    }
  }
  
}

template <class CalcType>
void TCSysMatrix<CalcType>::SetIndexes(unsigned long val_nPoint, unsigned long val_nPointDomain, unsigned short val_nVar, unsigned short val_nEq, unsigned long* val_row_ptr, unsigned long* val_col_ind, unsigned long val_nnz, CConfig *config) {
  
  unsigned long iVar;
  
  nPoint       = val_nPoint;        // Assign number of points in the mesh
  nPointDomain = val_nPointDomain;  // Assign number of points in the mesh
  nVar         = val_nVar;          // Assign number of vars in each block system
  nEqn         = val_nEq;           // Assign number of eqns in each block system
  
  row_ptr      = val_row_ptr;       // Assign row values in the spare system structure (Jacobian structure)
  col_ind      = val_col_ind;       // Assign colums values in the spare system structure (Jacobian structure)
  nnz          = val_nnz;           // Assign number of possible non zero blocks in the spare system structure (Jacobian structure)
  
  if (ilu_fill_in == 0) {
    row_ptr_ilu  = val_row_ptr;       // Assign row values in the spare system structure (ILU structure)
    col_ind_ilu  = val_col_ind;       // Assign colums values in the spare system structure (ILU structure)
    nnz_ilu      = val_nnz;           // Assign number of possible non zero blocks in the spare system structure (ILU structure)
  }
  
  matrix            = new CalcType [nnz*nVar*nEqn];  // Reserve memory for the values of the matrix
  block             = new CalcType [nVar*nEqn];
  block_weight      = new CalcType [nVar*nEqn];
  block_inverse     = new CalcType [nVar*nEqn];

  prod_block_vector = new CalcType [nEqn];
  prod_row_vector   = new CalcType [nVar];
  aux_vector        = new CalcType [nVar];
  sum_vector        = new CalcType [nVar];
  
  /*--- Memory initialization ---*/
  
  for (iVar = 0; iVar < nnz*nVar*nEqn; iVar++) matrix[iVar] = 0.0;
  for (iVar = 0; iVar < nVar*nEqn; iVar++)     block[iVar] = 0.0;
  for (iVar = 0; iVar < nVar*nEqn; iVar++)     block_weight[iVar] = 0.0;
  for (iVar = 0; iVar < nVar*nEqn; iVar++)     block_inverse[iVar] = 0.0;

  for (iVar = 0; iVar < nEqn; iVar++)          prod_block_vector[iVar] = 0.0;
  for (iVar = 0; iVar < nVar; iVar++)          prod_row_vector[iVar] = 0.0;
  for (iVar = 0; iVar < nVar; iVar++)          aux_vector[iVar] = 0.0;
  for (iVar = 0; iVar < nVar; iVar++)          sum_vector[iVar] = 0.0;
  
  if (ilu_fill_in == 0) {

    /*--- Set specific preconditioner matrices (ILU) ---*/
    
    if ((config->GetKind_Linear_Solver_Prec() == ILU) ||
        ((config->GetKind_SU2() == SU2_DEF) && (config->GetKind_Deform_Linear_Solver_Prec() == ILU)) ||
        ((config->GetKind_SU2() == SU2_DOT) && (config->GetKind_Deform_Linear_Solver_Prec() == ILU)) ||
        (config->GetKind_Linear_Solver() == SMOOTHER_ILU) ||
        (config->GetFSI_Simulation() && config->GetKind_Deform_Linear_Solver_Prec() == ILU) ||
        (config->GetDiscrete_Adjoint() && config->GetKind_DiscAdj_Linear_Prec() == ILU)) {
      
      /*--- Reserve memory for the ILU matrix. ---*/
      
      ILU_matrix = new CalcType [nnz_ilu*nVar*nEqn];
      for (iVar = 0; iVar < nnz_ilu*nVar*nEqn; iVar++) ILU_matrix[iVar] = 0.0;
      
    }
    
  }
  
  /*--- Set specific preconditioner matrices (Jacobi and Linelet) ---*/
  
  if ((config->GetKind_Linear_Solver_Prec() == JACOBI) ||
      (config->GetKind_Linear_Solver_Prec() == LINELET) ||
   		((config->GetKind_SU2() == SU2_DEF) && (config->GetKind_Deform_Linear_Solver_Prec() == JACOBI)) ||
    	((config->GetKind_SU2() == SU2_DOT) && (config->GetKind_Deform_Linear_Solver_Prec() == JACOBI)) ||
      (config->GetKind_Linear_Solver() == SMOOTHER_JACOBI) ||
      (config->GetKind_Linear_Solver() == SMOOTHER_LINELET) ||
      (config->GetDiscrete_Adjoint() && config->GetKind_DiscAdj_Linear_Prec() == JACOBI) ||
      (config->GetFSI_Simulation() && config->GetKind_Deform_Linear_Solver_Prec() == JACOBI))   {
    
    /*--- Reserve memory for the values of the inverse of the preconditioner. ---*/
    
    invM = new CalcType [nPoint*nVar*nEqn];
    for (iVar = 0; iVar < nPoint*nVar*nEqn; iVar++) invM[iVar] = 0.0;

  }

}

template<class CalcType>
CalcType *TCSysMatrix<CalcType>::GetBlock(unsigned long block_i, unsigned long block_j) {
  
  unsigned long step = 0, index;
  
  for (index = row_ptr[block_i]; index < row_ptr[block_i+1]; index++) {
    step++;
    if (col_ind[index] == block_j) { return &(matrix[(row_ptr[block_i]+step-1)*nVar*nEqn]); }
  }
  return NULL;
  
}

template<class CalcType>
CalcType TCSysMatrix<CalcType>::GetBlock(unsigned long block_i, unsigned long block_j, unsigned short iVar, unsigned short jVar) {
  
  unsigned long step = 0, index;
  
  for (index = row_ptr[block_i]; index < row_ptr[block_i+1]; index++) {
    step++;
    if (col_ind[index] == block_j) { return matrix[(row_ptr[block_i]+step-1)*nVar*nEqn+iVar*nEqn+jVar]; }
  }
  return 0;
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::SetBlock(unsigned long block_i, unsigned long block_j, CalcType **val_block) {
  
  unsigned long iVar, jVar, index, step = 0;
  
  for (index = row_ptr[block_i]; index < row_ptr[block_i+1]; index++) {
    step++;
    if (col_ind[index] == block_j) {
      for (iVar = 0; iVar < nVar; iVar++)
        for (jVar = 0; jVar < nEqn; jVar++)
//          matrix[(row_ptr[block_i]+step-1)*nVar*nEqn+iVar*nEqn+jVar] = val_block[iVar][jVar];  // Allow AD in Matrix Structure (disabled temporarily to avoid conflicts)
          matrix[(row_ptr[block_i]+step-1)*nVar*nEqn+iVar*nEqn+jVar] = SU2_TYPE::GetValue(val_block[iVar][jVar]);
      break;
    }
  }
  
}

template<class CalcType>  
void TCSysMatrix<CalcType>::SetBlock(unsigned long block_i, unsigned long block_j, CalcType *val_block) {
  
  unsigned long iVar, jVar, index, step = 0;
  
  for (index = row_ptr[block_i]; index < row_ptr[block_i+1]; index++) {
    step++;
    if (col_ind[index] == block_j) {
      for (iVar = 0; iVar < nVar; iVar++)
        for (jVar = 0; jVar < nEqn; jVar++)
//          matrix[(row_ptr[block_i]+step-1)*nVar*nEqn+iVar*nEqn+jVar] = val_block[iVar*nVar+jVar];  // Allow AD in Matrix Structure (disabled temporarily to avoid conflicts)
          matrix[(row_ptr[block_i]+step-1)*nVar*nEqn+iVar*nEqn+jVar] = SU2_TYPE::GetValue(val_block[iVar*nVar+jVar]);
      break;
    }
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::AddBlock(unsigned long block_i, unsigned long block_j, CalcType **val_block) {
  
  unsigned long iVar, jVar, index, step = 0;
  
  for (index = row_ptr[block_i]; index < row_ptr[block_i+1]; index++) {
    step++;
    if (col_ind[index] == block_j) {
      for (iVar = 0; iVar < nVar; iVar++)
        for (jVar = 0; jVar < nEqn; jVar++)
//          matrix[(row_ptr[block_i]+step-1)*nVar*nEqn+iVar*nEqn+jVar] += val_block[iVar][jVar];  // Allow AD in Matrix Structure (disabled temporarily to avoid conflicts)
          matrix[(row_ptr[block_i]+step-1)*nVar*nEqn+iVar*nEqn+jVar] += SU2_TYPE::GetValue(val_block[iVar][jVar]);
      break;
    }
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::SubtractBlock(unsigned long block_i, unsigned long block_j, CalcType **val_block) {
  
  unsigned long iVar, jVar, index, step = 0;
  
  for (index = row_ptr[block_i]; index < row_ptr[block_i+1]; index++) {
    step++;
    if (col_ind[index] == block_j) {
      for (iVar = 0; iVar < nVar; iVar++)
        for (jVar = 0; jVar < nEqn; jVar++)
//         matrix[(row_ptr[block_i]+step-1)*nVar*nEqn+iVar*nEqn+jVar] -= val_block[iVar][jVar];  // Allow AD in Matrix Structure (disabled temporarily to avoid conflicts)
          matrix[(row_ptr[block_i]+step-1)*nVar*nEqn+iVar*nEqn+jVar] -= SU2_TYPE::GetValue(val_block[iVar][jVar]);
      break;
    }
  }
  
}

template<class CalcType>
CalcType *TCSysMatrix<CalcType>::GetBlock_ILUMatrix(unsigned long block_i, unsigned long block_j) {
  
  unsigned long step = 0, index;
  
  for (index = row_ptr_ilu[block_i]; index < row_ptr_ilu[block_i+1]; index++) {
    step++;
    if (col_ind_ilu[index] == block_j) { return &(ILU_matrix[(row_ptr_ilu[block_i]+step-1)*nVar*nEqn]); }
  }
  return NULL;
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::SetBlock_ILUMatrix(unsigned long block_i, unsigned long block_j, CalcType *val_block) {
  
  unsigned long iVar, jVar, index, step = 0;
  
  for (index = row_ptr_ilu[block_i]; index < row_ptr_ilu[block_i+1]; index++) {
    step++;
    if (col_ind_ilu[index] == block_j) {
      for (iVar = 0; iVar < nVar; iVar++)
        for (jVar = 0; jVar < nEqn; jVar++)
          ILU_matrix[(row_ptr_ilu[block_i]+step-1)*nVar*nEqn+iVar*nEqn+jVar] = val_block[iVar*nVar+jVar];
      break;
    }
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::SetBlockTransposed_ILUMatrix(unsigned long block_i, unsigned long block_j, CalcType *val_block) {

  unsigned long iVar, jVar, index, step = 0;

  for (index = row_ptr_ilu[block_i]; index < row_ptr_ilu[block_i+1]; index++) {
    step++;
    if (col_ind_ilu[index] == block_j) {
      for (iVar = 0; iVar < nVar; iVar++)
        for (jVar = 0; jVar < nEqn; jVar++)
          ILU_matrix[(row_ptr_ilu[block_i]+step-1)*nVar*nEqn+iVar*nEqn+jVar] = val_block[jVar*nVar+iVar];
      break;
    }
  }

}

template<class CalcType>
void TCSysMatrix<CalcType>::SubtractBlock_ILUMatrix(unsigned long block_i, unsigned long block_j, CalcType *val_block) {
  
  unsigned long iVar, jVar, index, step = 0;
  
  for (index = row_ptr_ilu[block_i]; index < row_ptr_ilu[block_i+1]; index++) {
    step++;
    if (col_ind_ilu[index] == block_j) {
      for (iVar = 0; iVar < nVar; iVar++)
        for (jVar = 0; jVar < nEqn; jVar++)
          ILU_matrix[(row_ptr_ilu[block_i]+step-1)*nVar*nEqn+iVar*nEqn+jVar] -= val_block[iVar*nVar+jVar];
      break;
    }
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::MatrixVectorProduct(CalcType *matrix, CalcType *vector, CalcType *product) {
  
  unsigned short iVar, jVar;
  
  for (iVar = 0; iVar < nVar; iVar++) {
    product[iVar] = 0.0;
    for (jVar = 0; jVar < nVar; jVar++) {
      product[iVar] += matrix[iVar*nVar+jVar] * vector[jVar];
    }
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::MatrixMatrixProduct(CalcType *matrix_a, CalcType *matrix_b, CalcType *product) {
  
  unsigned short iVar, jVar, kVar;

  for (iVar = 0; iVar < nVar; iVar++) {
    for (jVar = 0; jVar < nVar; jVar++) {
      product[iVar*nVar+jVar] = 0.0;
      for (kVar = 0; kVar < nVar; kVar++) {
        product[iVar*nVar+jVar] += matrix_a[iVar*nVar+kVar]*matrix_b[kVar*nVar+jVar];
      }
    }
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::AddVal2Diag(unsigned long block_i, CalcType val_matrix) {
  
  unsigned long step = 0, iVar, index;
  
  for (index = row_ptr[block_i]; index < row_ptr[block_i+1]; index++) {
    step++;
    if (col_ind[index] == block_i) {	// Only elements on the diagonal
      for (iVar = 0; iVar < nVar; iVar++)
//        matrix[(row_ptr[block_i]+step-1)*nVar*nVar+iVar*nVar+iVar] += val_matrix;  // Allow AD in Matrix Structure (disabled temporarily to avoid conflicts)
        matrix[(row_ptr[block_i]+step-1)*nVar*nVar+iVar*nVar+iVar] += SU2_TYPE::GetValue(val_matrix);
      break;
    }
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::SetVal2Diag(unsigned long block_i, CalcType val_matrix) {
  
  unsigned long step = 0, iVar, jVar, index;
  
  for (index = row_ptr[block_i]; index < row_ptr[block_i+1]; index++) {
    step++;
    if (col_ind[index] == block_i) {	// Only elements on the diagonal
      
      for (iVar = 0; iVar < nVar; iVar++)
        for (jVar = 0; jVar < nVar; jVar++)
          matrix[(row_ptr[block_i]+step-1)*nVar*nVar+iVar*nVar+jVar] = 0.0;
      
      for (iVar = 0; iVar < nVar; iVar++)
//        matrix[(row_ptr[block_i]+step-1)*nVar*nVar+iVar*nVar+iVar] = val_matrix;  // Allow AD in Matrix Structure (disabled temporarily to avoid conflicts)
        matrix[(row_ptr[block_i]+step-1)*nVar*nVar+iVar*nVar+iVar] = SU2_TYPE::GetValue(val_matrix);
      
      break;
    }
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::DeleteValsRowi(unsigned long i) {
  
  unsigned long block_i = i/nVar;
  unsigned long row = i - block_i*nVar;
  unsigned long index, iVar;
  
  for (index = row_ptr[block_i]; index < row_ptr[block_i+1]; index++) {
    for (iVar = 0; iVar < nVar; iVar++)
      matrix[index*nVar*nVar+row*nVar+iVar] = 0.0; // Delete row values in the block
    if (col_ind[index] == block_i)
      matrix[index*nVar*nVar+row*nVar+row] = 1.0; // Set 1 to the diagonal element
  }
  
}


template<class CalcType>
CalcType TCSysMatrix<CalcType>::MatrixDeterminant(CalcType **a, unsigned long n) {
  
  unsigned long i, j, j1, j2;
  CalcType det = 0;
  CalcType **m = NULL;
  
  if (n < 1) { }
  else if (n == 1) { det = a[0][0]; }
  else if (n == 2) { det = a[0][0] * a[1][1] - a[1][0] * a[0][1]; }
  else {
    det = 0.0;

    for (j1=0;j1<n;j1++) {
      m = new CalcType*[n-1];
      for (i=0;i<n-1;i++)
        m[i] = new CalcType[n-1];
      
      for (i=1;i<n;i++) {
        j2 = 0;
        for (j=0;j<n;j++) {
          if (j == j1)
          continue;
          m[i-1][j2] = a[i][j];
          j2++;
        }
      }
      
      det += pow(-1.0,j1+2.0) * a[0][j1] * MatrixDeterminant(m,n-1);
      for (i=0;i<n-1;i++)
      delete [] m[i];
      delete [] m;
    }
    
  }
  
  return(det);
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::MatrixCoFactor(CalcType **a, unsigned long n, CalcType **b) {
  
  unsigned long i,j,ii,jj,i1,j1;
  CalcType det;
  CalcType **c;
  
  c = new CalcType*[n-1];
  for (i=0;i<n-1;i++)
    c[i] = new CalcType[n-1];
  
  for (j=0;j<n;j++) {
    for (i=0;i<n;i++) {
      
      /*--- Form the adjoint a_ij ---*/
      i1 = 0;
      for (ii=0;ii<n;ii++) {
        if (ii == i)
        continue;
        j1 = 0;
        for (jj=0;jj<n;jj++) {
          if (jj == j)
          continue;
          c[i1][j1] = a[ii][jj];
          j1++;
        }
        i1++;
      }
      
      /*--- Calculate the determinate ---*/
      det = MatrixDeterminant(c,n-1);
      
      /*--- Fill in the elements of the cofactor ---*/
      b[i][j] = pow(-1.0,i+j+2.0) * det;
    }
  }
  for (i=0;i<n-1;i++)
    delete [] c[i];
  delete [] c;
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::MatrixTranspose(CalcType **a, unsigned long n) {
  
  unsigned long i, j;
  CalcType tmp;
  
  for (i=1;i<n;i++) {
    for (j=0;j<i;j++) {
      tmp = a[i][j];
      a[i][j] = a[j][i];
      a[j][i] = tmp;
    }
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::Gauss_Elimination(unsigned long block_i, CalcType* rhs, bool transposed) {
  
  short iVar, jVar, kVar; // This is important, otherwise some compilers optimizations will fail
  CalcType weight, aux;
  
  CalcType *Block = GetBlock(block_i, block_i);
  
  /*--- Copy block matrix, note that the original matrix
   is modified by the algorithm---*/
  
  if (!transposed) {
    for (iVar = 0; iVar < (short)nVar; iVar++)
      for (jVar = 0; jVar < (short)nVar; jVar++)
        block[iVar*nVar+jVar] = Block[iVar*nVar+jVar];
  } else {
    for (iVar = 0; iVar < (short)nVar; iVar++)
      for (jVar = 0; jVar < (short)nVar; jVar++)
        block[iVar*nVar+jVar] = Block[jVar*nVar+iVar];
  }
  /*--- Gauss elimination ---*/
  
  if (nVar == 1) {
    rhs[0] /= block[0];
  }
  else {
    
    /*--- Transform system in Upper Matrix ---*/
    
    for (iVar = 1; iVar < (short)nVar; iVar++) {
      for (jVar = 0; jVar < iVar; jVar++) {
        weight = block[iVar*nVar+jVar] / block[jVar*nVar+jVar];
        for (kVar = jVar; kVar < (short)nVar; kVar++)
          block[iVar*nVar+kVar] -= weight*block[jVar*nVar+kVar];
        rhs[iVar] -= weight*rhs[jVar];
      }
    }
    
    /*--- Backwards substitution ---*/
    
    rhs[nVar-1] = rhs[nVar-1] / block[nVar*nVar-1];
    for (iVar = (short)nVar-2; iVar >= 0; iVar--) {
      aux = 0.0;
      for (jVar = iVar+1; jVar < (short)nVar; jVar++)
        aux += block[iVar*nVar+jVar]*rhs[jVar];
      rhs[iVar] = (rhs[iVar]-aux) / block[iVar*nVar+iVar];
      if (iVar == 0) break;
    }
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::Gauss_Elimination_ILUMatrix(unsigned long block_i, CalcType* rhs) {
  
  short iVar, jVar, kVar; // This is important, otherwise some compilers optimizations will fail
  CalcType weight, aux;
  
  CalcType *Block = GetBlock_ILUMatrix(block_i, block_i);
  
  /*--- Copy block matrix, note that the original matrix
   is modified by the algorithm---*/
  
  for (iVar = 0; iVar < (short)nVar; iVar++)
    for (jVar = 0; jVar < (short)nVar; jVar++)
      block[iVar*nVar+jVar] = Block[iVar*nVar+jVar];
  
  /*--- Gauss elimination ---*/

  if (nVar == 1) {
    rhs[0] /= block[0];
  }
  else {
    
    /*--- Transform system in Upper Matrix ---*/
    
    for (iVar = 1; iVar < (short)nVar; iVar++) {
      for (jVar = 0; jVar < iVar; jVar++) {
        weight = block[iVar*nVar+jVar] / block[jVar*nVar+jVar];
        for (kVar = jVar; kVar < (short)nVar; kVar++)
          block[iVar*nVar+kVar] -= weight*block[jVar*nVar+kVar];
        rhs[iVar] -= weight*rhs[jVar];
      }
    }
    
    /*--- Backwards substitution ---*/
    
    rhs[nVar-1] = rhs[nVar-1] / block[nVar*nVar-1];
    for (iVar = (short)nVar-2; iVar >= 0; iVar--) {
      aux = 0.0;
      for (jVar = iVar+1; jVar < (short)nVar; jVar++)
        aux += block[iVar*nVar+jVar]*rhs[jVar];
      rhs[iVar] = (rhs[iVar]-aux) / block[iVar*nVar+iVar];
      if (iVar == 0) break;
    }
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::Gauss_Elimination(CalcType* Block, CalcType* rhs) {
  
  short iVar, jVar, kVar; // This is important, otherwise some compilers optimizations will fail
  CalcType weight, aux;
  
  /*--- Copy block matrix, note that the original matrix
   is modified by the algorithm---*/
  
  for (iVar = 0; iVar < (short)nVar; iVar++)
    for (jVar = 0; jVar < (short)nVar; jVar++)
      block[iVar*nVar+jVar] = Block[iVar*nVar+jVar];
  
  
  if (nVar == 1) {
    rhs[0] /= block[0];
  }
  else {
    /*--- Transform system in Upper Matrix ---*/
    for (iVar = 1; iVar < (short)nVar; iVar++) {
      for (jVar = 0; jVar < iVar; jVar++) {
        weight = block[iVar*nVar+jVar] / block[jVar*nVar+jVar];
        for (kVar = jVar; kVar < (short)nVar; kVar++)
          block[iVar*nVar+kVar] -= weight*block[jVar*nVar+kVar];
        rhs[iVar] -= weight*rhs[jVar];
      }
    }
    
    /*--- Backwards substitution ---*/
    rhs[nVar-1] = rhs[nVar-1] / block[nVar*nVar-1];
    for (iVar = (short)nVar-2; iVar >= 0; iVar--) {
      aux = 0.0;
      for (jVar = iVar+1; jVar < (short)nVar; jVar++)
        aux += block[iVar*nVar+jVar]*rhs[jVar];
      rhs[iVar] = (rhs[iVar]-aux) / block[iVar*nVar+iVar];
      if (iVar == 0) break;
    }
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::ProdBlockVector(unsigned long block_i, unsigned long block_j, const TCSysVector<CalcType> & vec) {
  
  unsigned long j = block_j*nVar;
  unsigned short iVar, jVar;
  
  CalcType *block = GetBlock(block_i, block_j);
  
  for (iVar = 0; iVar < nVar; iVar++) {
    prod_block_vector[iVar] = 0;
    for (jVar = 0; jVar < nVar; jVar++)
      prod_block_vector[iVar] += block[iVar*nVar+jVar]*vec[j+jVar];
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::UpperProduct(TCSysVector<CalcType> & vec, unsigned long row_i) {
  
  unsigned long iVar, index;
  
  for (iVar = 0; iVar < nVar; iVar++)
    prod_row_vector[iVar] = 0;
  
  for (index = row_ptr[row_i]; index < row_ptr[row_i+1]; index++) {
    if (col_ind[index] > row_i) {
      ProdBlockVector(row_i, col_ind[index], vec);
      for (iVar = 0; iVar < nVar; iVar++)
        prod_row_vector[iVar] += prod_block_vector[iVar];
    }
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::LowerProduct(TCSysVector<CalcType> & vec, unsigned long row_i) {
  
  unsigned long iVar, index;
  
  for (iVar = 0; iVar < nVar; iVar++)
    prod_row_vector[iVar] = 0;
  
  for (index = row_ptr[row_i]; index < row_ptr[row_i+1]; index++) {
    if (col_ind[index] < row_i) {
      ProdBlockVector(row_i, col_ind[index], vec);
      for (iVar = 0; iVar < nVar; iVar++)
        prod_row_vector[iVar] += prod_block_vector[iVar];
    }
  }

}

template<class CalcType>
void TCSysMatrix<CalcType>::DiagonalProduct(TCSysVector<CalcType> & vec, unsigned long row_i) {
  
  unsigned long iVar, index;
  
  for (iVar = 0; iVar < nVar; iVar++)
    prod_row_vector[iVar] = 0;
  
  for (index = row_ptr[row_i]; index < row_ptr[row_i+1]; index++) {
    if (col_ind[index] == row_i) {
      ProdBlockVector(row_i, col_ind[index], vec);
      for (iVar = 0; iVar < nVar; iVar++)
        prod_row_vector[iVar] += prod_block_vector[iVar];
    }
  }
  
}


template<class CalcType>
void TCSysMatrix<CalcType>::RowProduct(const TCSysVector<CalcType> & vec, unsigned long row_i) {
  
  unsigned long iVar, index;
  
  for (iVar = 0; iVar < nVar; iVar++)
    prod_row_vector[iVar] = 0;
  
  for (index = row_ptr[row_i]; index < row_ptr[row_i+1]; index++) {
    ProdBlockVector(row_i, col_ind[index], vec);
    for (iVar = 0; iVar < nVar; iVar++)
      prod_row_vector[iVar] += prod_block_vector[iVar];
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::MatrixVectorProduct(const TCSysVector<CalcType> & vec, TCSysVector<CalcType> & prod) {
  
  unsigned long iPoint, iVar;
  
  for (iPoint = 0; iPoint < nPointDomain; iPoint++) {
    RowProduct(vec, iPoint);
    for (iVar = 0; iVar < nVar; iVar++)
      prod[iPoint*nVar+iVar] = prod_row_vector[iVar];
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::MatrixVectorProduct(const TCSysVector<CalcType> & vec, TCSysVector<CalcType> & prod, CGeometry *geometry, CConfig *config) {
  
  unsigned long prod_begin, vec_begin, mat_begin, index, iVar, jVar, row_i;
  
  /*--- Some checks for consistency between CSysMatrix and the CSysVectors ---*/
  if ( (nVar != vec.GetNVar()) || (nVar != prod.GetNVar()) ) {
    cerr << "TCSysMatrix<CalcType>::MatrixVectorProduct(const CSysVector&, CSysVector): "
    << "nVar values incompatible." << endl;
    throw(-1);
  }
  if ( (nPoint != vec.GetNBlk()) || (nPoint != prod.GetNBlk()) ) {
    cerr << "TCSysMatrix<CalcType>::MatrixVectorProduct(const CSysVector&, CSysVector): "
    << "nPoint and nBlk values incompatible." << endl;
    throw(-1);
  }
  
  prod = CalcType(0.0); // set all entries of prod to zero
  for (row_i = 0; row_i < nPointDomain; row_i++) {
    prod_begin = row_i*nVar; // offset to beginning of block row_i
    for (index = row_ptr[row_i]; index < row_ptr[row_i+1]; index++) {
      vec_begin = col_ind[index]*nVar; // offset to beginning of block col_ind[index]
      mat_begin = (index*nVar*nVar); // offset to beginning of matrix block[row_i][col_ind[indx]]
      for (iVar = 0; iVar < nVar; iVar++) {
        for (jVar = 0; jVar < nVar; jVar++) {
          prod[(unsigned long)(prod_begin+iVar)] += matrix[(unsigned long)(mat_begin+iVar*nVar+jVar)]*vec[(unsigned long)(vec_begin+jVar)];
        }
      }
    }
  }
  
  /*--- MPI Parallelization ---*/
  prod.SendReceive(geometry, config);
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::MatrixVectorProductTransposed(const TCSysVector<CalcType> & vec, TCSysVector<CalcType> & prod, CGeometry *geometry, CConfig *config) {

  unsigned long prod_begin, vec_begin, mat_begin, index, iVar, jVar , row_i;

  /*--- Some checks for consistency between CSysMatrix and the CSysVectors ---*/
  if ( (nVar != vec.GetNVar()) || (nVar != prod.GetNVar()) ) {
    SU2_MPI::Error("nVar values incompatible.", CURRENT_FUNCTION);
  }
  if ( (nPoint != vec.GetNBlk()) || (nPoint != prod.GetNBlk()) ) {
    SU2_MPI::Error("nPoint and nBlk values incompatible.", CURRENT_FUNCTION);
  }

  prod = CalcType(0.0); // set all entries of prod to zero
  for (row_i = 0; row_i < nPointDomain; row_i++) {
    vec_begin = row_i*nVar; // offset to beginning of block col_ind[index]
    for (index = row_ptr[row_i]; index < row_ptr[row_i+1]; index++) {
      prod_begin = col_ind[index]*nVar; // offset to beginning of block row_i
      mat_begin = (index*nVar*nVar); // offset to beginning of matrix block[row_i][col_ind[indx]]
      for (iVar = 0; iVar < nVar; iVar++) {
        for (jVar = 0; jVar < nVar; jVar++) {
            prod[(unsigned long)(prod_begin+jVar)] += matrix[(unsigned long)(mat_begin+iVar*nVar+jVar)]*vec[(unsigned long)(vec_begin+iVar)];
        }
      }
    }
  }

  /*--- MPI Parallelization ---*/
  
  prod.SendReceive_Reverse(geometry, config);

}

template<class CalcType>
void TCSysMatrix<CalcType>::GetMultBlockBlock(CalcType *c, CalcType *a, CalcType *b) {
  
  unsigned long iVar, jVar, kVar;
  
  for (iVar = 0; iVar < nVar; iVar++)
    for (jVar = 0; jVar < nVar; jVar++) {
      c[iVar*nVar+jVar] = 0.0;
      for (kVar = 0; kVar < nVar; kVar++)
        c[iVar*nVar+jVar] += a[iVar*nVar+kVar] * b[kVar*nVar+jVar];
    }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::GetMultBlockVector(CalcType *c, CalcType *a, CalcType *b) {
  
  unsigned long iVar, jVar;
  
  for (iVar = 0; iVar < nVar; iVar++) {
    c[iVar] =  0.0;
    for (jVar = 0; jVar < nVar; jVar++)
      c[iVar] += a[iVar*nVar+jVar] * b[jVar];
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::GetSubsBlock(CalcType *c, CalcType *a, CalcType *b) {
  
  unsigned long iVar, jVar;
  
  for (iVar = 0; iVar < nVar; iVar++)
    for (jVar = 0; jVar < nVar; jVar++)
      c[iVar*nVar+jVar] = a[iVar*nVar+jVar] - b[iVar*nVar+jVar];
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::GetSubsVector(CalcType *c, CalcType *a, CalcType *b) {
  
  unsigned long iVar;
  
  for (iVar = 0; iVar < nVar; iVar++)
    c[iVar] = a[iVar] - b[iVar];
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::InverseBlock(CalcType *Block, CalcType *invBlock) {
  
  unsigned long iVar, jVar;
  
  for (iVar = 0; iVar < nVar; iVar++) {
    for (jVar = 0; jVar < nVar; jVar++)
      aux_vector[jVar] = 0.0;
    aux_vector[iVar] = 1.0;
    
    /*--- Compute the i-th column of the inverse matrix ---*/
    Gauss_Elimination(Block, aux_vector);
    
    for (jVar = 0; jVar < nVar; jVar++)
      invBlock[jVar*nVar+iVar] = aux_vector[jVar];
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::InverseDiagonalBlock(unsigned long block_i, CalcType *invBlock, bool transpose) {
  
  unsigned long iVar, jVar;
  
  for (iVar = 0; iVar < nVar; iVar++) {
    for (jVar = 0; jVar < nVar; jVar++)
      aux_vector[jVar] = 0.0;
    aux_vector[iVar] = 1.0;
    
    /*--- Compute the i-th column of the inverse matrix ---*/
    
    Gauss_Elimination(block_i, aux_vector, transpose);
    for (jVar = 0; jVar < nVar; jVar++)
      invBlock[jVar*nVar+iVar] = aux_vector[jVar];
  }
  
  //  CalcType Det, **Matrix, **CoFactor;
  //  CalcType *Block = GetBlock(block_i, block_i);
  //
  //  Matrix = new CalcType*[nVar];
  //  CoFactor = new CalcType*[nVar];
  //  for (iVar=0;iVar<nVar;iVar++) {
  //    Matrix[iVar] = new CalcType[nVar];
  //    CoFactor[iVar] = new CalcType[nVar];
  //  }
  //
  //  for (iVar = 0; iVar < nVar; iVar++) {
  //    for (jVar = 0; jVar < nVar; jVar++)
  //    Matrix[iVar][jVar] = Block[jVar*nVar+iVar];
  //  }
  //
  //  Det =  MatrixDeterminant(Matrix, nVar);
  //  MatrixCoFactor(Matrix, nVar, CoFactor);
  //  MatrixTranspose(CoFactor, nVar);
  //
  //
  //  for (iVar = 0; iVar < nVar; iVar++) {
  //    for (jVar = 0; jVar < nVar; jVar++)
  //    invBlock[jVar*nVar+iVar] = CoFactor[iVar][jVar]/Det;
  //  }
  //
  //  for (iVar = 0; iVar < nVar; iVar++) {
  //    delete [] Matrix[iVar];
  //    delete [] CoFactor[iVar];
  //  }
  //  delete [] Matrix;
  //  delete [] CoFactor;
  
}


template<class CalcType>
void TCSysMatrix<CalcType>::InverseDiagonalBlock_ILUMatrix(unsigned long block_i, CalcType *invBlock) {
  
  unsigned long iVar, jVar;

  for (iVar = 0; iVar < nVar; iVar++) {
    for (jVar = 0; jVar < nVar; jVar++)
      aux_vector[jVar] = 0.0;
    aux_vector[iVar] = 1.0;
    
    /*--- Compute the i-th column of the inverse matrix ---*/
    
    Gauss_Elimination_ILUMatrix(block_i, aux_vector);
    for (jVar = 0; jVar < nVar; jVar++)
      invBlock[jVar*nVar+iVar] = aux_vector[jVar];
  }
  
  //  CalcType Det, **Matrix, **CoFactor;
  //  CalcType *Block = GetBlock_ILUMatrix(block_i, block_i);
  //
  //  Matrix = new CalcType*[nVar];
  //  CoFactor = new CalcType*[nVar];
  //  for (iVar=0;iVar<nVar;iVar++) {
  //    Matrix[iVar] = new CalcType[nVar];
  //    CoFactor[iVar] = new CalcType[nVar];
  //  }
  //
  //  for (iVar = 0; iVar < nVar; iVar++) {
  //    for (jVar = 0; jVar < nVar; jVar++)
  //    Matrix[iVar][jVar] = Block[jVar*nVar+iVar];
  //  }
  //
  //  Det =  MatrixDeterminant(Matrix, nVar);
  //  MatrixCoFactor(Matrix, nVar, CoFactor);
  //  MatrixTranspose(CoFactor, nVar);
  //
  //
  //  for (iVar = 0; iVar < nVar; iVar++) {
  //    for (jVar = 0; jVar < nVar; jVar++)
  //    invBlock[jVar*nVar+iVar] = CoFactor[iVar][jVar]/Det;
  //  }
  //
  //  for (iVar = 0; iVar < nVar; iVar++) {
  //    delete [] Matrix[iVar];
  //    delete [] CoFactor[iVar];
  //  }
  //  delete [] Matrix;
  //  delete [] CoFactor;
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::BuildJacobiPreconditioner(bool transpose) {

  unsigned long iPoint, iVar, jVar;

  /*--- Compute Jacobi Preconditioner ---*/
  for (iPoint = 0; iPoint < nPoint; iPoint++) {

    /*--- Compute the inverse of the diagonal block ---*/
    InverseDiagonalBlock(iPoint, block_inverse, transpose);

    /*--- Set the inverse of the matrix to the invM structure (which is a vector) ---*/
    for (iVar = 0; iVar < nVar; iVar++)
      for (jVar = 0; jVar < nVar; jVar++)
        invM[iPoint*nVar*nVar+iVar*nVar+jVar] = block_inverse[iVar*nVar+jVar];
  }

}


template<class CalcType>
void TCSysMatrix<CalcType>::ComputeJacobiPreconditioner(const TCSysVector<CalcType> & vec, TCSysVector<CalcType> & prod, CGeometry *geometry, CConfig *config) {
  
  unsigned long iPoint, iVar, jVar;
  
  for (iPoint = 0; iPoint < nPointDomain; iPoint++) {
    for (iVar = 0; iVar < nVar; iVar++) {
      prod[(unsigned long)(iPoint*nVar+iVar)] = 0.0;
      for (jVar = 0; jVar < nVar; jVar++)
        prod[(unsigned long)(iPoint*nVar+iVar)] +=
        invM[(unsigned long)(iPoint*nVar*nVar+iVar*nVar+jVar)]*vec[(unsigned long)(iPoint*nVar+jVar)];
    }
  }
  
  /*--- MPI Parallelization ---*/
  
  prod.SendReceive(geometry, config);
  
}

template<class CalcType>
unsigned long TCSysMatrix<CalcType>::Jacobi_Smoother(const TCSysVector<CalcType> & b, TCSysVector<CalcType> & x, TCMatrixVectorProduct<CalcType> & mat_vec, CalcType tol, unsigned long m, CalcType *residual, bool monitoring, CGeometry *geometry, CConfig *config) {
  
  unsigned long iPoint, iVar, jVar;
  
  /*---  Check the number of iterations requested ---*/
  
  if (m < 1) {
    char buf[100];
    SPRINTF(buf, "Illegal value for smoothing iterations, m = %lu", m );
    SU2_MPI::Error(string(buf), CURRENT_FUNCTION);
  }
  
  /*--- Create vectors to hold the residual and the Matrix-Vector product
   of the Jacobian matrix with the current solution (x^k). These must be
   stored in order to perform multiple iterations of the smoother. ---*/
  
  TCSysVector<CalcType> r(b);
  TCSysVector<CalcType> A_x(b);
  
  /*--- Calculate the initial residual, compute norm, and check
   if system is already solved. Recall, r holds b initially. ---*/
  
  mat_vec(x, A_x);
  r -= A_x;
  CalcType norm_r = r.norm();
  CalcType norm0  = b.norm();
  if ( (norm_r < tol*norm0) || (norm_r < eps) ) {
    if (rank == MASTER_NODE) cout << "TCSysMatrix<CalcType>::Jacobi_Smoother(): system solved by initial guess." << endl;
    return 0;
  }
  
  /*--- Set the norm to the initial initial residual value ---*/
  
  norm0 = norm_r;
  
  /*--- Output header information including initial residual ---*/
  
  int i = 0;
  if ((monitoring) && (rank == MASTER_NODE)) {
    cout << "\n# " << "Jacobi Smoother" << " residual history" << endl;
    cout << "# Residual tolerance target = " << tol << endl;
    cout << "# Initial residual norm     = " << norm_r << endl;
    cout << "     " << i << "     " << norm_r/norm0 << endl;
  }
  
  /*---  Loop over all smoothing iterations ---*/
  
  for (i = 0; i < (int)m; i++) {
    
    /*--- Apply the Jacobi smoother, i.e., multiply by the inverse of the
     diagonal matrix of A, which was built in the preprocessing phase. Note
     that we are directly updating the solution (x^k+1) during the loop. ---*/
    
    for (iPoint = 0; iPoint < nPointDomain; iPoint++) {
      for (iVar = 0; iVar < nVar; iVar++) {
        for (jVar = 0; jVar < nVar; jVar++)
          x[(unsigned long)(iPoint*nVar+iVar)] +=
          invM[(unsigned long)(iPoint*nVar*nVar+iVar*nVar+jVar)]*r[(unsigned long)(iPoint*nVar+jVar)];
      }
    }
    
    /*--- MPI Parallelization ---*/
    
    x.SendReceive(geometry, config);
    
    /*--- Update the residual (r^k+1 = b - A*x^k+1) with the new solution ---*/
    
    r = b;
    mat_vec(x, A_x);
    r -= A_x;
    
    /*--- Check if solution has converged, else output the relative
     residual if necessary. ---*/
    
    norm_r = r.norm();
    if (norm_r < tol*norm0) break;
    if (((monitoring) && (rank == MASTER_NODE)) && ((i+1) % 5 == 0))
      cout << "     " << i << "     " << norm_r/norm0 << endl;
    
  }
  
  if ((monitoring) && (rank == MASTER_NODE)) {
    cout << "# Jacobi smoother final (true) residual:" << endl;
    cout << "# Iteration = " << i << ": |res|/|res0| = "  << norm_r/norm0 << ".\n" << endl;
  }
  
  return (unsigned long) i;
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::BuildILUPreconditioner(bool transposed) {
  
  unsigned long index, index_, iVar;
  CalcType *Block_ij, *Block_jk;
  long iPoint, jPoint, kPoint;
  

  /*--- Copy block matrix, note that the original matrix
   is modified by the algorithm, so that we have the factorization stored
   in the ILUMatrix at the end of this preprocessing. ---*/

  for (iVar = 0; iVar < nnz_ilu*nVar*nEqn; iVar++) ILU_matrix[iVar] = 0.0;

  for (iPoint = 0; iPoint < (long)nPointDomain; iPoint++) {
    for (index = row_ptr[iPoint]; index < row_ptr[iPoint+1]; index++) {
      jPoint = col_ind[index];
      if (transposed) {
        Block_ij = GetBlock(jPoint, iPoint);
        SetBlockTransposed_ILUMatrix(iPoint, jPoint, Block_ij);
      } else {
        Block_ij = GetBlock(iPoint, jPoint);
        SetBlock_ILUMatrix(iPoint, jPoint, Block_ij);
      }
    }
  }
  
  /*--- Transform system in Upper Matrix ---*/
  
  for (iPoint = 1; iPoint < (long)nPointDomain; iPoint++) {
    
    /*--- For each row (unknown), loop over all entries in A on this row
     row_ptr_ilu[iPoint+1] will have the index for the first entry on the next
     row. ---*/
    
    for (index = row_ptr_ilu[iPoint]; index < row_ptr_ilu[iPoint+1]; index++) {
      
      /*--- jPoint here is the column for each entry on this row ---*/
      
      jPoint = col_ind_ilu[index];
      
      /*--- Check that this column is in the lower triangular portion ---*/
      
      if ((jPoint < iPoint) && (jPoint < (long)nPointDomain)) {
        
        /*--- If we're in the lower triangle, get the pointer to this block,
         invert it, and then right multiply against the original block ---*/
        
        Block_ij = GetBlock_ILUMatrix(iPoint, jPoint);
        InverseDiagonalBlock_ILUMatrix(jPoint, block_inverse);
        MatrixMatrixProduct(Block_ij, block_inverse, block_weight);
        
        /*--- block_weight holds Aij*inv(Ajj). Jump to the row for jPoint ---*/
        
        for (index_ = row_ptr_ilu[jPoint]; index_ < row_ptr_ilu[jPoint+1]; index_++) {
          
          /*--- Get the column of the entry ---*/
          
          kPoint = col_ind_ilu[index_];
          
          /*--- If the column is greater than or equal to jPoint, i.e., the
           upper triangular part, then multiply and modify the matrix.
           Here, Aik' = Aik - Aij*inv(Ajj)*Ajk. ---*/
          
          if ((kPoint >= jPoint) && (jPoint < (long)nPointDomain)) {
            
            Block_jk = GetBlock_ILUMatrix(jPoint, kPoint);
            MatrixMatrixProduct(block_weight, Block_jk, block);
            SubtractBlock_ILUMatrix(iPoint, kPoint, block);
            
          }
        }
        
        /*--- Lastly, store block_weight in the lower triangular part, which
         will be reused during the forward solve in the precon/smoother. ---*/
        
        SetBlock_ILUMatrix(iPoint, jPoint, block_weight);
        
      }
    }
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::ComputeILUPreconditioner(const TCSysVector<CalcType> & vec, TCSysVector<CalcType> & prod, CGeometry *geometry, CConfig *config) {
  
  unsigned long index;
  CalcType *Block_ij;
  long iPoint, jPoint;
  unsigned short iVar;
  
  /*--- Copy block matrix, note that the original matrix
   is modified by the algorithm---*/
  
  for (iPoint = 0; iPoint < (long)nPointDomain; iPoint++) {
    for (iVar = 0; iVar < nVar; iVar++) {
      prod[iPoint*nVar+iVar] = vec[iPoint*nVar+iVar];
    }
  }
  
  /*--- Forward solve the system using the lower matrix entries that
   were computed and stored during the ILU preprocessing. Note
   that we are overwriting the residual vector as we go. ---*/
  
  for (iPoint = 1; iPoint < (long)nPointDomain; iPoint++) {
    for (index = row_ptr_ilu[iPoint]; index < row_ptr_ilu[iPoint+1]; index++) {
      jPoint = col_ind_ilu[index];
      if ((jPoint < iPoint) && (jPoint < (long)nPointDomain)) {
        Block_ij = GetBlock_ILUMatrix(iPoint, jPoint);
        MatrixVectorProduct(Block_ij, &prod[jPoint*nVar], aux_vector);
        for (iVar = 0; iVar < nVar; iVar++)
          prod[iPoint*nVar+iVar] -= aux_vector[iVar];
        
      }
    }
  }
  
  /*--- Backwards substitution (starts at the last row) ---*/
  
  InverseDiagonalBlock_ILUMatrix((nPointDomain-1), block_inverse);
  MatrixVectorProduct(block_inverse, &prod[(nPointDomain-1)*nVar], aux_vector);
  
  for (iVar = 0; iVar < nVar; iVar++)
    prod[ (nPointDomain-1)*nVar + iVar] = aux_vector[iVar];
  
  for (iPoint = nPointDomain-2; iPoint >= 0; iPoint--) {
    for (iVar = 0; iVar < nVar; iVar++) sum_vector[iVar] = 0.0;
    for (index = row_ptr_ilu[iPoint]; index < row_ptr_ilu[iPoint+1]; index++) {
      jPoint = col_ind_ilu[index];
      if ((jPoint >= iPoint+1) && (jPoint < (long)nPointDomain)) {
        Block_ij = GetBlock_ILUMatrix(iPoint, jPoint);
        MatrixVectorProduct(Block_ij, &prod[jPoint*nVar], aux_vector);
        for (iVar = 0; iVar < nVar; iVar++) sum_vector[iVar] += aux_vector[iVar];
      }
    }
    for (iVar = 0; iVar < nVar; iVar++) prod[iPoint*nVar+iVar] = (prod[iPoint*nVar+iVar]-sum_vector[iVar]);
    InverseDiagonalBlock_ILUMatrix(iPoint, block_inverse);
    MatrixVectorProduct(block_inverse, &prod[iPoint*nVar], aux_vector);
    for (iVar = 0; iVar < nVar; iVar++) prod[iPoint*nVar+iVar] = aux_vector[iVar];
    if (iPoint == 0) break;
  }
  
  /*--- MPI Parallelization ---*/
  
  prod.SendReceive(geometry, config);
  
}

template<class CalcType>
unsigned long TCSysMatrix<CalcType>::ILU_Smoother(const TCSysVector<CalcType> & b, TCSysVector<CalcType> & x, TCMatrixVectorProduct<CalcType> & mat_vec, CalcType tol, unsigned long m, CalcType *residual, bool monitoring, CGeometry *geometry, CConfig *config) {
  
  unsigned long index;
  CalcType *Block_ij, omega = 1.0;
  long iPoint, jPoint;
  unsigned short iVar;
  
  /*---  Check the number of iterations requested ---*/
  
  if (m < 1) {
    char buf[100];
    SPRINTF(buf, "Illegal value for smoothing iterations, m = %lu", m );
    SU2_MPI::Error(string(buf), CURRENT_FUNCTION);
  }
  
  /*--- Create vectors to hold the residual and the Matrix-Vector product
   of the Jacobian matrix with the current solution (x^k). These must be
   stored in order to perform multiple iterations of the smoother. ---*/
  
  TCSysVector<CalcType> r(b);
  TCSysVector<CalcType> A_x(b);
  
  /*--- Calculate the initial residual, compute norm, and check
   if system is already solved. Recall, r holds b initially. ---*/
  
  mat_vec(x, A_x);
  r -= A_x;
  CalcType norm_r = r.norm();
  CalcType norm0  = b.norm();
  if ( (norm_r < tol*norm0) || (norm_r < eps) ) {
    if (rank == MASTER_NODE) cout << "TCSysMatrix<CalcType>::ILU_Smoother(): system solved by initial guess." << endl;
    return 0;
  }
  
  /*--- Set the norm to the initial initial residual value ---*/
  
  norm0 = norm_r;
  
  /*--- Output header information including initial residual ---*/
  
  int i = 0;
  if ((monitoring) && (rank == MASTER_NODE)) {
    cout << "\n# " << "ILU Smoother" << " residual history" << endl;
    cout << "# Residual tolerance target = " << tol << endl;
    cout << "# Initial residual norm     = " << norm_r << endl;
    cout << "     " << i << "     " << norm_r/norm0 << endl;
  }
  
  /*---  Loop over all smoothing iterations ---*/
  
  for (i = 0; i < (int)m; i++) {
    
    /*--- Forward solve the system using the lower matrix entries that
     were computed and stored during the ILU preprocessing. Note
     that we are overwriting the residual vector as we go. ---*/
    
    for (iPoint = 1; iPoint < (long)nPointDomain; iPoint++) {
      
      /*--- For each row (unknown), loop over all entries in A on this row
       row_ptr_ilu[iPoint+1] will have the index for the first entry on the next
       row. ---*/
      
      for (index = row_ptr_ilu[iPoint]; index < row_ptr_ilu[iPoint+1]; index++) {
        
        /*--- jPoint here is the column for each entry on this row ---*/
        
        jPoint = col_ind_ilu[index];
        
        /*--- Check that this column is in the lower triangular portion ---*/
        
        if ((jPoint < iPoint) && (jPoint < (long)nPointDomain)) {
          
          /*--- Lastly, get Aij*inv(Ajj) from the lower triangular part, which
           was calculated in the preprocessing, and apply to r. ---*/
          
          Block_ij = GetBlock_ILUMatrix(iPoint, jPoint);
          MatrixVectorProduct(Block_ij, &r[jPoint*nVar], aux_vector);
          for (iVar = 0; iVar < nVar; iVar++)
            r[iPoint*nVar+iVar] -= aux_vector[iVar];
          
        }
      }
    }
    
    /*--- Backwards substitution (starts at the last row) ---*/
    
    InverseDiagonalBlock_ILUMatrix((nPointDomain-1), block_inverse);
    MatrixVectorProduct(block_inverse, &r[(nPointDomain-1)*nVar], aux_vector);
    
    for (iVar = 0; iVar < nVar; iVar++)
      r[(nPointDomain-1)*nVar + iVar] = aux_vector[iVar];
    
    for (iPoint = nPointDomain-2; iPoint >= 0; iPoint--) {
      for (iVar = 0; iVar < nVar; iVar++) sum_vector[iVar] = 0.0;
      for (index = row_ptr_ilu[iPoint]; index < row_ptr_ilu[iPoint+1]; index++) {
        jPoint = col_ind_ilu[index];
        if ((jPoint >= iPoint+1) && (jPoint < (long)nPointDomain)) {
          Block_ij = GetBlock_ILUMatrix(iPoint, jPoint);
          MatrixVectorProduct(Block_ij, &r[jPoint*nVar], aux_vector);
          for (iVar = 0; iVar < nVar; iVar++) sum_vector[iVar] += aux_vector[iVar];
        }
      }
      for (iVar = 0; iVar < nVar; iVar++) r[iPoint*nVar+iVar] = (r[iPoint*nVar+iVar]-sum_vector[iVar]);
      InverseDiagonalBlock_ILUMatrix(iPoint, block_inverse);
      MatrixVectorProduct(block_inverse, &r[iPoint*nVar], aux_vector);
      for (iVar = 0; iVar < nVar; iVar++) r[iPoint*nVar+iVar] = aux_vector[iVar];
      if (iPoint == 0) break;
    }
    
    /*--- Update solution (x^k+1 = x^k + w*M^-1*r^k) using the residual vector,
     which holds the update after applying the ILU smoother, i.e., M^-1*r^k.
     Omega is a relaxation factor that we have currently set to 1.0. ---*/
    
    x.Plus_AX(omega, r);
    
    /*--- MPI Parallelization ---*/
    
    x.SendReceive(geometry, config);
    
    /*--- Update the residual (r^k+1 = b - A*x^k+1) with the new solution ---*/
    
    r = b;
    mat_vec(x, A_x);
    r -= A_x;
    
    /*--- Check if solution has converged, else output the relative 
     residual if necessary. ---*/
    
    norm_r = r.norm();
    if (norm_r < tol*norm0) break;
    if (((monitoring) && (rank == MASTER_NODE)) && ((i+1) % 5 == 0))
      cout << "     " << i << "     " << norm_r/norm0 << endl;
    
  }
  
  if ((monitoring) && (rank == MASTER_NODE)) {
    cout << "# ILU smoother final (true) residual:" << endl;
    cout << "# Iteration = " << i << ": |res|/|res0| = "  << norm_r/norm0 << ".\n" << endl;
  }
  
  return (unsigned int) i;
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::ComputeLU_SGSPreconditioner(const TCSysVector<CalcType> & vec, TCSysVector<CalcType> & prod, CGeometry *geometry, CConfig *config) {
  unsigned long iPoint, iVar;
  
  /*--- First part of the symmetric iteration: (D+L).x* = b ---*/
  
  for (iPoint = 0; iPoint < nPointDomain; iPoint++) {
    LowerProduct(prod, iPoint);                                        // Compute L.x*
    for (iVar = 0; iVar < nVar; iVar++)
      aux_vector[iVar] = vec[iPoint*nVar+iVar] - prod_row_vector[iVar]; // Compute aux_vector = b - L.x*
    Gauss_Elimination(iPoint, aux_vector);                            // Solve D.x* = aux_vector
    for (iVar = 0; iVar < nVar; iVar++)
      prod[iPoint*nVar+iVar] = aux_vector[iVar];                       // Assesing x* = solution
  }
  
  /*--- MPI Parallelization ---*/
  
  prod.SendReceive(geometry, config);
  
  /*--- Second part of the symmetric iteration: (D+U).x_(1) = D.x* ---*/
  
  for (iPoint = nPointDomain-1; (int)iPoint >= 0; iPoint--) {
    DiagonalProduct(prod, iPoint);                 // Compute D.x*
    for (iVar = 0; iVar < nVar; iVar++)
      aux_vector[iVar] = prod_row_vector[iVar];   // Compute aux_vector = D.x*
    UpperProduct(prod, iPoint);                    // Compute U.x_(n+1)
    for (iVar = 0; iVar < nVar; iVar++)
      aux_vector[iVar] -= prod_row_vector[iVar];  // Compute aux_vector = D.x*-U.x_(n+1)
    Gauss_Elimination(iPoint, aux_vector);        // Solve D.x* = aux_vector
    for (iVar = 0; iVar < nVar; iVar++)
      prod[iPoint*nVar + iVar] = aux_vector[iVar]; // Assesing x_(1) = solution
  }
  
  /*--- MPI Parallelization ---*/
  
  prod.SendReceive(geometry, config);
  
}

template<class CalcType>
unsigned long TCSysMatrix<CalcType>::LU_SGS_Smoother(const TCSysVector<CalcType> & b, TCSysVector<CalcType> & x, TCMatrixVectorProduct<CalcType> & mat_vec, CalcType tol, unsigned long m, CalcType *residual, bool monitoring, CGeometry *geometry, CConfig *config) {
  
  unsigned long iPoint, iVar;
  CalcType omega = 1.0;
  
  /*---  Check the number of iterations requested ---*/
  
  if (m < 1) {
    char buf[100];
    SPRINTF(buf, "Illegal value for smoothing iterations, m = %lu", m );
    SU2_MPI::Error(string(buf), CURRENT_FUNCTION);
  }
  
  /*--- Create vectors to hold the residual and the Matrix-Vector product
   of the Jacobian matrix with the current solution (x^k). These must be
   stored in order to perform multiple iterations of the smoother. ---*/
  
  TCSysVector<CalcType> r(b);
  TCSysVector<CalcType> A_x(b);
  TCSysVector<CalcType> xStar(x);
  
  /*--- Calculate the initial residual, compute norm, and check
   if system is already solved. Recall, r holds b initially. ---*/
  
  mat_vec(x, A_x);
  r -= A_x;
  CalcType norm_r = r.norm();
  CalcType norm0  = b.norm();
  if ( (norm_r < tol*norm0) || (norm_r < eps) ) {
    if (rank == MASTER_NODE) cout << "TCSysMatrix<CalcType>::LU_SGS_Smoother(): system solved by initial guess." << endl;
    return 0;
  }
  
  /*--- Set the norm to the initial initial residual value ---*/
  
  norm0 = norm_r;
  
  /*--- Output header information including initial residual ---*/
  
  int i = 0;
  if ((monitoring) && (rank == MASTER_NODE)) {
    cout << "\n# " << "LU_SGS Smoother" << " residual history" << endl;
    cout << "# Residual tolerance target = " << tol << endl;
    cout << "# Initial residual norm     = " << norm_r << endl;
    cout << "     " << i << "     " << norm_r/norm0 << endl;
  }
  
  /*---  Loop over all smoothing iterations ---*/
  
  for (i = 0; i < (int)m; i++) {

    /*--- First part of the symmetric iteration: (D+L).x* = b ---*/
    
    for (iPoint = 0; iPoint < nPointDomain; iPoint++) {
      LowerProduct(xStar, iPoint);                                      // Compute L.x*
      for (iVar = 0; iVar < nVar; iVar++)
        aux_vector[iVar] = r[iPoint*nVar+iVar] - prod_row_vector[iVar]; // Compute aux_vector = b - L.x*
      Gauss_Elimination(iPoint, aux_vector);                            // Solve D.x* = aux_vector
      for (iVar = 0; iVar < nVar; iVar++)
        xStar[iPoint*nVar+iVar] = aux_vector[iVar];                     // Assesing x* = solution, stored in r
    }
    
    /*--- MPI Parallelization ---*/
    
    xStar.SendReceive(geometry, config);
    
    /*--- Second part of the symmetric iteration: (D+U).x_(1) = D.x* ---*/
    
    for (iPoint = nPointDomain-1; (int)iPoint >= 0; iPoint--) {
      DiagonalProduct(xStar, iPoint);               // Compute D.x*
      for (iVar = 0; iVar < nVar; iVar++)
        aux_vector[iVar] = prod_row_vector[iVar];   // Compute aux_vector = D.x*
      UpperProduct(xStar, iPoint);                  // Compute U.x_(n+1)
      for (iVar = 0; iVar < nVar; iVar++)
        aux_vector[iVar] -= prod_row_vector[iVar];  // Compute aux_vector = D.x*-U.x_(n+1)
      Gauss_Elimination(iPoint, aux_vector);        // Solve D.x* = aux_vector
      for (iVar = 0; iVar < nVar; iVar++)
        xStar[iPoint*nVar+iVar] = aux_vector[iVar]; // Assesing x_(1) = solution
    }
    
    /*--- Update solution (x^k+1 = x^k + w*M^-1*r^k) using the xStar vector,
     which holds the update after applying the LU_SGS smoother, i.e., M^-1*r^k.
     Omega is a relaxation factor that we have currently set to 1.0. ---*/
    
    x.Plus_AX(omega, xStar);
    
    /*--- MPI Parallelization ---*/
    
    x.SendReceive(geometry, config);
    
    /*--- Update the residual (r^k+1 = b - A*x^k+1) with the new solution ---*/
    
    r = b;
    mat_vec(x, A_x);
    r -= A_x;
    xStar = x;
    
    /*--- Check if solution has converged, else output the relative
     residual if necessary. ---*/
    
    norm_r = r.norm();
    if (norm_r < tol*norm0) break;
    if (((monitoring) && (rank == MASTER_NODE)) && ((i+1) % 5 == 0))
      cout << "     " << i << "     " << norm_r/norm0 << endl;
    
  }
  
  if ((monitoring) && (rank == MASTER_NODE)) {
    cout << "# LU_SGS smoother final (true) residual:" << endl;
    cout << "# Iteration = " << i << ": |res|/|res0| = "  << norm_r/norm0 << ".\n" << endl;
  }
  
  return (unsigned int) i;
  
}

template<class CalcType>
unsigned short TCSysMatrix<CalcType>::BuildLineletPreconditioner(CGeometry *geometry, CConfig *config) {
  
//  bool *check_Point, add_point;
//  unsigned long iEdge, iPoint, jPoint, index_Point, iLinelet, iVertex, next_Point, counter, iElem;
//  unsigned short iMarker, iNode, ExtraLines = 100, MeanPoints;
//  CalcType alpha = 0.9, weight, max_weight, *normal, area, volume_iPoint, volume_jPoint;
//  unsigned long Local_nPoints, Local_nLineLets, Global_nPoints, Global_nLineLets;
  
//  /*--- Memory allocation --*/
  
//  check_Point = new bool [geometry->GetnPoint()];
//  for (iPoint = 0; iPoint < geometry->GetnPoint(); iPoint++)
//    check_Point[iPoint] = true;
  
//  LineletBool = new bool[geometry->GetnPoint()];
//  for (iPoint = 0; iPoint < geometry->GetnPoint(); iPoint ++)
//    LineletBool[iPoint] = false;
  
//  nLinelet = 0;
//  for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {
//    if ((config->GetMarker_All_KindBC(iMarker) == HEAT_FLUX              ) ||
//        (config->GetMarker_All_KindBC(iMarker) == ISOTHERMAL             ) ||
//        (config->GetMarker_All_KindBC(iMarker) == EULER_WALL             ) ||
//        (config->GetMarker_All_KindBC(iMarker) == DISPLACEMENT_BOUNDARY)) {
//      nLinelet += geometry->nVertex[iMarker];
//    }
//  }
  
//  /*--- If the domain contains well defined Linelets ---*/
  
//  if (nLinelet != 0) {
    
//    /*--- Basic initial allocation ---*/
    
//    LineletPoint = new vector<unsigned long>[nLinelet + ExtraLines];
    
//    /*--- Define the basic linelets, starting from each vertex ---*/
    
//    for (iMarker = 0; iMarker < config->GetnMarker_All(); iMarker++) {
//      if ((config->GetMarker_All_KindBC(iMarker) == HEAT_FLUX              ) ||
//          (config->GetMarker_All_KindBC(iMarker) == ISOTHERMAL             ) ||
//          (config->GetMarker_All_KindBC(iMarker) == EULER_WALL             ) ||
//          (config->GetMarker_All_KindBC(iMarker) == DISPLACEMENT_BOUNDARY)) {
//        iLinelet = 0;
//        for (iVertex = 0; iVertex < geometry->nVertex[iMarker]; iVertex++) {
//          iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
//          LineletPoint[iLinelet].push_back(iPoint);
//          check_Point[iPoint] = false;
//          iLinelet++;
//        }
//      }
//    }
    
//    /*--- Create the linelet structure ---*/
    
//    iLinelet = 0;
    
//    do {
      
//      index_Point = 0;
      
//      do {
        
//        /*--- Compute the value of the max weight ---*/
        
//        iPoint = LineletPoint[iLinelet][index_Point];
//        max_weight = 0.0;
//        for (iNode = 0; iNode < geometry->node[iPoint]->GetnPoint(); iNode++) {
//          jPoint = geometry->node[iPoint]->GetPoint(iNode);
//          if ((check_Point[jPoint]) && geometry->node[jPoint]->GetDomain()) {
//            iEdge = geometry->FindEdge(iPoint, jPoint);
//            normal = geometry->edge[iEdge]->GetNormal();
//            if (geometry->GetnDim() == 3) area = sqrt(normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2]);
//            else area = sqrt(normal[0]*normal[0]+normal[1]*normal[1]);
//            volume_iPoint = geometry->node[iPoint]->GetVolume();
//            volume_jPoint = geometry->node[jPoint]->GetVolume();
//            weight = 0.5*area*((1.0/volume_iPoint)+(1.0/volume_jPoint));
//            max_weight = max(max_weight, weight);
//          }
//        }
        
//        /*--- Verify if any face of the control volume must be added ---*/
        
//        add_point = false;
//        counter = 0;
//        next_Point = geometry->node[iPoint]->GetPoint(0);
//        for (iNode = 0; iNode < geometry->node[iPoint]->GetnPoint(); iNode++) {
//          jPoint = geometry->node[iPoint]->GetPoint(iNode);
//          iEdge = geometry->FindEdge(iPoint, jPoint);
//          normal = geometry->edge[iEdge]->GetNormal();
//          if (geometry->GetnDim() == 3) area = sqrt(normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2]);
//          else area = sqrt(normal[0]*normal[0]+normal[1]*normal[1]);
//          volume_iPoint = geometry->node[iPoint]->GetVolume();
//          volume_jPoint = geometry->node[jPoint]->GetVolume();
//          weight = 0.5*area*((1.0/volume_iPoint)+(1.0/volume_jPoint));
//          if (((check_Point[jPoint]) && (weight/max_weight > alpha) && (geometry->node[jPoint]->GetDomain())) &&
//              ((index_Point == 0) || ((index_Point > 0) && (jPoint != LineletPoint[iLinelet][index_Point-1])))) {
//            add_point = true;
//            next_Point = jPoint;
//            counter++;
//          }
//        }
        
//        /*--- We have arrived to an isotropic zone ---*/
        
//        if (counter > 1) add_point = false;
        
//        /*--- Add a typical point to the linelet, no leading edge ---*/
        
//        if (add_point) {
//          LineletPoint[iLinelet].push_back(next_Point);
//          check_Point[next_Point] = false;
//          index_Point++;
//        }
        
//      } while (add_point);
//      iLinelet++;
//    } while (iLinelet < nLinelet);
    
//    /*--- Identify the points that belong to a Linelet ---*/
    
//    for (iLinelet = 0; iLinelet < nLinelet; iLinelet++) {
//      for (iElem = 0; iElem < LineletPoint[iLinelet].size(); iElem++) {
//        iPoint = LineletPoint[iLinelet][iElem];
//        LineletBool[iPoint] = true;
//      }
//    }
    
//    /*--- Identify the maximum number of elements in a Linelet ---*/
    
//    max_nElem = LineletPoint[0].size();
//    for (iLinelet = 1; iLinelet < nLinelet; iLinelet++)
//      if (LineletPoint[iLinelet].size() > max_nElem)
//        max_nElem = LineletPoint[iLinelet].size();
    
//  }
  
//  /*--- The domain doesn't have well defined linelets ---*/
  
//  else {
    
//    max_nElem = 0;
    
//  }
  
//  /*--- Screen output ---*/
  
//  Local_nPoints = 0;
//  for (iLinelet = 0; iLinelet < nLinelet; iLinelet++) {
//    Local_nPoints += LineletPoint[iLinelet].size();
//  }
//  Local_nLineLets = nLinelet;
  
//#ifndef HAVE_MPI
//  Global_nPoints = Local_nPoints;
//  Global_nLineLets = Local_nLineLets;
//#else
//  SU2_MPI::Allreduce(&Local_nPoints, &Global_nPoints, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
//  SU2_MPI::Allreduce(&Local_nLineLets, &Global_nLineLets, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
//#endif
  
//  MeanPoints = SU2_TYPE::Int(CalcType(Global_nPoints)/CalcType(Global_nLineLets));
  
//  /*--- Memory allocation --*/
  
//  UBlock = new CalcType* [max_nElem];
//  invUBlock = new CalcType* [max_nElem];
//  LBlock = new CalcType* [max_nElem];
//  yVector = new CalcType* [max_nElem];
//  zVector = new CalcType* [max_nElem];
//  rVector = new CalcType* [max_nElem];
//  for (iElem = 0; iElem < max_nElem; iElem++) {
//    UBlock[iElem] = new CalcType [nVar*nVar];
//    invUBlock[iElem] = new CalcType [nVar*nVar];
//    LBlock[iElem] = new CalcType [nVar*nVar];
//    yVector[iElem] = new CalcType [nVar];
//    zVector[iElem] = new CalcType [nVar];
//    rVector[iElem] = new CalcType [nVar];
//  }
  
//  LFBlock = new CalcType [nVar*nVar];
//  LyVector = new CalcType [nVar];
//  FzVector = new CalcType [nVar];
  
//  /*--- Memory deallocation --*/
  
//  delete [] check_Point;
  
//  return MeanPoints;
  
  return 0;
}

template<class CalcType>
void TCSysMatrix<CalcType>::ComputeLineletPreconditioner(const TCSysVector<CalcType> & vec, TCSysVector<CalcType> & prod,
                                              CGeometry *geometry, CConfig *config) {
  
  unsigned long iVar, jVar, nElem = 0, iLinelet, im1Point, iPoint, ip1Point, iElem;
  long iElemLoop;
  CalcType *block;
  
  if (size == SINGLE_NODE) {
    
    /*--- Jacobi preconditioning if there is no linelet ---*/
    
    for (iPoint = 0; iPoint < nPointDomain; iPoint++) {
      if (!LineletBool[iPoint]) {
        for (iVar = 0; iVar < nVar; iVar++) {
          prod[(unsigned long)(iPoint*nVar+iVar)] = 0.0;
          for (jVar = 0; jVar < nVar; jVar++)
            prod[(unsigned long)(iPoint*nVar+iVar)] +=
            invM[(unsigned long)(iPoint*nVar*nVar+iVar*nVar+jVar)]*vec[(unsigned long)(iPoint*nVar+jVar)];
        }
      }
    }
    
    /*--- MPI Parallelization ---*/
    
    prod.SendReceive(geometry, config);
    
    /*--- Solve linelet using a Thomas' algorithm ---*/
    
    for (iLinelet = 0; iLinelet < nLinelet; iLinelet++) {
      
      nElem = LineletPoint[iLinelet].size();
      
      /*--- Copy vec vector to the new structure ---*/
      
      for (iElem = 0; iElem < nElem; iElem++) {
        iPoint = LineletPoint[iLinelet][iElem];
        for (iVar = 0; iVar < nVar; iVar++)
          rVector[iElem][iVar] = vec[(unsigned long)(iPoint*nVar+iVar)];
      }
      
      /*--- Initialization (iElem = 0) ---*/
      
      iPoint = LineletPoint[iLinelet][0];
      block = GetBlock(iPoint, iPoint);
      for (iVar = 0; iVar < nVar; iVar++) {
        yVector[0][iVar] = rVector[0][iVar];
        for (jVar = 0; jVar < nVar; jVar++)
          UBlock[0][iVar*nVar+jVar] = block[iVar*nVar+jVar];
      }
      
      /*--- Main loop (without iElem = 0) ---*/
      
      for (iElem = 1; iElem < nElem; iElem++) {
        
        im1Point = LineletPoint[iLinelet][iElem-1];
        iPoint = LineletPoint[iLinelet][iElem];
        
        InverseBlock(UBlock[iElem-1], invUBlock[iElem-1]);
        block = GetBlock(iPoint, im1Point); GetMultBlockBlock(LBlock[iElem], block, invUBlock[iElem-1]);
        block = GetBlock(im1Point, iPoint); GetMultBlockBlock(LFBlock, LBlock[iElem], block);
        block = GetBlock(iPoint, iPoint); GetSubsBlock(UBlock[iElem], block, LFBlock);
        
        /*--- Forward substituton ---*/
        
        GetMultBlockVector(LyVector, LBlock[iElem], yVector[iElem-1]);
        GetSubsVector(yVector[iElem], rVector[iElem], LyVector);
        
      }
      
      /*--- Backward substituton ---*/
      
      InverseBlock(UBlock[nElem-1], invUBlock[nElem-1]);
      GetMultBlockVector(zVector[nElem-1], invUBlock[nElem-1], yVector[nElem-1]);
      
      for (iElemLoop = nElem-2; iElemLoop >= 0; iElemLoop--) {
        iPoint = LineletPoint[iLinelet][iElemLoop];
        ip1Point = LineletPoint[iLinelet][iElemLoop+1];
        block = GetBlock(iPoint, ip1Point); GetMultBlockVector(FzVector, block, zVector[iElemLoop+1]);
        GetSubsVector(aux_vector, yVector[iElemLoop], FzVector);
        GetMultBlockVector(zVector[iElemLoop], invUBlock[iElemLoop], aux_vector);
      }
      
      /*--- Copy zVector to the prod vector ---*/
      
      for (iElem = 0; iElem < nElem; iElem++) {
        iPoint = LineletPoint[iLinelet][iElem];
        for (iVar = 0; iVar < nVar; iVar++)
          prod[(unsigned long)(iPoint*nVar+iVar)] = zVector[iElem][iVar];
      }
      
    }
    
    /*--- MPI Parallelization ---*/
    
    prod.SendReceive(geometry, config);
    
  }
  else {
    SU2_MPI::Error("Linelet not implemented in parallel.", CURRENT_FUNCTION);
  }
  
}

template<class CalcType>
void TCSysMatrix<CalcType>::ComputeResidual(const TCSysVector<CalcType> & sol, const TCSysVector<CalcType> & f, TCSysVector<CalcType> & res) {
  
  unsigned long iPoint, iVar;
  
  for (iPoint = 0; iPoint < nPointDomain; iPoint++) {
    RowProduct(sol, iPoint);
    for (iVar = 0; iVar < nVar; iVar++) {
      res[iPoint*nVar+iVar] = prod_row_vector[iVar] - f[iPoint*nVar+iVar];
    }
  }
  
}
