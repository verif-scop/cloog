
   /**-------------------------------------------------------------------**
    **                               CLooG                               **
    **-------------------------------------------------------------------**
    **                             matrix.c                              **
    **-------------------------------------------------------------------**
    **                    First version: april 17th 2005                 **
    **-------------------------------------------------------------------**/


/******************************************************************************
 *               CLooG : the Chunky Loop Generator (experimental)             *
 ******************************************************************************
 *                                                                            *
 * Copyright (C) 2005 Cedric Bastoul                                          *
 *                                                                            *
 * This is free software; you can redistribute it and/or modify it under the  *
 * terms of the GNU General Public License as published by the Free Software  *
 * Foundation; either version 2 of the License, or (at your option) any later *
 * version.                                                                   *
 *                                                                            *
 * This software is distributed in the hope that it will be useful, but       *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   *
 * for more details.                                                          *
 *                                                                            *
 * You should have received a copy of the GNU General Public License along    *
 * with software; if not, write to the Free Software Foundation, Inc.,        *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA                     *
 *                                                                            *
 * CLooG, the Chunky Loop Generator                                           *
 * Written by Cedric Bastoul, Cedric.Bastoul@inria.fr                         *
 *                                                                            *
 ******************************************************************************/
/* CAUTION: the english used for comments is probably the worst you ever read,
 *          please feel free to correct and improve it !
 */


# include <stdlib.h>
# include <stdio.h>
# include <ctype.h>
#include <cloog/polylib/cloog.h>


#define ALLOC(type) (type*)malloc(sizeof(type))
#define ALLOCN(type,n) (type*)malloc((n)*sizeof(type))


/******************************************************************************
 *                             Memory leaks hunting                           *
 ******************************************************************************/


/**
 * These functions and global variables are devoted to memory leaks hunting: we
 * want to know at each moment how many CloogMatrix structures are allocated
 * (cloog_matrix_allocated) and how many had been freed (cloog_matrix_freed).
 * Each time a CloogMatrix structure is allocated, a call to the function
 * cloog_matrix_leak_up() must be carried out, and respectively
 * cloog_matrix_leak_down() when a CloogMatrix structure is freed. The special
 * variable cloog_matrix_max gives the maximal number of CloogMatrix structures
 * simultaneously alive (i.e. allocated and non-freed) in memory.
 * - April 17th 2005: first version.
 */


int cloog_matrix_allocated = 0 ;
int cloog_matrix_freed = 0 ;
int cloog_matrix_max = 0 ;


static void cloog_matrix_leak_up()
{ cloog_matrix_allocated ++ ;
  if ((cloog_matrix_allocated-cloog_matrix_freed) > cloog_matrix_max)
  cloog_matrix_max = cloog_matrix_allocated - cloog_matrix_freed ;
}


static void cloog_matrix_leak_down()
{ cloog_matrix_freed ++ ;
}


/******************************************************************************
 *                              PolyLib interface                             *
 ******************************************************************************/


/**
 * CLooG makes an intensive use of matrix operations and the PolyLib do
 * the job. Here are the interfaces to all the PolyLib calls (CLooG uses 18
 * PolyLib functions), with or without some adaptations. If another matrix
 * library can be used, only these functions have to be changed.
 */


/**
 * cloog_matrix_print function:
 * This function prints the content of a CloogMatrix structure (matrix) into a
 * file (foo, possibly stdout).
 */
void cloog_matrix_print(FILE * foo, CloogMatrix * matrix)
{ Matrix_Print(foo,P_VALUE_FMT,matrix) ;
}


/**
 * cloog_matrix_free function:
 * This function frees the allocated memory for a CloogMatrix structure
 * (matrix).
 */
void cloog_matrix_free(CloogMatrix * matrix)
{ cloog_matrix_leak_down() ;
  Matrix_Free(matrix) ;
}

void cloog_constraints_free(CloogConstraints *constraints)
{
	cloog_matrix_free(constraints);
}

int cloog_constraints_contain_level(CloogConstraints *constraints,
			int level, int nb_parameters)
{
	return constraints->NbColumns - 2 - nb_parameters >= level;
}

/* Check if the variable at position level is defined by an
 * equality.  If so, return the row number.  Otherwise, return -1.
 *
 * If there is an equality, we can print it directly -no ambiguity-.
 * PolyLib can give more than one equality, we use just the first one
 * (this is a PolyLib problem, but all equalities are equivalent).
 */
int cloog_constraints_defining_equality(CloogConstraints *matrix, int level)
{
	int i;

	for (i = 0; i < matrix->NbRows; i++)
		if (value_zero_p(matrix->p[i][0]) &&
		    value_notzero_p(matrix->p[i][level]))
			return i;
	return -1;
}

/* Check if two vectors are eachothers opposite.
 * Return 1 if they are, 0 if they are not.
 */
static int Vector_Opposite(Value *p1, Value *p2, unsigned len)
{
	int i;

	for (i = 0; i < len; ++i) {
		if (value_abs_ne(p1[i], p2[i]))
			return 0;
		if (value_zero_p(p1[i]))
			continue;
		if (value_eq(p1[i], p2[i]))
			return 0;
	}
	return 1;
}

/* Check if the variable (e) at position level is defined by a
 * pair of inequalities
 *		 <a, i> + -m e +  <b, p> + k1 >= 0
 *		<-a, i> +  m e + <-b, p> + k2 >= 0
 * with 0 <= k1 + k2 < m
 * If so return the row number of the upper bound and set *lower
 * to the row number of the lower bound.  If not, return -1.
 *
 * If the variable at position level occurs in any other constraint,
 * then we currently return -1.  The modulo guard that we would generate
 * would still be correct, but we would also need to generate
 * guards corresponding to the other constraints, and this has not
 * been implemented yet.
 */
int cloog_constraints_defining_inequalities(CloogConstraints *matrix,
	int level, int *lower, int nb_par)
{
	int i, j, k;
	Value m;
	unsigned len = matrix->NbColumns - 2;
	unsigned nb_iter = len - nb_par;

	for (i = 0; i < matrix->NbRows; i++) {
		if (value_zero_p(matrix->p[i][level]))
			continue;
		if (value_zero_p(matrix->p[i][0]))
			return -1;
		if (value_one_p(matrix->p[i][level]))
			return -1;
		if (value_mone_p(matrix->p[i][level]))
			return -1;
		if (First_Non_Zero(matrix->p[i]+level+1,
				    (1+nb_iter)-(level+1)) != -1)
			return -1;
		for (j = i+1; j < matrix->NbRows; ++j) {
			if (value_zero_p(matrix->p[j][level]))
				continue;
			if (value_zero_p(matrix->p[j][0]))
				return -1;
			if (value_one_p(matrix->p[j][level]))
				return -1;
			if (value_mone_p(matrix->p[j][level]))
				return -1;
			if (First_Non_Zero(matrix->p[j]+level+1,
					    (1+nb_iter)-(level+1)) != -1)
				return -1;

			value_init(m);
			value_addto(m, matrix->p[i][1+len], matrix->p[j][1+len]);
			if (value_neg_p(m) ||
			    value_abs_ge(m, matrix->p[i][level])) {
				value_clear(m);
				return -1;
			}
			value_clear(m);

			if (!Vector_Opposite(matrix->p[i]+1, matrix->p[j]+1,
						len))
				return -1;
			for (k = j+1; k < matrix->NbRows; ++k)
				if (value_notzero_p(matrix->p[k][level]))
					return -1;
			if (value_pos_p(matrix->p[i][level])) {
				*lower = i;
				return j;
			} else {
				*lower = j;
				return i;
			}
		}
	}
	return -1;
}

int cloog_constraints_total_dimension(CloogConstraints *constraints)
{
	return constraints->NbColumns - 2;
}

/**
 * cloog_matrix_alloc function:
 * This function allocates the memory space for a CloogMatrix structure having
 * nb_rows rows and nb_columns columns, it set its elements to 0.
 */
CloogMatrix * cloog_matrix_alloc(unsigned nb_rows, unsigned nb_columns)
{ cloog_matrix_leak_up() ;
  return Matrix_Alloc(nb_rows,nb_columns) ;
}


/**
 * cloog_matrix_matrix function:
 * This function converts a PolyLib Matrix to a CloogMatrix structure.
 */
CloogMatrix * cloog_matrix_matrix(Matrix *matrix)
{
  cloog_matrix_leak_up();
  return matrix;
}


/******************************************************************************
 *                          Structure display function                        *
 ******************************************************************************/


/**
 * cloog_loop_print_structure function:
 * Displays a CloogMatrix structure (matrix) into a file (file, possibly stdout)
 * in a way that trends to be understandable without falling in a deep
 * depression or, for the lucky ones, getting a headache... It includes an
 * indentation level (level) in order to work with others print_structure
 * functions. Written by Olivier Chorier, Luc Marchaud, Pierre Martin and
 * Romain Tartiere.
 * - April 24th 2005: Initial version.
 * - June   2nd 2005: (Ced) Extraction from cloog_loop_print_structure and
 *                   integration in matrix.c.
 * - June  22rd 2005: Adaptation for GMP.
 */
void cloog_matrix_print_structure(FILE * file, CloogMatrix * matrix, int level)
{ int i, j ;
  
  /* Display the matrix. */
  for (i=0; i<matrix->NbRows; i++)
  { for(j=0; j<=level; j++)
    fprintf(file,"|\t") ;
      
    fprintf(file,"[ ") ;
      
    for (j=0; j<matrix->NbColumns; j++)
    { value_print(file,P_VALUE_FMT,matrix->p[i][j]) ;
      fprintf(file," ") ;
    }      

    fprintf(file,"]\n") ;
  }
}


/******************************************************************************
 *                               Reading function                             *
 ******************************************************************************/


/**
 * cloog_matrix_read function:
 * Adaptation from the PolyLib. This function reads a matrix into a file (foo,
 * posibly stdin) and returns a pointer this matrix.
 * October 18th 2001: first version.
 * - April 17th 2005: this function moved from domain.c to here.
 * - June  21rd 2005: Adaptation for GMP (based on S. Verdoolaege's version of
 *                    CLooG 0.12.1).
 */
CloogMatrix * cloog_matrix_read(FILE * foo)
{ unsigned NbRows, NbColumns ;
  int i, j, n ;
  char *c, s[MAX_STRING], str[MAX_STRING] ;
  CloogMatrix * matrix ;
  Value * p ;
  
  while (fgets(s,MAX_STRING,foo) == 0) ;
  while ((*s=='#' || *s=='\n') || (sscanf(s," %u %u",&NbRows,&NbColumns)<2))
  fgets(s,MAX_STRING,foo) ;
  
  matrix = cloog_matrix_alloc(NbRows,NbColumns) ;

  p = matrix->p_Init ;
  for (i=0;i<matrix->NbRows;i++) 
  { do 
    { c = fgets(s,MAX_STRING,foo) ;
      while ((c != NULL) && isspace(*c) && (*c != '\n'))
      c++ ;
    }
    while (c != NULL && (*c == '#' || *c == '\n'));
    
    if (c == NULL) 
    { fprintf(stderr, "[CLooG]ERROR: not enough rows.\n") ;
      exit(1) ;
    }
    for (j=0;j<matrix->NbColumns;j++) 
    { if (c == NULL || *c == '#' || *c == '\n')
      { fprintf(stderr, "[CLooG]ERROR: not enough columns.\n") ;
        exit(1) ;
      }
      /* NdCed : Dans le n ca met strlen(str). */
      if (sscanf(c,"%s%n",str,&n) == 0) 
      { fprintf(stderr, "[CLooG]ERROR: not enough rows.\n") ;
        exit(1) ;
      }
      value_read(*(p++),str) ;
      c += n ;
    }
  }
  
  return(matrix) ;
}


/******************************************************************************
 *                        Equalities spreading functions                      *
 ******************************************************************************/


/* Equalities are stored inside a CloogMatrix data structure called "equal".
 * This matrix has (nb_scattering + nb_iterators + 1) rows (i.e. total
 * dimensions + 1, the "+ 1" is because a statement can be included inside an
 * external loop without iteration domain), and (nb_scattering + nb_iterators +
 * nb_parameters + 2) columns (all unknowns plus the scalar plus the equality
 * type). The ith row corresponds to the equality "= 0" for the ith dimension
 * iterator. The first column gives the equality type (0: no equality, then
 * EQTYPE_* -see pprint.h-). At each recursion of pprint, if an equality for
 * the current level is found, the corresponding row is updated. Then the
 * equality if it exists is used to simplify expressions (e.g. if we have 
 * "i+1" while we know that "i=2", we simplify it in "3"). At the end of
 * the pprint call, the corresponding row is reset to zero.
 */

CloogEqualities *cloog_equal_alloc(int n, int nb_levels,
			int nb_parameters)
{
    int i;
    CloogEqualities *equal = ALLOC(CloogEqualities);

    equal->constraints = cloog_matrix_alloc(n, nb_levels + nb_parameters + 1);
    equal->types = ALLOCN(int, n);
    for (i = 0; i < n; ++i)
	equal->types[i] = EQTYPE_NONE;
    return equal;
}

void cloog_equal_free(CloogEqualities *equal)
{
    cloog_matrix_free(equal->constraints);
    free(equal->types);
    free(equal);
}

int cloog_equal_count(CloogEqualities *equal)
{
    return equal->constraints->NbRows;
}

CloogConstraints *cloog_equal_constraints(CloogEqualities *equal)
{
    return equal->constraints;
}


/**
 * cloog_constraint_equal_type function :
 * This function returns the type of the equality in the constraint (line) of
 * (constraints) for the element (level). An equality is 'constant' iff all
 * other
 * factors are null except the constant one. It is a 'pure item' iff one and
 * only one factor is non null and is 1 or -1. Otherwise it is an 'affine
 * expression'.
 * For instance:
 *   i = -13 is constant, i = j, j = -M are pure items,
 *   j = 2*M, i = j+1 are affine expressions.
 * When the equality comes from a 'one time loop', (line) is ONE_TIME_LOOP.
 * This case require a specific treatment since we have to study all the
 * constraints.
 * - constraints is the matrix of constraints,
 * - level is the column number in equal of the element which is 'equal to',
 * - line is the line number in equal of the constraint we want to study;
 *   if it is -1, all lines must be studied.
 **
 * - July     3rd 2002: first version, called pprint_equal_isconstant. 
 * - July     6th 2002: adaptation for the 3 types. 
 * - June    15th 2005: (debug) expr = domain->Constraint[line] was evaluated
 *                      before checking if line != ONE_TIME_LOOP. Since
 *                      ONE_TIME_LOOP is -1, an invalid read was possible.
 * - October 19th 2005: Removal of the once-time-loop specific processing.
 */
static int cloog_constraint_equal_type(CloogConstraints *constraints,
					int level, int line)
{ 
  int i, one=0 ;
  Value * expr ;
    
  expr = constraints->p[line] ;
  
  /* There is only one non null factor, and it must be +1 or -1 for
   * iterators or parameters.
   */ 
  for (i=1;i<=constraints->NbColumns-2;i++)
  if (value_notzero_p(expr[i]) && (i != level))
  { if ((value_notone_p(expr[i]) && value_notmone_p(expr[i])) || (one != 0))
    return EQTYPE_EXAFFINE ;
    else
    one = 1 ;
  }
  /* if the constant factor is non null, it must be alone. */
  if (one != 0)
  { if (value_notzero_p(expr[constraints->NbColumns-1]))
    return EQTYPE_EXAFFINE ;
  }
  else
  return EQTYPE_CONSTANT ;
  
  return EQTYPE_PUREITEM ;
}


int cloog_equal_type(CloogEqualities *equal, int level)
{
	return equal->types[level-1];
}


/**
 * cloog_equal_update function:
 * this function updates a matrix of equalities where each row corresponds to
 * the equality "=0" of an affine expression such that the entry at column
 * "row" (="level") is not zero. This matrix is upper-triangular, except the
 * row number "level-1" which has to be updated for the matrix to be triangular.
 * This function achieves the processing.
 * - equal is the matrix to be updated,
 * - level gives the row that has to be updated (it is actually row "level-1"),
 * - nb_par is the number of parameters of the program.
 **
 * - September 20th 2005: first version.
 */
static void cloog_equal_update(CloogEqualities *equal, int level, int nb_par)
{ int i, j ;
  Value gcd, factor_level, factor_outer, temp_level, temp_outer ;
  
  value_init_c(gcd) ;
  value_init_c(temp_level) ;
  value_init_c(temp_outer) ;
  value_init_c(factor_level) ;
  value_init_c(factor_outer) ;
  
  /* For each previous level, */
  for (i=level-2;i>=0;i--)
  { /* if the corresponding iterator is inside the current equality and is equal
     * to something,
     */
    if (value_notzero_p(equal->constraints->p[level-1][i+1]) && equal->types[i])
    { /* Compute the Greatest Common Divisor. */ 
      Gcd(equal->constraints->p[level-1][i+1],
	  equal->constraints->p[i][i+1], &gcd);
      
      /* Compute the factors to apply to each row vector element. */
      value_division(factor_level, equal->constraints->p[i][i+1], gcd);
      value_division(factor_outer, equal->constraints->p[level-1][i+1], gcd);
            
      /* Now update the row 'level'. */
      /* - the iterators, up to level, */
      for (j=1;j<=level;j++)
      { value_multiply(temp_level, factor_level,
			equal->constraints->p[level-1][j]);
        value_multiply(temp_outer, factor_outer, equal->constraints->p[i][j]);
        value_substract(equal->constraints->p[level-1][j], temp_level, temp_outer);
      }
      /* - between last useful iterator (level) and the first parameter, the
       *   matrix is sparse (full of zeroes), we just do nothing there. 
       * - the parameters and the scalar.
       */
      for (j=0;j<nb_par+1;j++)
      { value_multiply(temp_level,factor_level,
                       equal->constraints->p[level-1]
					    [equal->constraints->NbColumns-j-1]);
        value_multiply(temp_outer,factor_outer,
                       equal->constraints->p[i][equal->constraints->NbColumns-j-1]);
        value_substract(equal->constraints->p[level-1]
					     [equal->constraints->NbColumns-j-1],
	               temp_level,temp_outer) ;
      }
    }
  }
  
  /* Normalize (divide by GCD of all elements) the updated equality. */
  Vector_Normalize(&(equal->constraints->p[level-1][1]),
			equal->constraints->NbColumns-1);

  value_clear_c(gcd) ;
  value_clear_c(temp_level) ;
  value_clear_c(temp_outer) ;
  value_clear_c(factor_level) ;
  value_clear_c(factor_outer) ;
}


/**
 * cloog_equal_add function:
 * This function updates the row (level-1) of the equality matrix (equal) with
 * the row that corresponds to the row (line) of the matrix (matrix).
 * - equal is the matrix of equalities,
 * - matrix is the matrix of constraints,
 * - level is the column number in matrix of the element which is 'equal to',
 * - line is the line number in matrix of the constraint we want to study,
 * - the infos structure gives the user all options on code printing and more.
 **
 * - July     2nd 2002: first version. 
 * - October 19th 2005: Addition of the once-time-loop specific processing.
 */
void cloog_equal_add(CloogEqualities *equal, CloogConstraints *matrix,
			int level, int line, int nb_par)
{ 
  int i ;
  Value numerator, denominator, division, modulo ;

  /* If we are in the case of a loop running once, this means that the equality
   * comes from an inequality. Here we find this inequality.
   */
  if (line == ONE_TIME_LOOP)
  { for (i=0;i<matrix->NbRows;i++)
    if ((value_notzero_p(matrix->p[i][0]))&&
        (value_notzero_p(matrix->p[i][level])))
    { line = i ;
      
      /* Since in once-time-loops, equalities derive from inequalities, we
       * may have to offset the values. For instance if we have 2i>=3, the
       * equality is in fact i=2. This may happen when the level coefficient is
       * not 1 or -1 and the scalar value is not zero. In any other case (e.g.,
       * if the inequality is an expression including outer loop counters or
       * parameters) the once time loop would not have been detected
       * because of floord and ceild functions.
       */
      if (value_ne_si(matrix->p[i][level],1) &&
          value_ne_si(matrix->p[i][level],-1) &&
	  value_notzero_p(matrix->p[i][matrix->NbColumns-1]))
      { value_init_c(numerator) ;
        value_init_c(denominator) ;
        value_init_c(division) ;
        value_init_c(modulo) ;
        
	value_assign(denominator,matrix->p[i][level]) ;
	value_absolute(denominator,denominator) ; 
        value_assign(numerator,matrix->p[i][matrix->NbColumns-1]) ;   
        value_modulus(modulo,numerator,denominator) ;
        value_division(division,numerator,denominator) ;
	
	/* There are 4 scenarios:
	 *  di +n >= 0  -->  i + (n div d) >= 0
	 * -di +n >= 0  --> -i + (n div d) >= 0
	 *  di -n >= 0  -->  if (n%d == 0)  i + ((-n div d)+1) >= 0
	 *                   else           i +  (-n div d)    >= 0
	 * -di -n >= 0  -->  if (n%d == 0) -i + ((-n div d)-1) >= 0
	 *                   else          -i +  (-n div d)    >= 0
	 * In the following we distinct the scalar value setting and the
	 * level coefficient.
	 */
	if (value_pos_p(numerator) || value_zero_p(modulo))
	value_assign(matrix->p[i][matrix->NbColumns-1],division) ;
	else
	{ if (value_pos_p(matrix->p[i][level]))
	  value_increment(matrix->p[i][matrix->NbColumns-1],division) ;
	  else
	  value_decrement(matrix->p[i][matrix->NbColumns-1],division) ;
	}
        
	if (value_pos_p(matrix->p[i][level]))
	value_set_si(matrix->p[i][level],1) ;
	else
	value_set_si(matrix->p[i][level],-1) ;
	
	value_clear_c(numerator) ;
        value_clear_c(denominator) ;
        value_clear_c(division) ;
        value_clear_c(modulo) ;
      }
            
      break ;
    }
  }
  
  /* We update the line of equal corresponding to level:
   * - the first element gives the equality type,
   */
  equal->types[level-1] = cloog_constraint_equal_type(matrix, level, line);
  /* - the other elements corresponding to the equality itself
   *   (the iterators up to level, then the parameters and the scalar).
   */
  for (i=1;i<=level;i++)
      value_assign(equal->constraints->p[level-1][i], matrix->p[line][i]);
  for (i = 0; i < nb_par + 1; i++)
      value_assign(equal->constraints->p[level-1][equal->constraints->NbColumns-i-1],
		   matrix->p[line][matrix->NbColumns-i-1]);
  
  cloog_equal_update(equal, level, nb_par);
}


/**
 * cloog_equal_del function :
 * This function reset the equality corresponding to the iterator (level)
 * in the equality matrix (equal).
 * - July 2nd 2002: first version. 
 */
void cloog_equal_del(CloogEqualities *equal, int level)
{ 
    equal->types[level-1] = EQTYPE_NONE;
}



/******************************************************************************
 *                            Processing functions                            *
 ******************************************************************************/

/**
 * Function cloog_constraints_normalize:
 * This function will modify the constraint system in such a way that when
 * there is an equality depending on the element at level 'level', there are
 * no more (in)equalities depending on this element. For instance, try
 * test/valilache.cloog with options -f 8 -l 9, with and without the call
 * to this function. At a given moment, for the level L we will have
 * 32*P=L && L>=1 (P is a lower level), this constraint system cannot be
 * translated directly into a source code. Thus, we normalize the domain to
 * remove L from the inequalities. In our example, this leads to
 * 32*P=L && 32*P>=1, that can be transated to the code
 * if (P>=1) { L=32*P ; ... }. This function solves the DaeGon Kim bug.
 * WARNING: Remember that if there is another call to Polylib after a call to
 * this function, we have to recall this function.
 *  -June    16th 2005: first version (adaptation from URGent June-7th-2005 by
 *                      N. Vasilache).
 * - June    21rd 2005: Adaptation for GMP.
 * - November 4th 2005: Complete rewriting, simpler and faster. It is no more an
 *                      adaptation from URGent. 
 */
void cloog_constraints_normalize(CloogConstraints *matrix, int level)
{ int ref, i, j ;
  Value factor_i, factor_ref, temp_i, temp_ref, gcd ;
    
  if (matrix == NULL)
  return ;

  /* Don't "normalize" the constant term. */
  if (level == matrix->NbColumns-1)
    return;

  /* Let us find an equality for the current level that can be propagated. */
  for (ref=0;ref<matrix->NbRows;ref++)
  if (value_zero_p(matrix->p[ref][0]) && value_notzero_p(matrix->p[ref][level]))
  { value_init_c(gcd) ;
    value_init_c(temp_i) ;
    value_init_c(temp_ref) ;
    value_init_c(factor_i) ;
    value_init_c(factor_ref) ;
  
    /* Row "ref" is the reference equality, now let us find a row to simplify.*/
    for (i=ref+1;i<matrix->NbRows;i++)
    if (value_notzero_p(matrix->p[i][level]))
    { /* Now let us set to 0 the "level" coefficient of row "j" using "ref".
       * First we compute the factors to apply to each row vector element.
       */
      Gcd(matrix->p[ref][level],matrix->p[i][level],&gcd) ;
      value_division(factor_i,matrix->p[ref][level],gcd) ;
      value_division(factor_ref,matrix->p[i][level],gcd) ;
      
      /* Maybe we are simplifying an inequality: factor_i must not be <0. */
      if (value_neg_p(factor_i))
      { value_absolute(factor_i,factor_i) ;
        value_oppose(factor_ref,factor_ref) ;
      }
      
      /* Now update the vector. */
      for (j=1;j<matrix->NbColumns;j++)
      { value_multiply(temp_i,factor_i,matrix->p[i][j]) ;
        value_multiply(temp_ref,factor_ref,matrix->p[ref][j]) ;
        value_substract(matrix->p[i][j],temp_i,temp_ref) ;
      }
    
      /* Normalize (divide by GCD of all elements) the updated vector. */
      Vector_Normalize(&(matrix->p[i][1]),matrix->NbColumns-1) ;
    }
    
    value_clear_c(gcd) ;
    value_clear_c(temp_i) ;
    value_clear_c(temp_ref) ;
    value_clear_c(factor_i) ;
    value_clear_c(factor_ref) ;
    break ;
  }
}



/**
 * cloog_constraints_copy function:
 * this functions builds and returns a "hard copy" (not a pointer copy) of a
 * CloogMatrix data structure.
 * - October 26th 2005: first version.
 */
CloogConstraints *cloog_constraints_copy(CloogConstraints *matrix)
{ int i, j ;
  CloogMatrix * copy ;

  copy = cloog_matrix_alloc(matrix->NbRows,matrix->NbColumns) ;
  
  for (i=0;i<matrix->NbRows;i++)
  for (j=0;j<matrix->NbColumns;j++)
  value_assign(copy->p[i][j],matrix->p[i][j]) ;
  
  return copy ;
}


/**
 * cloog_matrix_vector_copy function:
 * this function rutuns a hard copy of the vector "vector" of length "length"
 * provided as input.
 * - November 3rd 2005: first version.
 */
static Value *cloog_matrix_vector_copy(Value *vector, int length)
{ int i ;
  Value * copy ;
  
  /* We allocate the memory space for the new vector, and we fill it with
   * the original coefficients.
   */
  copy = (Value *)malloc(length * sizeof(Value)) ;
  for (i=0;i<length;i++)
  { value_init_c(copy[i]) ;
    value_assign(copy[i],vector[i]) ;
  }
  
  return copy ;
}


/**
 * cloog_matrix_vector_free function:
 * this function clears the elements of a vector "vector" of length "length",
 * then frees the vector itself this is useful for the GMP version of CLooG
 * which has to clear all elements.
 * - October 29th 2005: first version.
 */
static void cloog_matrix_vector_free(Value * vector, int length)
{ int i ;
  
  for (i=0;i<length;i++)
  value_clear_c(vector[i]) ;
  free(vector) ;
}


/**
 * cloog_equal_vector_simplify function:
 * this function simplify an affine expression with its coefficients in
 * "vector" of length "length" thanks to an equality matrix "equal" that gives
 * for some elements of the affine expression an equality with other elements,
 * preferably constants. For instance, if the vector contains i+j+3 and the
 * equality matrix gives i=n and j=2, the vector is simplified to n+3 and is
 * returned in a new vector.
 * - vector is the array of affine expression coefficients
 * - equal is the matrix of equalities,
 * - length is the vector length,
 * - level is a level we don't want to simplify (-1 if none),
 * - nb_par is the number of parameters of the program.
 **
 * - September 20th 2005: first version.
 * - November   2nd 2005: (debug) we are simplifying inequalities, thus we are
 *                        not allowed to multiply the vector by a negative
 *                        constant.Problem found after a report of Michael
 *                        Classen.
 */
Value *cloog_equal_vector_simplify(CloogEqualities *equal, Value *vector,
				    int length, int level, int nb_par)
{ int i, j ;
  Value gcd, factor_vector, factor_equal, temp_vector, temp_equal, * simplified;
  
  simplified = cloog_matrix_vector_copy(vector,length) ;
  
  value_init_c(gcd) ;
  value_init_c(temp_vector) ;
  value_init_c(temp_equal) ;
  value_init_c(factor_vector) ;
  value_init_c(factor_equal) ;
    
  /* For each non-null coefficient in the vector, */
  for (i=length-nb_par-2;i>0;i--)
  if (i != level)
  { /* if the coefficient in not null, and there exists a useful equality */
    if ((value_notzero_p(simplified[i])) && equal->types[i-1])
    { /* Compute the Greatest Common Divisor. */ 
      Gcd(simplified[i], equal->constraints->p[i-1][i], &gcd);
      
      /* Compute the factors to apply to each row vector element. */
      value_division(factor_vector, equal->constraints->p[i-1][i], gcd);
      value_division(factor_equal,simplified[i],gcd) ;
      
      /* We are simplifying an inequality: factor_vector must not be <0. */
      if (value_neg_p(factor_vector))
      { value_absolute(factor_vector,factor_vector) ;
        value_oppose(factor_equal,factor_equal) ;
      }
      
      /* Now update the vector. */
      /* - the iterators, up to the current level, */
      for (j=1;j<=length-nb_par-2;j++)
      { value_multiply(temp_vector,factor_vector,simplified[j]) ;
        value_multiply(temp_equal, factor_equal, equal->constraints->p[i-1][j]);
        value_substract(simplified[j],temp_vector,temp_equal) ;
      }
      /* - between last useful iterator (i) and the first parameter, the equal
       *   matrix is sparse (full of zeroes), we just do nothing there. 
       * - the parameters and the scalar.
       */
      for (j=0;j<nb_par+1;j++)
      { value_multiply(temp_vector,factor_vector,simplified[length-1-j]) ;
        value_multiply(temp_equal,factor_equal,
		 equal->constraints->p[i-1][equal->constraints->NbColumns-j-1]);
        value_substract(simplified[length-1-j],temp_vector,temp_equal) ;
      }
    }
  }
  
  /* Normalize (divide by GCD of all elements) the updated vector. */
  Vector_Normalize(&(simplified[1]),length-1) ;

  value_clear_c(gcd) ;
  value_clear_c(temp_vector) ;
  value_clear_c(temp_equal) ;
  value_clear_c(factor_vector) ;
  value_clear_c(factor_equal) ;
  
  return simplified ;
}


/**
 * cloog_constraints_simplify function:
 * this function simplify all constraints inside the matrix "matrix" thanks to
 * an equality matrix "equal" that gives for some elements of the affine
 * constraint an equality with other elements, preferably constants.
 * For instance, if a row of the matrix contains i+j+3>=0 and the equality
 * matrix gives i=n and j=2, the constraint is simplified to n+3>=0. The
 * simplified constraints are returned back inside a new simplified matrix.
 * - matrix is the set of constraints to simplify,
 * - equal is the matrix of equalities,
 * - level is a level we don't want to simplify (-1 if none),
 * - nb_par is the number of parameters of the program.
 **
 * - November 4th 2005: first version.
 */
CloogConstraints *cloog_constraints_simplify(CloogConstraints *matrix,
	CloogEqualities *equal, int level, int nb_par)
{ int i, j, k ;
  Value * vector ;
  CloogMatrix * simplified ;
  
  if (matrix == NULL)
  return NULL ;
  
  /* The simplified matrix is such that each row has been simplified thanks
   * tho the "equal" matrix. We allocate the memory for the simplified matrix,
   * then for each row of the original matrix, we compute the simplified
   * vector and we copy its content into the according simplified row.
   */
  simplified = cloog_matrix_alloc(matrix->NbRows,matrix->NbColumns) ;
  for (i=0;i<matrix->NbRows;i++)
  { vector = cloog_equal_vector_simplify(equal, matrix->p[i],
					  matrix->NbColumns, level, nb_par);
    for (j=0;j<matrix->NbColumns;j++)
    value_assign(simplified->p[i][j],vector[j]) ;
    
    cloog_matrix_vector_free(vector,matrix->NbColumns) ;
  }
  
  /* After simplification, it may happen that few constraints are the same,
   * we remove them here by replacing them with 0=0 constraints.
   */
  for (i=0;i<simplified->NbRows;i++)
  for (j=i+1;j<simplified->NbRows;j++)
  { for (k=0;k<simplified->NbColumns;k++)
    if (value_ne(simplified->p[i][k],simplified->p[j][k]))
    break ;
    
    if (k == matrix->NbColumns)
    { for (k=0;k<matrix->NbColumns;k++)
      value_set_si(simplified->p[j][k],0) ;
    }
  }
  
  return simplified ;
}


int cloog_constraints_count(CloogConstraints *constraints)
{
	return constraints->NbRows;
}

/**
 * Return true if constraint c involves variable v (zero-based).
 */
int cloog_constraint_involves(CloogConstraints *constraints, int c, int v)
{
	return value_notzero_p(constraints->p[c][1+v]);
}

int cloog_constraint_is_lower_bound(CloogConstraints *constraints, int c, int v)
{
	return value_pos_p(constraints->p[c][1+v]);
}

int cloog_constraint_is_upper_bound(CloogConstraints *constraints, int c, int v)
{
	return value_neg_p(constraints->p[c][1+v]);
}

int cloog_constraint_is_equality(CloogConstraints *constraints, int c)
{
	return value_zero_p(constraints->p[c][0]);
}

void cloog_constraint_clear(CloogConstraints *constraints, int c)
{
	int k;

	for (k = 1; k <= constraints->NbColumns - 2; k++)
		value_set_si(constraints->p[c][k], 0);
}

void cloog_constraint_coefficient_get(CloogConstraints *constraints,
			int c, int var, Value *val)
{
	value_assign(*val, constraints->p[c][1+var]);
}

void cloog_constraint_coefficient_set(CloogConstraints *constraints,
			int c, int var, Value val)
{
	value_assign(constraints->p[c][1+var], val);
}

void cloog_constraint_constant_get(CloogConstraints *constraints,
			int c, Value *val)
{
	value_assign(*val, constraints->p[c][constraints->NbColumns-1]);
}

/**
 * Copy the coefficient of constraint c into dst in PolyLib order,
 * i.e., first the coefficients of the variables, then the coefficients
 * of the parameters and finally the constant.
 */
void cloog_constraint_copy(CloogConstraints *constraints, int c, Value *dst)
{
	Vector_Copy(constraints->p[c]+1, dst, constraints->NbColumns-1);
}