///////////////////////////////////////////////////////////////////////////////
// FILE 'utility_functions.cpp'
//
//'Matrix' class member functions
// Module utility functions:
//	angle
//	sign
//	mat2tr
//	mat3tr
//	cad_distance
//	cad_geo84_in
//	cad_geo84vel_in
//	cad_geoc_in
//	cad_geoc_ine
//	cad_grav84
//	cad_in_geo84
//	cad_in_geoc
//	cad_in_orb
//	cad_kepler (based on Morth)
//	cad_kepler1 (based on Bate, Mueller & White)
//	cad_orb_in
//	cad_tdi84
//	cad_tei
//	cad_tge
//	cad_tgi84
//	cad_tip
// Stochastic functions
//	exponential
//	gauss
//	markov
//	rayleigh
//	uniform
//	unituni
// Table look-up
// Integration
// US76 Atmosphere
// US76 Atmosphere extended to 1000km (NASA Marshall)
//
//010628 Created by Peter H Zipfel
//020723 In 'markov' replaced static variable by '&value_saved', PZi
//020829 Dynamically dimensioned utilities, PZi
//030319 Added US76 atmosphere, PZi
//030411 WGS84 utilities, PZi
//030519 Overloaded operator [] for vector of type 'Matrix', PZi
//030717 Improved table look-up, PZi
//040311 US76 Atmosphere extended to 1000km (NASA Marshall), PZi
//040319 Kepler utility, PZi
//040326 Unit vector cross product operator%, PZi
//040510 Added cad_in_orb, cad_orb_in, cad_tip, PZi
//050202 Simplified and renamed 'integrate(...)' to Modified Euler method, PZi 
///////////////////////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_DEPRECATE
#include <fstream>
#include <cmath>
#include <iostream>
#include <iomanip>
#include "utility_header.hpp"
#include "global_header.hpp"

///////////////////////////////////////////////////////////////////////////////
/////////////////////// 'Matrix' member functions /////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//One dimensional and two dimensional arrays of any dimension of type 'double'
//
//020826 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
//Constructors
///////////////////////////////////////////////////////////////////////////////
Matrix::Matrix(){}

Matrix::Matrix(int row_size,int col_size)
{

   num_row=row_size;
   num_col=col_size;

   pbody=NULL;

   //allocating memory
   num_elem=row_size*col_size;
   pbody=new double[num_elem];
   if(pbody==0){cerr<<" *** Error: matrix memory allocation failed ***\n";system("pause");exit(1);}

   //initializing array to zero
   for(int i=0;i<num_elem;i++)
   {
      *(pbody+i)=0.;
   }

}

Matrix::Matrix(const Matrix &MAT)
{
//	cout<<" >>> copy constructing >>>\n";

          num_row=MAT.num_row;
          num_col=MAT.num_col;
          num_elem=MAT.num_elem;
          pbody=new double[num_elem];
          if(pbody==0){cerr<<" *** Error: matrix memory allocation failed ***\n";system("pause");exit(1);}

          //copying
          for(int i=0;i<num_elem;i++)
                    *(pbody+i)=(*(MAT.pbody+i));
}

///////////////////////////////////////////////////////////////////////////////
//Destructor
///////////////////////////////////////////////////////////////////////////////
Matrix::~Matrix()
{
//	cout<<" <<< destructing <<<\n";
          delete [] pbody;
}	


///////////////////////////////////////////////////////////////////////////////
//Printing matrix to console
//Example: MAT.print();
///////////////////////////////////////////////////////////////////////////////
void Matrix::print()
{

   double *pmem=pbody;
   //outside loop rows, inside loop columns
   for(int i=0;i<num_row;i++)
   {
      for(int j=0;j<num_col;j++)
      {
         cout << std::setprecision(10) <<*pbody<<"\t";
         pbody++;
      }
      cout<<'\n';
   }
   //resetting pointer
   pbody=pmem;
   cout<<"\n\n";

}

///////////////////////////////////////////////////////////////////////////////
//Absolute value of vector
//Example: avalue = VEC.absolute();
///////////////////////////////////////////////////////////////////////////////
double Matrix::absolute() 
{
          if(num_row>1&&num_col>1){cerr<<" *** Warning: not a vector in 'Matrix::absolute()' *** \n";}
          double ret=0.0;
          
          for(int i=0;i<num_elem;i++) 
                    ret+=(*(pbody+i))*(*(pbody+i));
          ret=sqrt(ret);

          return ret;
}

///////////////////////////////////////////////////////////////////////////////
//Adjoint matrix (same as determinant procedure however the matrix element
//is NOT multiplied into each cofactor)
//Example: BMAT = AMAT.adjoint();
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::adjoint()
{
          if(!(num_row==num_col))
          {cerr<<" *** Error: matrix not square in 'Matrix::adjoint()' *** \n";system("pause");exit(1);}
          if((num_row==1)&&(num_col==1))
          {cerr<<" *** Error: only one element in 'Matrix::adjoint()' *** \n";system("pause");exit(1);}

          Matrix RESULT(num_row,num_col);

          for(int i=0;i<num_elem;i++){
                    //row #
                    int row=i/num_col+1;
                    //column #
                    int col=i%num_col+1;

                    if (((row+col)%2)==0)
                              *(RESULT.pbody+i)=sub_matrix(row,col).determinant();
                    else
                              *(RESULT.pbody+i)=(-1.0)*sub_matrix(row,col).determinant();
          }
          return RESULT.trans();
}

//////////////////////////////////////////////////////////////////////////////
//Assigns a value to a matrix element (offset!)
//Example: MAT.assign_loc(r,c,val); ((r+1)th-row, (c+1)th-col)
///////////////////////////////////////////////////////////////////////////////
void Matrix::assign_loc(const int &r, const int &c, const double &val)
{
          if(r>num_row-1||c>num_col-1)
          {cerr<<" *** Error: location outside array in 'Matrix::assign_loc()' *** \n";system("pause");exit(1);}

          //assigning value
          int offset=num_col*(r)+c;
          *(pbody+offset)=val;	
}

///////////////////////////////////////////////////////////////////////////////
//Builds a 3x1 vector from three parameters
//Example: VEC.build_vec3(v1,v2,v3);
///////////////////////////////////////////////////////////////////////////////
Matrix & Matrix::build_vec3(const double &v1,const double &v2,const double &v3)
{
          num_row=3;
          num_col=1;
          *pbody=v1;
          *(pbody+1)=v2;
          *(pbody+2)=v3;

          return *this;
}

///////////////////////////////////////////////////////////////////////////////
//Builds a 3x3 matrix from nine paramters arranged in rows
//Example: MAT.build_mat33(v11,v12,v13,v21,v22,v23,v31,v32,v33);
///////////////////////////////////////////////////////////////////////////////
Matrix & Matrix::build_mat33(const double &v11,const double &v12,const double &v13
                                                             ,const double &v21,const double &v22,const double &v23
                                                       ,const double &v31,const double &v32,const double &v33)
{
          num_row=3;
          num_col=3;
          *pbody=v11;    *(pbody+1)=v12;*(pbody+2)=v13;
          *(pbody+3)=v21;*(pbody+4)=v22;*(pbody+5)=v23;
          *(pbody+6)=v31;*(pbody+7)=v32;*(pbody+8)=v33;

          return *this;
}

///////////////////////////////////////////////////////////////////////////////
//Calulates Cartesian from polar coordinates
//|V1|             | cos(elevation)*cos(azimuth)|
//|V2| = magnitude*|cos(elevation)*sin(azimuth) |
//|V3|		       |	  -sin(elevation)       |
//
//Example: VEC.cart_from_pol(magnitude,azimuth,elevation); 	
///////////////////////////////////////////////////////////////////////////////
Matrix & Matrix::cart_from_pol(const double &magnitude,const double &azimuth
                                                               ,const double &elevation)
{
          *pbody=magnitude*(cos(elevation)*cos(azimuth));
          *(pbody+1)=magnitude*(cos(elevation)*sin(azimuth));
          *(pbody+2)=magnitude*(sin(elevation)*(-1.0));

          return *this;
}

///////////////////////////////////////////////////////////////////////////////
//Returns column vector of column # 
//Example: VEC = MAT.col_vec(2); (2nd column!)
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::col_vec(const int &col)
{
          if(col<=0||col>num_col)
          {cerr<<" *** Error: column outside array in 'Matrix::col_vec()' *** \n";system("pause");exit(1);}
          
          Matrix RESULT(num_row,1);

          for(int i=0;i<num_row;i++){
                    int offset=i*num_col+col-1;
                    *(RESULT.pbody+i)=(*(pbody+offset));
          }
          return RESULT;
}

///////////////////////////////////////////////////////////////////////////////
//Returns the determinant		
//Determinant recursive procedure
//Example: det = MAT.determinant();
///////////////////////////////////////////////////////////////////////////////
double Matrix::determinant()
{
          if(!(num_row==num_col))
          {cerr<<" *** Error: matrix not square in 'Matrix::determinant()' *** \n";system("pause");exit(1);}
          
          double result=0.0;

          //base case of a single matrix element
          if ((num_col==1)&&(num_row==1))
                    return *pbody;

          //second base case of a 2x2 matrix
          else if ((num_col==2)&&(num_row==2))
                    return (*pbody)*(*(pbody+3))-(*(pbody+1))*(*(pbody+2));

          else
          {
                    for(int j=0;j<num_col;j++)
                    {
                              //use cofactors and submatricies to finish for nxn
                              if ((j%2)==0)
                              {
                                        //odd column (numbered!)
                                        result+=sub_matrix(1,j+1).determinant()*(*(pbody+j));
                              }
                                        else
                              {
                                        //even column (numbered!)
                                        result+=(-1.0)*sub_matrix(1,j+1).determinant()*(*(pbody+j));
                              }
                    }
          }	
          return result;
}

///////////////////////////////////////////////////////////////////////////////
//Returns nxn diagonal matrix  from nx1 vector 
//Example: DIAMAT=VEC.diamat_vec()
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::diamat_vec()
{
          if(num_col!=1)
          {cerr<<" *** Error: not a vector in 'Matrix::diagmat_vec()' *** \n";system("pause");exit(1);}

          Matrix RESULT(num_row,num_row);
          for(int i=0;i<num_row;i++){
                    int offset=i*num_row+i;
                    *(RESULT.pbody+offset)=(*(pbody+i));
          }
          return RESULT;
}

///////////////////////////////////////////////////////////////////////////////
//Returns nx1 diagonal vector from nxn matrix
//Example: VEC=MAT.diavec_mat();
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::diavec_mat()
{
          if(!(num_row==num_col))
          {cerr<<" *** Error: matrix not square in 'Matrix::diavec_mat()' *** \n";system("pause");exit(1);}
          
          Matrix RESULT(num_row,1);
          for(int i=0;i<num_row;i++){
                    int offset=i*num_row+i;
                    *(RESULT.pbody+i)=(*(pbody+offset));
          }
          return RESULT;
}

///////////////////////////////////////////////////////////////////////////////
//Dimensions a matrix of size row x col
//Only used to initialize arrays in class 'Variable'
//Example: MAT.dimension(3,3);
///////////////////////////////////////////////////////////////////////////////
void Matrix::dimension(int row,int col)
{
          num_row=row;
          num_col=col;

          pbody=NULL;

          //allocating memory
          num_elem=row*col;
          pbody=new double[num_elem];
          if(pbody==0){cerr<<" *** Error: memory allocation failed 'Matrix::dimension()' ***\n";system("pause");exit(1);}

          //initializing array to zero
          for(int i=0;i<num_elem;i++)
                    *(pbody+i)=0.;
}
///////////////////////////////////////////////////////////////////////////////
//Bi-variate ellipse
//calculating major and minor semi-axes of ellipse and rotation angle 
//    from the symmetrical pos semi-definite MAT(2x2) matrix
//coordinate axes orientation:
//          ^ 1-axis
//          |
//          |
//          |---> 2-axis
//
//angle is measured from 1st coordinate axis to the right
//
//major_semi_axis = ELLIPSE.get_loc(0,0);
//minor_semi_axis = ELLIPSE.get_loc(1,0);
//angle      = ELLIPSE.get_loc(2,0);
//
//Example: ELLIPSE = MAT.ellipse();
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::ellipse()
{
          Matrix ELLIPSE(3,1);
          double dum(0);
          double dum1(0);
          double dum2(0);
          double phi(0);
          double ak1(0);
          double ak2(0);
          Matrix X1V(2,1); //major principal axes of ellipse 
          Matrix X2V(2,1); //minor principal axes of ellipse 

          double a11=*pbody;
          double a22=*(pbody+3);
          double a12=*(pbody+1);
          double a1122=a11+a22;
          double aq1122=a1122*a1122;
          dum1=aq1122-4.*(a11*a22-a12*a12);
          if(dum1>=0.)dum2=sqrt(dum1);

          //major and minor semi-axes of ellipse
          double ama=(a1122+dum2)/2.;
          double ami=(a1122-dum2)/2.;
          ELLIPSE.assign_loc(0,0,ama);
          ELLIPSE.assign_loc(1,0,ami);
          if(ama==ami)return ELLIPSE;

          //angle of orientation of major axis wrt first principal axis
          if(a11-ama!=0.){
         dum1=-a12/(a11-ama);
         ak1=sqrt(1./(1.+dum1*dum1));
         X1V.assign_loc(0,0,dum1*ak1);
         X1V.assign_loc(1,0,ak1);
         dum=dum1*ak1;
         if(fabs(dum)>1.) dum=1.*sign(dum);
         phi=acos(dum);
                     ELLIPSE.assign_loc(2,0,phi);
          }
          else{
         dum1=-a12/(a22-ama);
         ak1=sqrt(1./(1.+dum1*dum1));
         X1V.assign_loc(0,0,ak1);
         X1V.assign_loc(1,0,dum1*ak1);
         if(fabs(ak1)>1.) ak1=1.*sign(ak1);
         phi=acos(ak1);
                     ELLIPSE.assign_loc(2,0,phi);
          }
          //second principal axis - not used
          if(a11-ami!=0.){
         dum2=-a12/(a11-ami);
         ak2=sqrt(1./(1.+dum2*dum2));
         X2V.assign_loc(0,0,dum2*ak2);
         X2V.assign_loc(1,0,ak2);
          }
          else{
         dum2=-a12/(a22-ami);
         ak2=sqrt(1./(1.+dum2*dum2));
         X2V.assign_loc(0,0,ak2);
         X2V.assign_loc(1,0,dum2*ak2);
          }
          return ELLIPSE;
}

///////////////////////////////////////////////////////////////////////////////
//Returns the number of columns of matrix MAT
//Example: nc = MAT.get_cols();
///////////////////////////////////////////////////////////////////////////////
int Matrix::get_cols()
{
          return num_col;
}

///////////////////////////////////////////////////////////////////////////////
//Returns offset-index given row# and col#
//Example: i = MAT.get_index(2,3); (2nd row, 3rd column)
//////////////////////////////////////////////////////////////////////////////
int Matrix::get_index(const int &row, const int &col)
{
          int index;
                    index=(row-1)*num_col+col-1;
          return index;
}

///////////////////////////////////////////////////////////////////////////////
//Returns the value at offset-row 'r' offset-col 'c' of MAT
//Example: value = MAT.get_loc(2,1); (3rd row, 2nd column)
///////////////////////////////////////////////////////////////////////////////
double Matrix::get_loc(const int &r,const int &c)
{
          if((r<num_row)&&(c<num_col))
                    return *(pbody+r*num_col+c);		
          else
          {
                    {cout<<" *** Error: invalid matrix location,'Matrix::get_loc()' *** \n";system("pause");exit(1);}
                    return 0.0;
          }
}

///////////////////////////////////////////////////////////////////////////////
//Returns the number of rows in the matrix
//Example: nr = MAT.get_rows()
///////////////////////////////////////////////////////////////////////////////
int Matrix::get_rows()
{
          return num_row;
}

///////////////////////////////////////////////////////////////////////////////
//Returns the pointer to MAT
//Example: ptr = MAT.get_pbody();
///////////////////////////////////////////////////////////////////////////////
double * Matrix::get_pbody()
{
          return pbody;
}

///////////////////////////////////////////////////////////////////////////////
//Builds a square identity matrix of object 'Matrix MAT'
//Example:	MAT.identity();
///////////////////////////////////////////////////////////////////////////////
Matrix & Matrix::identity()
{
          if (num_row==num_col)
          {
                    for(int r=0;r<num_row;r++)
                              *(pbody+r*num_row+r)=1.;
          }
          else
          {cout<<" *** Error: matrix not square 'Matrix::identiy()'*** \n";system("pause");exit(1);}

          return *this;
}

///////////////////////////////////////////////////////////////////////////////
//Returns the inverse of a square matrix AMAT 
//Inversion  INVERSE =(1/det(A))*Adj(A)
//Example: INVERSE = AMAT.inverse();
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::inverse()
{
          if (num_col!=num_row)
          {cerr<<" *** Error: not a square matrix 'Matrix::inverse()' *** \n";system("pause");exit(1);}

          Matrix RESULT(num_row,num_col);
          double d=0.;

          d=determinant();
          if (d==0.)
          {cerr<<" *** Error: singular! 'Matrix::inverse()' *** \n";
          exit(1);}

          d=1./d;
          RESULT=adjoint();
          RESULT=RESULT*d;

          return RESULT;
}
///////////////////////////////////////////////////////////////////////////////
//Returns 3x3 matrix row-wise from 9x1 vector
//Example: MAT=VEC.	mat33_vec9();
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::mat33_vec9()
{
          if(!(num_row==9 && num_col==1))
          {cerr<<" *** Error: vector not 9 x 1 'Matrix::mat33_vec9()' *** \n";system("pause");exit(1);}
          
          Matrix RESULT(3,3);
          for(int i=0;i<9;i++){
                    *(RESULT.pbody+i)=*(pbody+i);
          }
          return RESULT;
}

///////////////////////////////////////////////////////////////////////////////
//Forms  matrix MAT with all elements '1.' from object MAT(num_row,num_col)
//Example: MAT.ones();
///////////////////////////////////////////////////////////////////////////////
Matrix & Matrix::ones()
{
          for(int r=0;r<num_elem;r++)
                    *(pbody+r)=1.;

          return *this;
}

///////////////////////////////////////////////////////////////////////////////
//Inequality relational operator, returns true or false 
//returns true if elements differ by more than EPS
//Example: if(AMAT!=BMAT){...};
///////////////////////////////////////////////////////////////////////////////
bool Matrix::operator!=(const Matrix &B)
{
          //check dimensions
          if (num_col!=B.num_col)
                              return true;
          else if
                    (num_row!=B.num_row)
                              return true;

          for (int i=0;i<num_elem;i++){
                              //check to see if values differ by more or less than EPS
                              if ((*(pbody+i)-(*(B.pbody+i)))>EPS)
                                        return true;
                              else if ((*(pbody+i)-(*(B.pbody+i)))<(-1.*EPS))
                                        return true;
                    }
          return false;
}

///////////////////////////////////////////////////////////////////////////////
//Scalar multiplication operator
//Note: scalar must be the second operand
//Example: CMAT = AMAT * b;
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::operator*(const double &b)
{
          Matrix RESULT(num_row,num_col);

          for (int i=0;i<num_elem;i++)
                    *(RESULT.pbody+i)=*(pbody+i)*b;

          return RESULT;
}

///////////////////////////////////////////////////////////////////////////////
//Multiplication operator, return matrix product
// associative but not commutative
//Example: CMAT = AMAT * BMAT;
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::operator*(const Matrix &B)
{
          //create resultant matrix
          Matrix RESULT(num_row,B.num_col);
          int r=0; int c=0;

          //check for proper dimensions
          if (num_col!=B.num_row)
          {cout<<" *** Error: incompatible dimensions 'Matrix::operator*()' *** \n";system("pause");exit(1);}

          for(int i=0;i<RESULT.num_elem;i++){
                    r=i/B.num_col;
                    c=i%B.num_col;
                    for (int k=0; k<num_col;k++){
                              *(RESULT.pbody+i)+= *(pbody+k+num_col*r)*(*(B.pbody+k*B.num_col+c));
                    }
          }
          return RESULT;
}

///////////////////////////////////////////////////////////////////////////////
//Scalar multiplication assignment operator (scalar element by element multiplication)
//Example: AMAT *= b; meaning: AMAT = AMAT * b
///////////////////////////////////////////////////////////////////////////////
Matrix & Matrix::operator*=(const double &b)
{
          for (int i=0;i<num_elem;i++)
                    *(pbody+i)=*(pbody+i)*b;

          return *this;
}

///////////////////////////////////////////////////////////////////////////////
//Matrix multiplication assignment operator
//matrix B in argument must be square
//Example: AMAT *= BMAT; meaning: AMAT = AMAT * BMAT; 
///////////////////////////////////////////////////////////////////////////////
Matrix & Matrix::operator*=(const Matrix &B)
{
          //create resultant matrix
          Matrix RESULT(num_row,B.num_col);

          //check for proper dimensions
          if (num_col!=B.num_row)
          {cout<<" *** Error: incompatible dimensions in 'Matrix::operator*=()' *** \n";system("pause");exit(1);}

          //check for squareness of B
          if (B.num_col!=B.num_row)
          {cout<<" *** Error: Second matrix is not square in 'Matrix::operator*=()' *** \n";system("pause");exit(1);}

          int i(0);
          for(i=0;i<RESULT.num_elem;i++){
                    int r=i/B.num_col;
                    int c=i%B.num_col;
                    for (int k=0; k<num_col;k++){
                              *(RESULT.pbody+i)+= *(pbody+k+num_col*r)*(*(B.pbody+k*B.num_col+c));
                    }
          }
          num_col=RESULT.num_col;
          num_row=RESULT.num_row;
          num_elem=num_row*num_col;
          for (i=0;i<num_elem;i++)
                    *(pbody+i)=*(RESULT.pbody+i);

          return *this;
}

///////////////////////////////////////////////////////////////////////////////
//Scalar Addition operator (scalar element by element addition)
//Note: scalar must be the second operand
//Example: CMAT = AMAT + b; 
///////////////////////////////////////////////////////////////////////////////
Matrix  Matrix::operator+(const double &b)
{
          Matrix RESULT(num_row,num_col);

          for (int i=0;i<num_elem;i++)
                    *(RESULT.pbody+i)=*(pbody+i)+b;

          return RESULT;
}

///////////////////////////////////////////////////////////////////////////////
//Addition operator, returns matrix addition
//Example: CMAT = AMAT + BMAT;
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::operator+(const Matrix &B)
{
          Matrix RESULT(num_row,num_col);

          if ((num_col!=B.num_col)||(num_row!=B.num_row))
          {cout<<" *** Error: matrices have different dimensions in 'Matrix::operator +' *** \n";system("pause");exit(1);}

          for (int i=0;i<num_elem;i++)
                    *(RESULT.pbody+i)=*(pbody+i)+(*(B.pbody+i));

          return RESULT;
}

///////////////////////////////////////////////////////////////////////////////
//Scalar addition assignment operator (scalar element by element addition)
//Example: AMAT += b; meaning: AMAT = AMAT + b 
///////////////////////////////////////////////////////////////////////////////
Matrix & Matrix::operator+=(const double &b)
{
          for (int i=0;i<num_elem;i++)
                    *(pbody+i)=*(pbody+i)+b;

          return *this;
}

///////////////////////////////////////////////////////////////////////////////
//Matrix addition assignment operator
//Example: AMAT += BMAT; meaning: AMAT = AMAT + BMAT;
///////////////////////////////////////////////////////////////////////////////
Matrix & Matrix::operator+=(const Matrix &B)
{
          if ((num_col!=B.num_col)||(num_row!=B.num_row))
          {cout<<" *** Error: matrices have different dimensions in 'Matrix::operator +=' *** \n";system("pause");exit(1);}

          for (int i=0;i<num_elem;i++)
                    *(pbody+i)=*(pbody+i)+(*(B.pbody+i));

          return *this;
}

///////////////////////////////////////////////////////////////////////////////
//Scalar substraction operator (scalar element by element substraction)
//Note: scalar must be the second operand
//Example: CMAT = AMAT - b;
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::operator-(const double &b)
{
          Matrix RESULT(num_row,num_col);
          for (int i=0;i<num_elem;i++)
                    *(RESULT.pbody+i)=*(pbody+i)-b;

          return RESULT;
}
///////////////////////////////////////////////////////////////////////////////
//Substraction operator, returns matrix substraction
//Example: CMAT = AMAT - BMAT;
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::operator-(const Matrix &B)
{
          Matrix RESULT(num_row,num_col);

          if ((num_col!=B.num_col)||(num_row!=B.num_row))
          {cout<<" *** Error: matrices have different dimensions in 'Matrix::operator -' *** \n";system("pause");exit(1);}
          for (int i=0;i<num_elem;i++)
                    *(RESULT.pbody+i)=*(pbody+i)-*(B.pbody+i);
          
          return RESULT;
}

///////////////////////////////////////////////////////////////////////////////
//Scalar substraction assignment operator (scalar element by element substraction)
//Example: AMAT -= b; meaning: AMAT = AMAT - b
///////////////////////////////////////////////////////////////////////////////
Matrix & Matrix::operator-=(const double &b)
{
          for (int i=0;i<num_elem;i++)
                    *(pbody+i)=*(pbody+i)-b;

          return *this;
}

///////////////////////////////////////////////////////////////////////////////
//Matrix subtraction assignment operator
//Example: AMAT -= BMAT; meaning: AMAT = AMAT - BMAT;
///////////////////////////////////////////////////////////////////////////////
Matrix & Matrix::operator-=(const Matrix &B)
{
          if ((num_col!=B.num_col)||(num_row!=B.num_row))
          {cout<<" *** Error: matrices have different dimensions in 'Matrix::operator +=' *** \n";system("pause");exit(1);}

          for (int i=0;i<num_elem;i++)
                    *(pbody+i)=*(pbody+i)-(*(B.pbody+i));

          return *this;
}

///////////////////////////////////////////////////////////////////////////////
//Assignment operator (deep copy)
//Example: AMAT = BMAT; also: AMAT = BMAT = CMAT;
//Actually: AMAT.operator=(BMAT); also: AMAT.operator=(BMAT.operator=(CMAT));
///////////////////////////////////////////////////////////////////////////////
Matrix & Matrix::operator=(const Matrix &B)
{
          if((num_row != B.num_row)||(num_col != B.num_col))
          {cerr<<" *** Error: incompatible dimensions in 'Matrix::operator=()' *** \n";system("pause");exit(1);}

          delete [] pbody;
          num_elem=B.num_elem;
          num_row=B.num_row;
          num_col=B.num_col;
          pbody=new double[num_elem];

          for (int i=0;i<num_elem;i++)
                    *(pbody+i)=(*(B.pbody+i));

          return *this;
}
///////////////////////////////////////////////////////////////////////////////
//Equality relational operator
//returns true if elements differ by less than EPS
//Example: if(AMAT==BMAT){...};
///////////////////////////////////////////////////////////////////////////////
bool Matrix::operator==(const Matrix &B)
{
          //check dimensions
          if (num_col!=B.num_col)
                              return false;
          else if
                    (num_row!=B.num_row)
                              return false;

          for (int i=0;i<num_elem;i++){
                              //check to see if values differ by more or less than EPS
                              if ((*(pbody+i)-(*(B.pbody+i)))>EPS)
                                        return false;
                              else if ((*(pbody+i)-(*(B.pbody+i)))<(-1.*EPS))
                                        return false;
                    }
          return true;
}
///////////////////////////////////////////////////////////////////////////////
//Extracting components from vector with offset operator []
//returns the component(i) from vector VEC[i] or assigns a value to component VEC[i]
//Examples: comp_i=VEC[i]; VEC[i]=comp_i;
///////////////////////////////////////////////////////////////////////////////
double & Matrix::operator[](const int &r)
{
          if((r<num_row)&&(num_col=1))
                    return *(pbody+r);		
          else
          {
                    {cout<<" *** Error: invalid offset index in 'Matrix::operator[]' *** \n";system("pause");exit(1);}
          }
}
///////////////////////////////////////////////////////////////////////////////
//Scalar product operator (any combination of row or column vectors)  
//Example: value = AMAT ^ BMAT;  
///////////////////////////////////////////////////////////////////////////////
double Matrix::operator^(const Matrix &B)
{
          //initialize the result
          double result(0);

          //check dimensions
          bool one=false;
          bool dim=false;
          //true if both arrays have dimension '1'
          if((num_row==1||num_col==1)&&(B.num_row==1||B.num_col==1))one=true;
          //true if both arrays have at least one equal dimension
          if((num_row==B.num_row||num_row==B.num_col)&&(num_col==B.num_col||num_col==B.num_row))dim=true;
          if(!one||!dim)
          {cerr<<" *** Error: incompatible dimensions in 'Matrix::operator^()' *** \n";system("pause");exit(1);}

          for (int i=0;i<num_row;i++)
                              result+=*(pbody+i)*(*(B.pbody+i));
          return result;
}
///////////////////////////////////////////////////////////////////////////////
//Unit vector cross product from two 3x1 vectors  
//Example: UNIT = VEC1%VEC2;  
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::operator%(const Matrix &B)
{
          //initializing the result
          Matrix RESULT(3,1);
          //local variables
          double v1(0);
          double v2(0);
          double v3(0);
          double dv(0);

          //check for proper dimensions
          if (num_elem!=3||B.num_elem!=3)
          {cout<<" *** Error: incompatible dimensions in 'Matrix::operator%()' *** \n";system("pause");exit(1);}
          
          v1=*(pbody+1)*(*(B.pbody+2))-*(pbody+2)*(*(B.pbody+1)); 
          v2=*(pbody+2)*(*(B.pbody))-*(pbody)*(*(B.pbody+2)); 
          v3=*(pbody)*(*(B.pbody+1))-*(pbody+1)*(*(B.pbody));	
          dv=sqrt(v1*v1+v2*v2+v3*v3);

          //check for zero magnitude
          if (dv==0)
          {cout<<" *** Error: divide by zero in 'Matrix::operator%()' *** \n";system("pause");exit(1);}

          RESULT.assign_loc(0,0,v1/dv);
          RESULT.assign_loc(1,0,v2/dv);
          RESULT.assign_loc(2,0,v3/dv);
          return RESULT;
}
///////////////////////////////////////////////////////////////////////////////
//Alternate transpose Aji <- Aij			
//Example: BMAT = ~AMAT;
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::operator~()
{
          Matrix RESULT(num_col, num_row);
          int i=0; //offset for original matrix
          int j=0; //offset for transposed matrix

          for (int r=0;r<num_row;r++){
                    for(int c=0;c<num_col;c++){
                              //offset for transposed
                              j=c*num_row+r;
                              *(RESULT.pbody+j)=*(pbody+i);
                              i++;j++;
                    }
          }			
          return RESULT;
}

///////////////////////////////////////////////////////////////////////////////
//Returns polar from cartesian coordinates
// magnitude = POLAR[0] = |V|
// azimuth   = POLAR[1] = atan2(V2,V1)
// elevation = POLAR[2] = atan2(-V3,sqrt(V1^2+V2^2)
//Example: POLAR = VEC.pol_from_cart();
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::pol_from_cart() 
{
          double d=0.0;
          double azimuth=0.0;
          double elevation=0.0;
          double denom;
          Matrix POLAR(3,1);
          
          double v1=(*pbody);
          double v2=(*(pbody+1));
          double v3=(*(pbody+2));

          d=sqrt(v1*v1+v2*v2+v3*v3);
          azimuth=atan2(v2,v1);

          denom=sqrt(v1*v1+v2*v2);
          if(denom>0.)
                    elevation=atan2(-v3,denom);
          else{
                    if(v3>0) elevation=-PI/2.;
                    if(v3<0) elevation=PI/2.;
                    if(v3==0) elevation=0.;
          }
          
          *POLAR.pbody=d;
          *(POLAR.pbody+1)=azimuth;
          *(POLAR.pbody+2)=elevation;

          return POLAR;
}
/*
///////////////////////////////////////////////////////////////////////////////
//Puts # of cols into private member Matrix::col_num of MAT
//Example: MAT.put_cols(cols);
///////////////////////////////////////////////////////////////////////////////
void Matrix::put_cols(int cols)
{
          num_col=cols;
}

///////////////////////////////////////////////////////////////////////////////
//Puts # of elems into private member Matrix::elem_num of MAT
//Example: MAT.put_elems(elems);
///////////////////////////////////////////////////////////////////////////////
void Matrix::put_elems(int elems)
{
          num_elem=elems;
}

///////////////////////////////////////////////////////////////////////////////
//Puts # of rows into private member Matrix::row_num of MAT
//Example: MAT.put_rows(rows);
///////////////////////////////////////////////////////////////////////////////
void Matrix::put_rows(int rows)
{
          num_row=rows;
}
*/
///////////////////////////////////////////////////////////////////////////////
//Returns row vector of row # 
//Example: VEC = MAT.row_vec(2); (2nd row!)
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::row_vec(const int &row)
{
          if(row<=0||row>num_row)
          {cerr<<" *** Error: row outside array in 'Matrix::row_vec()' *** \n";system("pause");exit(1);}
          
          Matrix RESULT(1,num_col);

          for(int i=0;i<num_col;i++){
                    int offset=(row-1)*num_col+i;
                    *(RESULT.pbody+i)=(*(pbody+offset));
          }
          return RESULT;
}

///////////////////////////////////////////////////////////////////////////////
//Returns the skew-symmetric matrix from a 3-dim vector VEC
//			| 0 -c  b|		|a|
//			| c  0 -a| <--	|b|
//			|-b  a  0|		|c|
//
//Example: MAT = VEC.skew_sym();
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::skew_sym()
{
          Matrix RESULT(3,3);
          //check for proper dimensions
          if (num_col!=1||num_row!=3)
          {cout<<" *** Error: not a 3x1 column vector in 'Matrix::skew_sym()' *** \n";system("pause");exit(1);}
          
          *(RESULT.pbody+5)=-(*pbody);
          *(RESULT.pbody+7)=(*pbody);
          *(RESULT.pbody+2)=(*(pbody+1));
          *(RESULT.pbody+6)=-(*(pbody+1));
          *(RESULT.pbody+1)=-(*(pbody+2));
          *(RESULT.pbody+3)=(*(pbody+2));

          return RESULT;
}

///////////////////////////////////////////////////////////////////////////////
//Returns the sub matrix after 'row'  and 'col' have been ommitted 
//Example: BMAT = AMAT.sub_matrix(1,3); (deleting first row and third column!) 
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::sub_matrix(const int &row, const int &col)
{ 
          if((row>num_row)||(col>num_col))
          {cerr<<" *** Error: row or column outside array in 'Matrix::sub_matrix()' *** \n";system("pause");exit(1);}
          if(row==0||col==0)
          {cerr<<" *** Error: row/col are numbered not offset in 'Matrix::sub_matrix()' *** \n";system("pause");exit(1);}

          //create return matrix
          Matrix RESULT(num_row-1,num_col-1);
          //start and stop of skipping matrix elements 
          int skip_start=(row-1)*num_col;
          int skip_end=skip_start+num_col;

          //initialize RESULT offset j
          int j=0;

          for (int i=0;i<num_elem;i++){
                    //skip elements of row to be removed
                    if((i<skip_start)||(i>=skip_end)){
                              //offset of column element to be removed
                              int offset_col=(col-1)+(i/num_col)*num_col;
                              //skip elements of col to be removed
                              if(i!=offset_col){
                                        *(RESULT.pbody+j)=*(pbody+i);
                                        j++;
                              }
                    }
          }
          return RESULT;
}

///////////////////////////////////////////////////////////////////////////////
//Transpose Aji <- Aij			
//Example: BMAT = AMAT.trans();
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::trans()
{
          Matrix RESULT(num_col, num_row);
          int i=0; //offset for original matrix
          int j=0; //offset for transposed matrix

          for (int r=0;r<num_row;r++){
                    for(int c=0;c<num_col;c++){
                              //offset for transposed
                              j=c*num_row+r;
                              *(RESULT.pbody+j)=*(pbody+i);
                              i++;j++;
                    }
          }			
          return RESULT;
}

///////////////////////////////////////////////////////////////////////////////
//Returns unit vector from 3x1 vector
//Example: UVEC=VEC.univec3();
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::univec3()
{
          Matrix RESULT(3,1);
          //check for proper dimensions
          if (num_col!=1||num_row!=3)
          {cout<<" *** Error: not a 3x1 column vector in 'Matrix::univec()' *** \n";system("pause");exit(1);}

          double v1=(*pbody);
          double v2=(*(pbody+1));
          double v3=(*(pbody+2));
          double d=sqrt(v1*v1+v2*v2+v3*v3);

          //if VEC is zero than the unit vector is also a zero vector
          if(d==0){		
                    *RESULT.pbody=0;
                    *(RESULT.pbody+1)=0;
                    *(RESULT.pbody+2)=0;
          }
          else{
                    *RESULT.pbody=v1/d;
                    *(RESULT.pbody+1)=v2/d;
                    *(RESULT.pbody+2)=v3/d;
          }	
          return RESULT;
}
///////////////////////////////////////////////////////////////////////////////
//Returns 9x1 vector from 3x3 matrix row-wise
//Example: VEC=MAT.vec9_mat33();
///////////////////////////////////////////////////////////////////////////////
Matrix Matrix::vec9_mat33()
{
          if(!(num_row==3 && num_col==3))
          {cerr<<" *** Error: matrix not 3 x 3 in 'Matrix::vec9_mat33()' *** \n";system("pause");exit(1);}
          
          Matrix RESULT(9,1);
          for(int i=0;i<9;i++){
                    *(RESULT.pbody+i)=*(pbody+i);
          }
          return RESULT;
}

///////////////////////////////////////////////////////////////////////////////
//Forms a zero matrix MAT from object MAT(num_row,num_col)
//Example: MAT.zero();
///////////////////////////////////////////////////////////////////////////////
Matrix & Matrix::zero()
{
          for(int i=0;i<num_elem;i++)
                    *(pbody+i)=0.0;

          return *this;
}
///////////////////////////////////////////////////////////////////////////////
////////////////// Module utility functions ///////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//Returns the angle between two 3x1 vectors
//Example: theta=angle(VEC1,VEC2);
//010824 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
double angle(Matrix VEC1,Matrix VEC2)
{
          double argument;
          double scalar=VEC1^VEC2;
          double abs1=VEC1.absolute();
          double abs2=VEC2.absolute();

          double dum=abs1*abs2;
          if(abs1*abs2>EPS)
                    argument=scalar/dum;
          else
                    argument=1.;
          if(argument>1.) argument=1.;
          if(argument<-1.) argument=-1.;

          return acos(argument);
}
///////////////////////////////////////////////////////////////////////////////
//Returns the sign of the variable
//Example: value_signed=value*sign(variable) 
//010824 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
int sign(const double &variable)
{
          int sign=0;
          if(variable<0.)sign=-1;
          if(variable>=0.)sign=1;

          return sign;
}
////////////////////////////////////////////////////////////////////////////////
//Returns the T.M. of the psivg -> thtvg sequence
//
//010628 Created by Peter H Zipfel
////////////////////////////////////////////////////////////////////////////////
Matrix mat2tr(const double &psivg,const double &thtvg)
{
          Matrix AMAT(3,3);

          AMAT.assign_loc(0,2,-sin(thtvg));
          AMAT.assign_loc(1,0,-sin(psivg));
          AMAT.assign_loc(1,1,cos(psivg));
          AMAT.assign_loc(2,2,cos(thtvg));
          AMAT.assign_loc(0,0,(AMAT.get_loc(2,2) * AMAT.get_loc(1,1)));
          AMAT.assign_loc(0,1,(-AMAT.get_loc(2,2) * AMAT.get_loc(1,0)));
          AMAT.assign_loc(2,0,(-AMAT.get_loc(0,2) * AMAT.get_loc(1,1)));
          AMAT.assign_loc(2,1,(AMAT.get_loc(0,2) * AMAT.get_loc(1,0)));
          AMAT.assign_loc(1,2,0.0);

          return AMAT;
}
////////////////////////////////////////////////////////////////////////////////
//Returns the T.M. of the psi->tht->phi sequence
//Euler angle transformation matrix of flight mechanics
//
//011126 Created by Peter H Zipfel
////////////////////////////////////////////////////////////////////////////////

Matrix mat3tr(const double &psi,const double &tht,const double &phi)
{
          double spsi=sin(psi);
          double cpsi=cos(psi);
          double stht=sin(tht);
          double ctht=cos(tht);
          double sphi=sin(phi);
          double cphi=cos(phi);

          Matrix AMAT(3,3);
          AMAT.assign_loc(0,0,cpsi*ctht);
          AMAT.assign_loc(1,0,cpsi*stht*sphi-spsi*cphi);
          AMAT.assign_loc(2,0,cpsi*stht*cphi+spsi*sphi);
          AMAT.assign_loc(0,1,spsi*ctht);
          AMAT.assign_loc(1,1,spsi*stht*sphi+cpsi*cphi);
          AMAT.assign_loc(2,1,spsi*stht*cphi-cpsi*sphi);
          AMAT.assign_loc(0,2,-stht);
          AMAT.assign_loc(1,2,ctht*sphi);
          AMAT.assign_loc(2,2,ctht*cphi);

          return AMAT;
}
///////////////////////////////////////////////////////////////////////////////
//Returns great circle distance between two point on a spherical Earth
// Reference: Bate et al. "Fundamentals of Astrodynamics", Dover 1971, p. 310
// Return output
//			       distancex = great circle distance - km
// Parameter input
//			       lon1 = longitude of first point - rad
//                 lat1 = latitude -of first point  rad
//			       lon2 = longitude of second point - rad
//                 lat2 = latitude -of second point  rad
//
//030414 Created from FORTRAN by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
double cad_distance(const double &lon1,const double &lat1,const double &lon2,const double &lat2)			  
{
          double dum=sin(lat2)*sin(lat1)+cos(lat2)*cos(lat1)*cos(lon2-lon1);
          if(fabs(dum)>1.)dum=1.*sign(dum);
          double distancex=REARTH*acos(dum)*1.e-3;

          return distancex;
}
//////////////////////////////////////////////////////////////////////////////
//Calculates geodetic longitude, latitude, and altitude from inertial displacement vector
// using the WGS 84 reference ellipsoid
// Reference: Britting,K.R."Inertial Navigation Systems Analysis", Wiley. 1971
//
// Parameter output
//			       lon = geodetic longitude - rad
//                 lat = geodetic latitude - rad
//                 alt = altitude above ellipsoid - m
// Parameter input
//			       SBII(3x1) = Inertial position - m
//
//030414 Created from FORTRAN by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
void cad_geo84_in(double &lon,double &lat,double &alt, Matrix SBII,const double &time)
{
          int count(0);
          double lat0(0);
          double alamda(0);

          //initializing geodetic latitude using geocentric latitude
          double dbi=SBII.absolute();
          double latg=asin(SBII.get_loc(2,0)/dbi);
          lat=latg;
          
          //iterating to calculate geodetic latitude and altitude
          do{
                    lat0=lat;
                    double r0 = SMAJOR_AXIS * 
                    (
                        1. -
                        FLATTENING * (1. - cos(2. * lat0)) / 2. +
                        5. * pow(FLATTENING, 2) * (1. - cos(4. * lat0)) / 16.
                    ); //eq 4-21
                    alt=dbi-r0;
                    double dd=FLATTENING*sin(2.*lat0)*(1.-FLATTENING/2.-alt/r0); //eq 4-15
                    lat=latg+dd;
                    count++;
                    if(count>100){
                              cout << "\n *** Stop: Geodetic latitude does not converge,'cad_geo84_in()' ***\n";system("pause");exit(1);}

          }while(fabs(lat-lat0)>SMALL);

          //longitude
          double sbii1=SBII.get_loc(0,0);
          double sbii2=SBII.get_loc(1,0);
          double dum4=asin(sbii2/sqrt(sbii1*sbii1+sbii2*sbii2));
          //Resolving the multi-valued arcsin function 
          if((sbii1>=0.0)&&(sbii2>=0.0)) alamda=dum4;				// quadrant I
          if((sbii1<0.0)&&(sbii2>=0.0)) alamda=(180.*RAD)-dum4;	// quadrant II
          if((sbii1<0.0)&&(sbii2<0.0)) alamda=(180.*RAD)-dum4;	// quadrant III
          if((sbii1>0.0)&&(sbii2<0.0)) alamda=(360.*RAD)+dum4;	// quadrant IV
          lon=alamda-WEII3*time-GW_CLONG;
          if((lon)>(180.*RAD)) lon=-((360.*RAD)-lon);  // east positive, west negative
}
//////////////////////////////////////////////////////////////////////////////
//Returns geodetic velocity vector information from inertial postion and velocity
// using the WGS 84 reference ellipsoid
//
// Calls utilities
//					cad_geo84_in(...), cad_tdi84(...)
// Parameter output
//			       dvbe = geodetic velocity - m/s
//                 psivdx = geodetic heading angle - deg
//                 thtvdx = geodetic flight path angle - deg
// Parameter input
//			       SBII(3x1) = Inertial position - m
//			       VBII(3x1) = Inertial velocity - m
//
//040710 created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
void cad_geo84vel_in(double &dvbe,double &psivdx,double &thtvdx
                                                   ,Matrix SBII,Matrix VBII,const double &time)			  
{
          double lon(0);
          double lat(0);
          double alt(0);
          //geodetic longitude, latitude and altitude
          cad_geo84_in(lon,lat,alt,SBII,time);
          Matrix TDI=cad_tdi84(lon,lat,alt,time);

          //Earth's angular velocity skew-symmetric matrix (3x3)
          Matrix WEII(3,3);
          WEII.assign_loc(0,1,-WEII3);
          WEII.assign_loc(1,0,WEII3);

          //geographic velocity in geodetic axes VBED(3x1) and flight path angles
          Matrix VBED=TDI*(VBII-WEII*SBII);
          Matrix POLAR=VBED.pol_from_cart();
          dvbe=POLAR[0];
          psivdx=DEG*POLAR[1];
          thtvdx=DEG*POLAR[2];
}
////////////////////////////////////////////////////////////////////////////////
//Returns geocentric lon, lat, alt from inertial displacement vector
// for spherical Earth
//
// Parameter output
//				lonc = geocentric longitude - rad
//				latc = geocentric latitude - rad
//				altc = geocentric latitude - m
// Parameter input
//				SBII = Inertial position - m
//              time = simulation time - sec
//
//010628 Created by Peter H Zipfel
//030416 Modified for SBII (not SBIE) input, PZi
////////////////////////////////////////////////////////////////////////////////
void cad_geoc_in(double &lonc,double &latc,double &altc, Matrix SBII,const double &time)
{
          double lon_cel(0);
          double sbii1=SBII.get_loc(0,0);
          double sbii2=SBII.get_loc(1,0);
          double sbii3=SBII.get_loc(2,0);
          Matrix RESULT(3,1);

             //latitude  
          double dbi=sqrt(sbii1*sbii1+sbii2*sbii2+sbii3*sbii3);
          latc=asin((sbii3)/dbi);

          //altitude 
          altc=dbi-REARTH;

          //longitude 
          double dum4=asin(sbii2/sqrt(sbii1*sbii1+sbii2*sbii2));
          //Resolving the multi-valued arcsin function 
          if((sbii1>=0.0)&&(sbii2>=0.0)) lon_cel=dum4;			// quadrant I
          if((sbii1<0.0)&&(sbii2>=0.0)) lon_cel=(180.*RAD)-dum4;	// quadrant II
          if((sbii1<0.0)&&(sbii2<0.0)) lon_cel=(180.*RAD)-dum4;	// quadrant III
          if((sbii1>0.0)&&(sbii2<0.0)) lon_cel=(360.*RAD)+dum4;	// quadrant IV
          lonc=lon_cel-WEII3*time-GW_CLONG;
          if((lonc)>(180.*RAD)) lonc=-((360.*RAD)-lonc);  // east positive, west negative
}
////////////////////////////////////////////////////////////////////////////////
//Returns lon, lat, alt from displacement vector in Earth coord for spherical earth
//
// Return output
//		RETURN[0]=lon
//		RETURN[1]=lat
//		RETURN[2]=alt
// Paramter input
//		SBIE = displacement of vehicle wrt Earth center in Earth coordinates 
//
//010628 Created by Peter H Zipfel
////////////////////////////////////////////////////////////////////////////////
Matrix cad_geoc_ine(Matrix SBIE)
{
          double dum4(0);
          double alamda(0);
          double x(0),y(0),z(0);
          double dbi(0);
          double alt(0);
          double lat(0);
          double lon(0);
          Matrix RESULT(3,1);

          //downloading inertial components
          x=SBIE.get_loc(0,0);
          y=SBIE.get_loc(1,0);
          z=SBIE.get_loc(2,0);
             //Latitude  
          dbi=sqrt(x*x+y*y+z*z);
          lat=asin((z)/dbi);

          //Altitude 
          alt=dbi-REARTH;

          //Longitude 
          dum4=asin(y/sqrt(x*x+y*y));

          
          //Resolving the multi-valued arcsin function 
          if((x>=0.0)&&(y>=0.0))
          {
                    alamda=dum4;   // quadrant I
          }
          if((x<0.0)&&(y>=0.0))
          {
                    alamda=(180.0*RAD)-dum4;   // quadrant II
          }
          if((x<0.0)&&(y<0.0))
          {
                    alamda=(180.0*RAD)-dum4;  // quadrant III
          }
          if((x>=0.0)&&(y<0.0))
          {
                    alamda=(360.0*RAD)+dum4;  // quadrant IV
          }
          
          lon = alamda;
          if((lon)>(180.0*RAD))
          {
                    lon = -((360.0 * RAD) - lon);  // east positive, west negative
          }
          RESULT.assign_loc(0,0,lon);
          RESULT.assign_loc(1,0,lat);
          RESULT.assign_loc(2,0,alt);

          return RESULT;
}
///////////////////////////////////////////////////////////////////////////////
//Earth gravitational acceleration, using the WGS 84 ellipsoid
//Ref: Chatfield, A.B.,"Fundamentals of High Accuracy Inertial
//Navigation",p.10, Prog.Astro and Aeronautics, Vol 174, AIAA, 1997.
//
// Return output
//			       GRAVG(3x1) = gravitational acceleration in geocentric coord - m/s^2
// Parameter input
//			       SBII = inertial displacement vector - m
//				   time = simulation time - sec 
//
//030417 Created from FORTRAN by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
Matrix cad_grav84(Matrix SBII,const double &time)			  
{

          double lonc(0),latc(0),altc(0);
          Matrix GRAVG(3,1);

          cad_geoc_in(lonc,latc,altc, SBII,time);
          double dbi=SBII.absolute();
          double dum1=GM/(dbi*dbi);
          double dum2=3*sqrt(5.);
          double dum3=pow((SMAJOR_AXIS/dbi),2);
          double gravg1=-dum1*dum2*C20*dum3*sin(latc)*cos(latc);
          double gravg2=0;
          double gravg3=dum1*(1.+dum2/2.*C20*dum3*(3.*pow(sin(latc),2)-1.));

          GRAVG.assign_loc(0,0,gravg1);
          GRAVG.assign_loc(1,0,gravg2);
          GRAVG.assign_loc(2,0,gravg3);

          return GRAVG;

}
///////////////////////////////////////////////////////////////////////////////
//Returns the inertial displacement vector from longitude, latitude and altitude
// using the WGS 84 reference ellipsoid
// Reference: Britting,K.R."Inertial Navigation Systems Analysis"
// pp.45-49, Wiley, 1971
//
// Return output
//			       SBII(3x1) = Inertial vehicle position - m
// Parameter input
//			       lon = geodetic longitude - rad
//                 lat = geodetic latitude - rad
//                 alt = altitude above ellipsoid - m
//				   time = simulation time - sec 
//
//030411 Created from FORTRAN by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
Matrix cad_in_geo84(const double lon,const double lat,const double alt
                                ,const double &time)
{
          Matrix SBII(3,1);
          Matrix SBID(3,1);

          //deflection of the normal, dd, and length of earth's radius to ellipse surface, R0
          double r0=SMAJOR_AXIS*(1.-FLATTENING*(1.-cos(2.*lat))/2.+5.*pow(FLATTENING,2)*(1.-cos(4.*lat))/16.); //eq 4-21
          double dd=FLATTENING*sin(2.*lat)*(1.-FLATTENING/2.-alt/r0); //eq 4-15

          //vehicle's displacement vector from earth's center SBID(3x1) in geodetic coord.
          double dbi=r0+alt;
          SBID.assign_loc(0,0,-dbi*sin(dd));
          SBID.assign_loc(1,0,0.);
          SBID.assign_loc(2,0,-dbi*cos(dd));

          //celestial longitude of vehicle at simulation 'time' 
         double lon_cel=GW_CLONG+WEII3*time+lon;
          
          //vehicle's displacement vector in inertial coord. SBII(3x1)=TID(3x3)xSBID(3x3)
          double slat=sin(lat);
          double clat=cos(lat);
          double slon=sin(lon_cel);
          double clon=cos(lon_cel);
          double sbid1=SBID.get_loc(0,0);
          double sbid2=SBID.get_loc(1,0);
          double sbid3=SBID.get_loc(2,0);
          double sbii1=-slat*clon*sbid1-clat*clon*sbid3;
          double sbii2=-slat*slon*sbid1-clat*slon*sbid3;
          double sbii3=clat*sbid1-slat*sbid3;
          SBII.build_vec3(sbii1,sbii2,sbii3);

          return SBII;
}
///////////////////////////////////////////////////////////////////////////////
//Returns the inertial displacement vector from geocentric longitude, latitude and altitude
// for spherical Earth
//
// Return output
//			SBII = position of vehicle wrt center of Earth, in inertial coord
// Argument input 
//			lon = geographic longitude - rad
//			lat = geocentric latitude - rad
//			alt = altitude above spherical Earth = m
//
//010405 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
Matrix cad_in_geoc(const double &lon,const double &lat,const double &alt
                                ,const double &time)
{
          Matrix VEC(3,1);

          double dbi=alt+REARTH;
          double cel_lon=lon+WEII3*time+GW_CLONG;
          double clat=cos(lat);
          double slat=sin(lat);
          double clon=cos(cel_lon);
          double slon=sin(cel_lon);

          VEC.assign_loc(0,0,dbi*clat*clon);
          VEC.assign_loc(1,0,dbi*clat*slon);
          VEC.assign_loc(2,0,dbi*slat);

          return VEC;
}
///////////////////////////////////////////////////////////////////////////////
//Calculates inertial displacement and velocity vectors from orbital elements
// Reference: Bate et al. "Fundamentals of Astrodynamics", Dover 1971, p.71
//
// Return output
//				parabola_flag = 0 ok
//							  = 1 not suitable (divide by zero), because parabolic trajectory
// Parameter output
//				SBII = Inertial position - m
//				SBII = Inertial velocity - m/s
// Parameter input
//              semi = semi-major axis of orbital ellipsoid - m
//              ecc = eccentricity of elliptical orbit - ND
//				inclx = inclination of orbital wrt equatorial plane - deg
//              lon_anodex = celestial longitude of the ascending node - deg
//              arg_perix = argument of periapsis (ascending node to periapsis) - deg
//              true_anomx = true anomaly (periapsis to satellite) - deg
//
//040510 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
int cad_in_orb(Matrix &SBII,Matrix &VBII ,const double &semi,const double &ecc
                                ,const double &inclx,const double &lon_anodex,const double &arg_perix
                                ,const double &true_anomx)
{
          //local variable
          int parabola_flag(0);
          Matrix SBIP(3,1);
          Matrix VBIP(3,1);
          Matrix TIP(3,3);
          
          //semi-latus rectum from semi-major axis and eccentricity
          double pp=semi*(1-ecc*ecc);

          //angles
          double c_true_anom=cos(true_anomx*RAD);
          double s_true_anom=sin(true_anomx*RAD);

          //inertial distance in perifocal coordinates
          double dbi=pp/(1+ecc*c_true_anom);

          //inertial position vector
          SBIP[0]=dbi*c_true_anom;
          SBIP[1]=dbi*s_true_anom;
          SBIP[2]=0;

          //pypass calculation if parabola
          if(pp==0)
                    parabola_flag=1;
          else{
                    //inertial velocity
                    double dum=sqrt(GM/pp);
                    VBIP[0]=-dum*s_true_anom;
                    VBIP[1]=dum*(ecc+c_true_anom);
                    VBIP[2]=0;
          }

          //transforming to inertial coordinates
          TIP=cad_tip(inclx*RAD,lon_anodex*RAD,arg_perix*RAD);
          SBII=TIP*SBIP;
          VBII=TIP*VBIP;

          return parabola_flag;
}
////////////////////////////////////////////////////////////////////////////////
// Projects initial state through 'tgo' to final state along a Keplerian trajectory
// Based on Ray Morth, unpublished utility
//
// Return output
//			       kepler_flag = 0: good Kepler projection;
//							   = 1: bad (# of iterations>20, or neg. sqrt), no new proj cal, use prev value;  - ND
// Parameter output
//			       SPII = projected inertial position after tgo - m
//			       VPII = projected inertial velocity after tgo - m/s
// Parameter input
//			       SBII = current inertial position - m
//			       VBII = current inertial velocity - m/s
//				    tgo = time-to-go to projected point - sec
//
//040319 Created from FORTRAN by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
int cad_kepler(Matrix &SPII,Matrix &VPII, Matrix SBII,Matrix VBII,const double &tgo)
{
          //local variables
          double sde(0);
          double cde(0);
          int kepler_flag(0);

          double sqrt_GM=sqrt(GM);	
          double ro=SBII.absolute();
          double vo=VBII.absolute();
          double rvo=SBII^VBII;
          double a1=vo*vo/GM;
          double sa=ro/(2-ro*a1);
          if(sa<0){
                    //return without re-calculating SPII, VPII
                    kepler_flag=1;
                    return kepler_flag;
          }
          double smua=sqrt_GM*sqrt(sa);
          double mdot=smua/(sa*sa);

          //calculating 'de'iteratively
          double dm=mdot*tgo;
          double de=dm; //initialize eccentricity
          double a11=rvo/smua;
          double a21=(sa-ro)/sa;
          int count20=0;
          double adm(0);
          do{
                    cde=1-cos(de);
                    sde=sin(de);
                    double dmn=de+a11*cde-a21*sde;
                    double dmerr=dm-dmn;

                    adm=fabs(dmerr)/mdot;
                    double dmde=1+a11*sde-a21*(1-cde);
                    de=de+dmerr/dmde;
                    count20++;
                    if(count20>20){
                              //return without re-calculating SPII, VPII
                              kepler_flag=1;
                              return kepler_flag;
                    }
          }while(adm>SMALL);

          //projected position
          double fk=(ro-sa*cde)/ro;
          double gk=(dm+sde-de)/mdot;
          SPII=SBII*fk+VBII*gk;

          //projected velocity
          double rp=SPII.absolute();
          double fdk=-smua*sde/ro;
          double gdk=rp-sa*cde;
          VPII=SBII*(fdk/rp)+VBII*(gdk/rp);
          
          return kepler_flag;
}
////////////////////////////////////////////////////////////////////////////////
// Projects initial state through 'tgo' to final state along a Keplerian trajectory
// Based on: Bate, Mueller, White, "Fundamentals of Astrodynamics", Dover 1971
//
// Return output
//			       iter_flag=0: # of iterations < 20; =1: # of iterations > 20;  - ND
// Parameter output
//			       SPII = projected inertial position after tgo - m
//			       VPII = projected inertial velocity after tgo - m/s
// Parameter input
//			       SBII = current inertial position - m
//			       VBII = current inertial velocity - m/s
//				    tgo = time-to-go to projected point - sec
//
//040318 Created from ASTRO_KEP by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
int cad_kepler1(Matrix &SPII,Matrix &VPII, Matrix SBII,Matrix VBII,const double &tgo)
{
          double c(0);
          double s(0);
          double z(0);
          double dt(0);
          int iter_flag(0);

          double ro=SBII.absolute();
          double vo=VBII.absolute();
          double al=(2*GM/ro-vo*vo)/GM;
          double en=-GM*al/2; //specific mechanical energy - J/kg	
          Matrix AM=SBII.skew_sym()*VBII;
          double h=AM.absolute(); //angular momentum - m^2/s
          double de2=1+2*en*h*h/(GM*GM);
          if(de2<0)de2=0;
          double de=sqrt(de2); //eccentricity
          double dum=SBII^VBII;
                    double sqrt_GM=sqrt(GM);

          //initial guaess of x
          double x=0;
          int count20=0;

          //calculating x using newton iteration
          do{
                    count20++;
                    z=x*x*al;
                    cadkepler1_ucs(c,s, z);
                    dt=(x*x*x*s+dum*x*x*c/sqrt_GM+ro*x*(1-z*s))/sqrt_GM;
                    double dtx=(x*x*c+dum*x*(1-z*s)/sqrt_GM+ro*(1-z*c))/sqrt_GM;
                    x=x+(tgo-dt)/dtx;
          }while(fabs((tgo-dt)/tgo)>SMALL);

          //projected inertial position
          double f=1-x*x*c/ro;
          double g=tgo-x*x*s/sqrt_GM;
          SPII=SBII*f+VBII*g;

          //projecting inertial velocity
          double rx=SPII.absolute();
          double fd=sqrt_GM*x*(z*s-1)/(ro*rx);
          double gd=1-x*x*c/rx;
          VPII=SBII*fd+VBII*gd;

          //dignostics: if one=1  f and g variables are ok
          double one=f*gd-fd*g;
          
          //diagnostic: bad iteration if count20 > 20 
          if(count20>20) iter_flag=1;
          return iter_flag;
}

////////////////////////////////////////////////////////////////////////////////
// Calculates utility functions c(z) and s(z) for cad_kepler(...)
// Reference: Bate, Mueller, White, "Fundamentals of Astrodynamics", Dover 1971, p.196
//
// Paramter output
//			       c = c(z) utility function
//			       s = s(z) utility function
// Parameter input
//			       z = z-variable
//
//040318 Created from ASTRO_UCS by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
void cadkepler1_ucs(double &c,double &s, const double &z)
{
          double sd(0);
          double cd(0);

          if(z>0.1){
                    c=(1-cos(sqrt(z)))/z;
                    s=(sqrt(z)-sin(sqrt(z)))/sqrt(z*z*z);
                    sd=(c-3*s)/(2*z);
                    cd=(1-z*s-2*c)/(2*z);
          }
          if(z<-0.1){
                    c=(1-cosh(sqrt(-z)))/z;
                    s=(sinh(sqrt(-z))-sqrt(-z))/sqrt(-z*z*z);
                    sd=(c-3*s)/(2*z);
                    cd=(1-z*s-2*c)/(2*z);
          }
          if(fabs(z)<=0.1){
                    double dc=2;
                    c=1/dc;
                    double dcd=-24;
                    cd=1/dcd;
                    double ds=6;
                    s=1/ds;
                    double dsd=-120;
                    sd=1/dsd;

                    for(int k=1;k<7;k++){
                              double z_pow_k=pow(-z,k);
                              int n=2*k+1;		
                              dc=dc*n*(n+1);
                              c=c+z_pow_k/dc;
                              dcd=dcd*(n+2)*(n+3);
                              cd=cd-(k+1)*z_pow_k/dcd;
                              ds=ds*(n+1)*(n+2);
                              s=s+z_pow_k/ds;
                              dsd=dsd*(n+3)*(n+4);		
                                sd=sd-(k+1)*z_pow_k/dsd;
                    }
          }
}
///////////////////////////////////////////////////////////////////////////////
//Calculates the orbital elements from inertial displacement and velocity
// Reference: Bate et al. "Fundamentals of Astrodynamics", Dover 1971, p.58
//
// Return output
//			cadorbin_flag = 0 ok
//						    1 'true_anomx' not calculated, because of circular orbit
//						    2 'semi' not calculated, because parabolic orbit
//						    3 'lon_anodex' not calculated, because equatorial orbit
//						    13 'arg_perix' not calculated, because equatorialand/or circular orbit
// Parameter output:
//          semi = semi-major axis of orbital ellipsoid - m
//          ecc = eccentricity of elliptical orbit - ND
//			inclx = inclination of orbital wrt equatorial plane - deg
//          lon_anodex = celestial longitude of the ascending node - deg
//          arg_perix = argument of periapsis (ascending node to periapsis) - deg
//          true_anomx = true anomaly (periapsis to satellite) - deg
//
// Parameter input:
//				SBII = Inertial position - m
//				VBII = Inertial velocity - m/s
//
//040510 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
int cad_orb_in(double &semi,double &ecc,double &inclx,double &lon_anodex			  
                               ,double &arg_perix,double &true_anomx, Matrix &SBII,Matrix &VBII)
{
          //local variable
          int cadorbin_flag(0);
          Matrix NODE_I(3,1);
          double lon_anode(0);
          double arg_peri(0);
          double true_anom(0);

          //angular momentum vector of orbit
          Matrix ANGL_MOM_I=SBII.skew_sym()*VBII;
          double angl_mom=ANGL_MOM_I.absolute();

          //vector of the ascending node (undefined if inclx=0)
          NODE_I[0]=-ANGL_MOM_I[1];
          NODE_I[1]=ANGL_MOM_I[0];
          double node=NODE_I.absolute();

          //orbit eccentricity vector and magnitude
          double dbi=SBII.absolute();
          double dvbi=VBII.absolute();
          Matrix EI(3,1);
          EI=(SBII*(dvbi*dvbi-GM/dbi)-VBII*(SBII^VBII))*(1/GM);
          ecc=EI.absolute();

          //semi latus rectum
          double pp=angl_mom*angl_mom/GM;

          //semi-major axis of orbit
          if(pp==1)
                    cadorbin_flag=2;
          else
                    semi=pp/(1-ecc*ecc);

          //orbit inclination
          double arg=ANGL_MOM_I[2]*(1/angl_mom);
          if(fabs(arg)>1)
                    arg=1;
          inclx=acos(arg)*DEG;

          //bypass calculations if equatorial orbit
          if(node<SMALL){
                    cadorbin_flag=3;
          }
          else{
                    //longitude of the ascending node
                    arg=NODE_I[0]/node;
                    if(fabs(arg)>1)
                              arg=1;
                    lon_anode=acos(arg);
          }

          //bypass calculations if circular and/or equatorial orbit
          if(ecc<SMALL||node<SMALL)
                    cadorbin_flag=13;
          else{
                    //argument of periapsis
                    arg=NODE_I^EI*(1/(node*ecc));
                    if(fabs(arg)>1)
                              arg=1;
                    arg_peri=acos(arg);
          }

          //bypass calculations if circular orbit
          if(ecc<SMALL)
                    cadorbin_flag=1;
          else{
                    //true anomaly
                    arg=SBII^EI*(1/(dbi*ecc));
                    if(fabs(arg)>1)
                              arg=1;
                    true_anom=acos(arg);
          }

          //quadrant resolution
          double quadrant=SBII^VBII;
          if(quadrant>=0)
                    true_anomx=true_anom*DEG;
          else
                    true_anomx=(2*PI-true_anom)*DEG;

          if(EI[2]>=0)
                    arg_perix=arg_peri*DEG;
          else
                    arg_perix=(2*PI-arg_peri)*DEG;

          if(NODE_I[1]>0)
                    lon_anodex=lon_anode*DEG;
          else
                    lon_anodex=(2*PI-lon_anode)*DEG;

          return cadorbin_flag;
}
///////////////////////////////////////////////////////////////////////////////
//Returns the T.M. of geodetic wrt inertial coordinates
// using the WGS 84 reference ellipsoid
//
// Return output
//			       TDI(3x3) = T.M.of geosetic wrt inertial coord - ND
// Parameter input
//			       lon = geodetic longitude - rad
//                 lat = geodetic latitude - rad
//                 alt = altitude above ellipsoid - m
//
//030424 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
Matrix cad_tdi84(const double &lon,const double &lat,const double &alt
                                ,const double &time)
{
          Matrix TDI(3,3);

          //celestial longitude of vehicle at simulation 'time' 
          double lon_cel=GW_CLONG+WEII3*time+lon;

          //T.M. of geodetic coord wrt inertial coord., TDI(3x3)
          double tdi13=cos(lat);
          double tdi33=-sin(lat);
          double tdi22=cos(lon_cel);
          double tdi21=-sin(lon_cel);
          TDI.assign_loc(0,2,tdi13);
          TDI.assign_loc(2,2,tdi33);
          TDI.assign_loc(1,1,tdi22);
          TDI.assign_loc(1,0,tdi21);
          TDI.assign_loc(0,0,tdi33*tdi22);
          TDI.assign_loc(0,1,-tdi33*tdi21);
          TDI.assign_loc(2,0,-tdi13*tdi22);
          TDI.assign_loc(2,1,tdi13*tdi21);

          return TDI;
}
////////////////////////////////////////////////////////////////////////////////
//Returns the T.M. of earth wrt inertial coordinates 
//
// Return output
//			TEI = T.M. of Earthy wrt inertial coordinates 
//
// Argument input
//			time = time since start of simulation - s
//
//010628 Created by Peter H Zipfel
////////////////////////////////////////////////////////////////////////////////

Matrix cad_tei(const double &time)
{
   Matrix TEI(3,3);

   double xi=WEII3*time+GW_CLONG;
   double sxi=sin(xi);
   double cxi=cos(xi);

   TEI.identity();
   TEI.assign_loc(0,0, cxi); TEI.assign_loc(0,1, sxi);
   TEI.assign_loc(1,0,-sxi); TEI.assign_loc(1,1, cxi);

   return TEI;
}
///////////////////////////////////////////////////////////////////////////////
//Returns the T.M. of geographic wrt earth coordinates, TGE
// spherical Earth only
//
// Parameter input
//			lon = geographic longitude - rad
//			lat = geographic latitude - rad
//
//010628 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////

Matrix cad_tge(const double &lon,const double &lat)
{
          Matrix TGE(3,3);
          
          double clon = cos(lon);
          double slon = sin(lon);
          double clat = cos(lat);
          double slat = sin(lat);

          TGE.assign_loc(0,0,(-slat * clon));
          TGE.assign_loc(0,1,(-slat * slon));
          TGE.assign_loc(0,2,clat);
          TGE.assign_loc(1,0,-slon);
          TGE.assign_loc(1,1,clon);
          TGE.assign_loc(1,2,0.0);
          TGE.assign_loc(2,0,(-clat * clon));
          TGE.assign_loc(2,1,(-clat * slon));
          TGE.assign_loc(2,2,-slat);

          return TGE;
}
////////////////////////////////////////////////////////////////////////////////
//Returns the T.M. of geographic (geocentric) wrt inertial
// using the WGS 84 reference ellipsoid
// Reference: Britting,K.R."Inertial Navigation Systems Analysis",
// pp.45-49, Wiley, 1971
//
// Return output
//			       TGI(3x3) = T.M.of geographic wrt inertial coord - ND
// Parameter input
//			       lon = geodetic longitude - rad
//                 lat = geodetic latitude - rad
//                 alt = altitude above ellipsoid - m
//
//030414 Created from FORTRAN by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
Matrix cad_tgi84(const double &lon,const double &lat,const double &alt
                                ,const double &time)
{
          Matrix TDI(3,3);
          Matrix TGD(3,3);

          //celestial longitude of vehicle at simulation 'time' 
          double lon_cel=GW_CLONG+WEII3*time+lon;

          //T.M. of geodetic coord wrt inertial coord., TDI(3x3)
          double tdi13=cos(lat);
          double tdi33=-sin(lat);
          double tdi22=cos(lon_cel);
          double tdi21=-sin(lon_cel);
          TDI.assign_loc(0,2,tdi13);
          TDI.assign_loc(2,2,tdi33);
          TDI.assign_loc(1,1,tdi22);
          TDI.assign_loc(1,0,tdi21);
          TDI.assign_loc(0,0,tdi33*tdi22);
          TDI.assign_loc(0,1,-tdi33*tdi21);
          TDI.assign_loc(2,0,-tdi13*tdi22);
          TDI.assign_loc(2,1,tdi13*tdi21);

          //deflection of the normal, dd, and length of earth's radius to ellipse surface, R0
          double r0=SMAJOR_AXIS*(1.-FLATTENING*(1.-cos(2.*lat))/2.+5.*pow(FLATTENING,2)*(1.-cos(4.*lat))/16.); //eq 4-21
          double dd=FLATTENING*sin(2.*lat)*(1.-FLATTENING/2.-alt/r0); //eq 4-15

          //T.M. of geographic (geocentric) wrt geodetic coord.,TGD(3x3)
          TGD.assign_loc(0,0,cos(dd));
          TGD.assign_loc(2,2,cos(dd));
          TGD.assign_loc(1,1,1);
          TGD.assign_loc(2,0,sin(dd));
          TGD.assign_loc(0,2,-sin(dd));

          //T.M. of geographic (geocentric) wrt inertial coord.,	TGI(3x3)
          Matrix TGI=TGD*TDI;

          return TGI;
}
///////////////////////////////////////////////////////////////////////////////
//Returns the transformation matrix of inertial wrt perifocal coordinates
//
// Return output
//				TIP = TM of inertial wrt perifocal
// Parameter input
//				incl = inclination of orbital wrt equatorial plane - rad
//              lon_anode = celestial longitude of the ascending node - rad
//              arg_peri = argument of periapsis (ascending node to periapsis) - rad
//
//040510 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
Matrix cad_tip(const double &incl,const double &lon_anode,const double &arg_peri)			  
{
          //local variable
          Matrix TIP(3,3);
          
    double clon_anode=cos(lon_anode);
    double carg_peri=cos(arg_peri);
    double cincl=cos(incl);
    double slon_anode=sin(lon_anode);
    double sarg_peri=sin(arg_peri);
    double sincl=sin(incl);
    TIP.assign_loc(0,0,clon_anode*carg_peri-slon_anode*sarg_peri*cincl);
    TIP.assign_loc(0,1,-clon_anode*sarg_peri-slon_anode*carg_peri*cincl);
    TIP.assign_loc(0,2,slon_anode*sincl);
    TIP.assign_loc(1,0,slon_anode*carg_peri+clon_anode*sarg_peri*cincl);
    TIP.assign_loc(1,1,-slon_anode*sarg_peri+clon_anode*carg_peri*cincl);
    TIP.assign_loc(1,2,-clon_anode*sincl);
    TIP.assign_loc(2,0,sarg_peri*sincl);
    TIP.assign_loc(2,1,carg_peri*sincl);
    TIP.assign_loc(2,2,cincl);

          return TIP;
}
///////////////////////////////////////////////////////////////////////////////
////////////////////// Stochastic functions ///////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//Generating an exponential distribution with a given mean density
//Ref:
// Tybrin, "CADAC Program documentation", June 2000 and source code CADX3.FOR
// Numerical Recipies, p 287, 1992 Cambridge University Press
//Function unituni() is a CADAC++ utility
//
//parameter input:
//			density = # of events per unit of variable (in the mean)
//return output:
//			value = units of variable to be traversed until next event occurs
//
//The variance is density^2
//
//010919 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
double exponential(double density)
{
          double value;

          value=-log(unituni());
          if(!density)
          {cout<<" *** Error: density not given a non-zero value in 'exponential()' *** \n";system("pause");exit(1);}
          return value/density;
}///////////////////////////////////////////////////////////////////////////////
//Generating a standard distribution with 'mean' and 'sig' std deviation
//Ref Numerical Recipies, p 289, 1992 Cambridge University Press
//Function unituni() is a CADAC++ utility
//
//parameter input:
//			min = standard deviation of Gaussian distribution - unit of variable
//			mean = mean value of Gaussian distribution - unit of variable
//return output:
//			value = value of variable - unit of variable
//
//010913 Created by Peter H Zipfel
//010914 Normalized gauss tested with a 2000 sample: mean=0.0054, sigma=0.9759
///////////////////////////////////////////////////////////////////////////////
double gauss(double mean,double sig)
{
          static int iset=0;
          static double gset;
          double fac,rsq,v1,v2,value;

          if(iset==0){
                    do{
                              v1=2.*unituni()-1.;
                              v2=2.*unituni()-1.;
                              rsq=v1*v1+v2*v2;
                    }while(rsq>=1.0||rsq==0);

                    fac=sqrt(-2.*log(rsq)/rsq);
                    gset=v1*fac;
                    iset=1;
                    value=v2*fac;
          }
          else{
                    iset=0;
                    value=gset;
          }
          return value*sig+mean;
}
///////////////////////////////////////////////////////////////////////////////
//Generating a time-correlated Gaussian variable with zero mean
//Ref: CADAC Subroutine CNT_GAUSS
//Function gauss() is CADAC++ utility
//
//parameter input:
//			sigma = standard deviation of Gaussian distribution - unit of variable
//			bcor = beta time correlation coefficient - 1/s (Hz)
//			time = simulation time - s
//			intstep = integration step size - s
//			value_saved = value of previous integration step
//return output:
//			value = value of variable - unit of variable
//
//010914 Created by Peter H Zipfel
//020723 Replaced static variable by '&value_saved', PZi
///////////////////////////////////////////////////////////////////////////////
double markov(double sigma,double bcor,double time,double intstep,double &value_saved)
{
          double value=0;

          value=gauss(0.,sigma);
          if(time==0.) value_saved=value;
          else{
                    if(bcor!=0.)
                    {
                              double dum=exp(-bcor*intstep);
                              double dumsqrd=dum*dum;
                              value=value*sqrt(1.-dumsqrd)+value_saved*dum;
                              value_saved=value;
                    }
          }
          return value;
}
///////////////////////////////////////////////////////////////////////////////
//Generating a Rayleigh distribution with peak value of pdf = 'mode'
//Ref: Tybrin, "CADAC Program documentation", June 2000 and source code CADX3.FOR
//Function unituni() is a CADAC++ utility
//
//parameter input:
//			mode= mode (peak value of pdf) of Rayleigh distribution - unit of variable
//return output:
//			value=value of variable - unit of variable
//
//The mean of the distribution is: mean = mode * (pi/2)
//The variance is: variance = mode^2 * (2 - pi/2)
//
//010918 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
double rayleigh(double mode)
{
          double value;

          value=sqrt(2.*(-log(unituni())));
          return value*mode;
}

///////////////////////////////////////////////////////////////////////////////
//Generating uniform random distribution between 'min' and 'max' 
//
//010913 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
double uniform(double min,double max)
{
          double value;
          value=min+(max-min)*unituni();
          return value;
}
///////////////////////////////////////////////////////////////////////////////
//Generating uniform random distribution between 0-1 based on C function rand()
//
//010913 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
double unituni()
{
          double value;
          value=(double)rand()/RAND_MAX;
          return value;
}
///////////////////////////////////////////////////////////////////////////////
//////////////// Table look-up and interpolation functions ////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//Single independent variable look-up
//Constant extrapolation at the upper end, slope extrapolation at the lower end
//
//030717 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
double Datadeck::look_up(string name,double value1)
{
          //finding slot of table in table pointer array (Table **table_ptr) 
          int slot(-1);
          string tbl_name;
          do{
                    slot++;
                    tbl_name=get_tbl(slot)->get_name();
          }while(name!=tbl_name);

                    //getting table index locater of discrete value just below of variable value
          int var1_dim=get_tbl(slot)->get_var1_dim();
          int loc1=find_index(var1_dim-1,value1,get_tbl(slot)->var1_values);

          //using max discrete value if value is outside table
          if (loc1==(var1_dim-1)) return get_tbl(slot)->data[loc1];
                    
          return interpolate(loc1,loc1+1,slot,value1);
}
///////////////////////////////////////////////////////////////////////////////
//Two independent variables look-up
//constant extrapolation at the upper end, slope extrapolation at the lower end
//
//030717 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
double Datadeck::look_up(string name,double value1,double value2)
{
          //finding slot of table in table pointer array (Table **table_ptr) 
          int slot(-1);
          string tbl_name;
          do{
                    slot++;
                    tbl_name=get_tbl(slot)->get_name();
          }while(name!=tbl_name);

          
          //getting table index (off-set) locater of discrete value just below or equal of the variable value
          int var1_dim=get_tbl(slot)->get_var1_dim();
          int loc1=find_index(var1_dim-1,value1,get_tbl(slot)->var1_values);

          int var2_dim=get_tbl(slot)->get_var2_dim();
          int loc2=find_index(var2_dim-1,value2,get_tbl(slot)->var2_values);
                    
          return interpolate(loc1,loc1+1,loc2,loc2+1,slot,value1,value2);
}
///////////////////////////////////////////////////////////////////////////////
//Three independent variables look-up
//constant extrapolation at the upper end, slope extrapolation at the lower end
//
//030723 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
double Datadeck::look_up(string name,double value1,double value2,double value3)
{
          //finding slot of table in table pointer array (Table **table_ptr) 
          int slot(-1);
          string tbl_name;
          do{
                    slot++;
                    tbl_name=get_tbl(slot)->get_name();
          }while(name!=tbl_name);

          
          //getting table index locater of discrete value just below of variable value
          int var1_dim=get_tbl(slot)->get_var1_dim();
          int loc1=find_index(var1_dim-1,value1,get_tbl(slot)->var1_values);

          int var2_dim=get_tbl(slot)->get_var2_dim();
          int loc2=find_index(var2_dim-1,value2,get_tbl(slot)->var2_values);
                    
          int var3_dim=get_tbl(slot)->get_var3_dim();
          int loc3=find_index(var3_dim-1,value3,get_tbl(slot)->var3_values);
                    
          return interpolate(loc1,loc1+1,loc2,loc2+1,loc3,loc3+1,slot,value1,value2,value3);
}
///////////////////////////////////////////////////////////////////////////////
//Table index finder
//This is a binary search method it is O(lgN)
// * Returns array locater (offset index) of the discrete_variable just below variable
// * Keeps max or min array locater if variable is outside those max or min  
//
//030717 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
int Datadeck::find_index(int max,double value,double *list)
{
          if(value>=list[max])
                    return max;
          else if (value<=list[0]){
                    return 0;
          }
          else{
                    int index=0;
                    int mid;
                    while(index<=max){
                              mid=(index+max)/2;		//integer division
                              if(value<list[mid])
                                        max=mid-1;
                              else if(value>list[mid])
                                        index=mid+1;
                              else
                                        return mid;
                    }
                    return max;
          }
}
///////////////////////////////////////////////////////////////////////////////
//Linear one-dimensional interpolation
//Data deck must contain table in the following format:
//
// X1       Table Values(X1)
//
// X11		Y11
// X12		Y12
// X13		Y13
//           
// * Constant extrapolation beyond max values of X1
// * Slope extrapolation beyond min values of X1
//
//030717 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
double Datadeck::interpolate(int ind1,int ind2,int slot,double val)
{
          double dx(0),dy(0);
          double dumx(0);

          double diff=val-get_tbl(slot)->var1_values[ind1];
          dx=get_tbl(slot)->var1_values[ind2]-get_tbl(slot)->var1_values[ind1];
          dy=get_tbl(slot)->data[ind2]-get_tbl(slot)->data[ind1];

          if(dx>EPS) dumx=diff/dx;
          dy=dumx*dy;

          return get_tbl(slot)->data[ind1]+dy;
}
///////////////////////////////////////////////////////////////////////////////
//Linear, two-dimensional interpolation
//File must contain table in the following form:
//
//  X1  X2  //Table Values(X1-row, X2-column)
//            ---------------
//  X11 X21   |Y11  Y12  Y13| 
//  X12 X22   |Y21  Y22  Y23|    <- data
//  X13 X23   |Y31  Y32  Y33| 
//  X14       |Y41  Y42  Y43| 
//            ---------------
//Constant extrapolation beyond max values of X1 and X2
//Slope extrapolation beyond min values of X1 and X2
//
//030718 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
double Datadeck::interpolate(int ind10,int ind11,int ind20,int ind21,int slot,double value1,
                                                  double value2)																
{
          double dx1(0),dx2(0);
          double dumx1(0),dumx2(0);

          int var1_dim=get_tbl(slot)->get_var1_dim();
          int var2_dim=get_tbl(slot)->get_var2_dim();

          double diff1=value1-get_tbl(slot)->var1_values[ind10];;
          double diff2=value2-get_tbl(slot)->var2_values[ind20];

          if(ind10==(var1_dim-1)) //Assures constant upper extrapolation of first variable
                    ind11=ind10;
          else
                    dx1=get_tbl(slot)->var1_values[ind11]-get_tbl(slot)->var1_values[ind10];

          if(ind20==(var2_dim-1)) //Assures constant upper extrapolation of second variable
                    ind21=ind20;
          else
                    dx2=get_tbl(slot)->var2_values[ind21]-get_tbl(slot)->var2_values[ind20];

          if(dx1>EPS) dumx1=diff1/dx1;		
          if(dx2>EPS) dumx2=diff2/dx2;
                    
          double y11=get_tbl(slot)->data[ind10*var2_dim+ind20];
          double y12=get_tbl(slot)->data[ind10*var2_dim+ind21];
          double y21=get_tbl(slot)->data[ind11*var2_dim+ind20];
          double y22=get_tbl(slot)->data[ind11*var2_dim+ind21];
          double y1=dumx1*(y21-y11)+y11;
          double y2=dumx1*(y22-y12)+y12;

          return dumx2*(y2-y1)+y1;
}
///////////////////////////////////////////////////////////////////////////////
//Linear, three-dimensional interpolation
//File must contain table in the following form:
//
//  X1  X2  X3    Table Values(X1-row, X2-block, X3-column) <- don't type (illustration only)
//
//                (X1 x X3) (X1 x X3) (X1 x X3) (X1 x X3)	<- don't type 
//				   for X21   for X22   for X23   for X24	<- don't type 
//               -----------------------------------------
//  X11 X21 X31  |Y111 Y112|Y121 Y122|Y131 Y132|Y141 Y142|  
//  X12 X22 X32  |Y211 Y212|Y221 Y222|Y231 Y232|Y241 Y242|  <- data; don't type: '|'
//  X13 X23      |Y311 Y312|Y321 Y322|Y331 Y332|Y341 Y342| 
//      X24      ----------------------------------------- 
//               
//Constant extrapolation beyond max values of X1, X2 and X3
//Slope extrapolation beyond min values of X1, X2 and X3
//
//030723 Created and corrected by Peter Zipfel
///////////////////////////////////////////////////////////////////////////////
double Datadeck::interpolate(int ind10,int ind11,int ind20,int ind21,int ind30,int ind31,
                                                                       int slot,double value1,double value2,double value3)
{
          double dx1(0),dx2(0),dx3(0);
          double dumx1(0),dumx2(0),dumx3(0);

          int var1_dim=get_tbl(slot)->get_var1_dim();
          int var2_dim=get_tbl(slot)->get_var2_dim();
          int var3_dim=get_tbl(slot)->get_var3_dim();

          double diff1=value1-get_tbl(slot)->var1_values[ind10];;
          double diff2=value2-get_tbl(slot)->var2_values[ind20];
          double diff3=value3-get_tbl(slot)->var3_values[ind30];

          if(ind10==(var1_dim-1)) //Assures constant upper extrapolation of first variable
                    ind11=ind10;
          else
                    dx1=get_tbl(slot)->var1_values[ind11]-get_tbl(slot)->var1_values[ind10];

          if(ind20==(var2_dim-1)) //Assures constant upper extrapolation of second variable
                    ind21=ind20;
          else
                    dx2=get_tbl(slot)->var2_values[ind21]-get_tbl(slot)->var2_values[ind20];

          if(ind30==(var3_dim-1)) //Assures constant upper extrapolation of third variable
                    ind31=ind30;
          else
                    dx3=get_tbl(slot)->var3_values[ind31]-get_tbl(slot)->var3_values[ind30];

          if(dx1>EPS) dumx1=diff1/dx1;		
          if(dx2>EPS) dumx2=diff2/dx2;
          if(dx3>EPS) dumx3=diff3/dx3;
          //int ind10,int ind11,int ind20,int ind21,int ind30,int ind31
          //      i        i+1        j         j+1       k        k+1
          // Use innner x1 and outer variable x3 for 2DIM interpolation, middle variable x2 is parameter
          // For parameter ind20
          double y11=get_tbl(slot)->data[ind10*var2_dim*var3_dim+ind20*var3_dim+ind30];
          double y12=get_tbl(slot)->data[ind10*var2_dim*var3_dim+ind20*var3_dim+ind30+var2_dim*var3_dim];
          double y31=get_tbl(slot)->data[ind10*var2_dim*var3_dim+ind20*var3_dim+ind31];
          double y32=get_tbl(slot)->data[ind10*var2_dim*var3_dim+ind20*var3_dim+ind31+var2_dim*var3_dim];
          //2DIM interpolation
          double y1=dumx1*(y12-y11)+y11;
          double y3=dumx1*(y32-y31)+y31;
          double y21=dumx3*(y3-y1)+y1;

          // For parameter ind21
          y11=get_tbl(slot)->data[ind10*var2_dim*var3_dim+ind21*var3_dim+ind30];
          y12=get_tbl(slot)->data[ind10*var2_dim*var3_dim+ind21*var3_dim+ind30+var2_dim*var3_dim];
          y31=get_tbl(slot)->data[ind10*var2_dim*var3_dim+ind21*var3_dim+ind31];
          y32=get_tbl(slot)->data[ind10*var2_dim*var3_dim+ind21*var3_dim+ind31+var2_dim*var3_dim];
          //2DIM interpolation
          y1=dumx1*(y12-y11)+y11;
          y3=dumx1*(y32-y31)+y31;
          double y22=dumx3*(y3-y1)+y1;

          //1DIM interpolation between the middle variable 
          return dumx2*(y22-y21)+y21;
}

///////////////////////////////////////////////////////////////////////////////
////////////////////  Integration functions  //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//Integration of scalar state variable
//Modified Euler Midpoint method
//Example first order lag:
//			phid_new=(phic-phi)/tphi;
//			phi=integrate(phid_new,phid,phi,int_step);
//			phid=phid_new;
//010628 Created by Peter H Zipfel
//050202 Simplified and renamed to Modified Euler method, PZi 
///////////////////////////////////////////////////////////////////////////////
double integrate(const double &dydx_new,const double &dydx,const double &y,const double &int_step)
{
          return y+(dydx_new+dydx)*int_step/2;
}
///////////////////////////////////////////////////////////////////////////////
//Integration of Matrix MAT(r,c) 
//
//030424 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
Matrix integrate(Matrix &DYDX_NEW,Matrix &DYDX,Matrix &Y,const double int_step)
{
          int nrow=Y.get_rows();int nrow1=DYDX_NEW.get_rows();int nrow2=DYDX.get_rows();
          int ncol=Y.get_cols();int ncol1=DYDX_NEW.get_cols();int ncol2=DYDX.get_cols();

          if(nrow!=nrow1||nrow!=nrow2)
                    {cerr<<" *** Error: incompatible row-dimensions in 'integrate()' *** \n";system("pause");exit(1);}
          if(ncol!=ncol1||ncol!=ncol2)
                    {cerr<<" *** Error: incompatible column-dimensions in 'integrate()' *** \n";system("pause");exit(1);}

          Matrix RESULT(nrow,ncol);
          for(int r=0;r<nrow;r++)
                    for(int c=0;c<ncol;c++)
                              RESULT.assign_loc(r,c,integrate(DYDX_NEW.get_loc(r,c)
                              ,DYDX.get_loc(r,c),Y.get_loc(r,c),int_step));

          return RESULT;
}

///////////////////////////////////////////////////////////////////////////////
// US Standard Atmosphere 1976 (Public Domain)
// *Calculates the atmospheric properties density pressure and temperature 
//	up to 85 km.
// *Extrapolation above 71 km and beyond 85 km is carried out from 71 km altitude 
// Ref: Public Domain Aeronautical Software (see Web) Fortran Code
//
// Argument output:
//					rho=Air density - kg/m^3
//					press= Air static pressure - Pa
//					tempk= Air temperature - degKelvin
// Argument input:
//					balt= Geometrical altitude above S.L. - m
//
// 030318 Created by Peter Zipfel
// 040310 Implemented constant values above stratosphere, PZi
///////////////////////////////////////////////////////////////////////////////
#include <cmath>

void atmosphere76(double &rho,double &press,double &tempk, const double balt)
{

          double rearth(6369.0); //radius of the earth - km
          double gmr(34.163195); //gas constant
          double rhosl(1.22500); //sea level density - kg/m^3
          double pressl(101325.); //sea level pressure - Pa
          double tempksl(288.15); //sea level temperature - dK
          double htab[8]={0.0, 11.0, 20.0, 32.0, 47.0, 51.0, 71.0, 84.852}; //altitude
          double ttab[8]={288.15, 216.65, 216.65, 228.65, 270.65, 270.65, 214.65, 186.946}; //temperature
          double ptab[8]={1.0, 2.233611e-1, 5.403295e-2, 8.5666784e-3, 1.0945601e-3, 6.6063531e-4, 3.9046834e-5, 3.68501e-6};  //pressure
          double gtab[8]={-6.5, 0.0, 1.0, 2.8, 0.0, -2.8, -2.0, 0.0};   //temperature gradient

          double delta(0);

          //convert geometric (m) to geopotential altitude (km)
          double alt=balt/1000; 
          double h=alt*rearth/(alt+rearth);

          //binary search determines altitude table entry i below actual altitude 
          int i(0); //offset of first value in table
          int j(7); //offset of last value in table
          for( ; ; )
          {
                    int k=(i+j)/2;     //integer division
                    if(h<htab[k])
                    {
                              j=k;
                    }

                    else
                    {
                              i=k;
                    }
                    if(j<=(i+1))
                    {
                              break;
                    }
          }
          //within stratosphere
          if(alt<84.852)
          {

                    //normalized temperature 'theta' from table look-up and gradient interpolation
                    double tgrad=gtab[i];
                    double tbase=ttab[i];
                    double deltah=h-htab[i];
                    double tlocal=tbase+tgrad*deltah;
                    double theta=tlocal/ttab[0]; 

                    //normalized pressure from hydrostatic equations 
                    if(tgrad==0)
                    {
                              delta=ptab[i]*exp(-gmr*deltah/tbase);
                    }
                    else
                    {
                              delta=ptab[i]*pow((tbase/tlocal),(gmr/tgrad));
                    }

                    //normalized density
                    double sigma=delta/theta;

                    //output
                    rho=rhosl*sigma;
                    press=pressl*delta;
                    tempk=tempksl*theta;

          }
          else
          {
                    //beyond stratosphere
                    rho=0;
                    press=0;
                    tempk=186.946;
          }

}
///////////////////////////////////////////////////////////////////////////////
// US Standard Atmosphere 1976 (NASA Marshall)
// *Calculates the atmospheric properties density, pressure and temperature 
//	with extensions up to 1000 km
//	There are actually two atmosphere modeling algorithms in this function,
//   one for altitudes less than 86 kilometers and the other for higher altitudes. 
// Source: NASA Marshall Space Flight Center
//
// Argument output:
//					d= density - kg/m^3
//					p= static pressure - Pa
//					t= temperature - degKelvin
//					s= speed of sound - m/s
// Return output:
//					=0 altitude is within tabular range
//					=1 altitude is outside tabular range
// Argument input:
//					z= Geometric altitude above S.L. - km
//
// 040311 Adopted and included clarifying comments, PZi
///////////////////////////////////////////////////////////////////////////////
/*
          $Log: us76.c,v $
 * Revision 0.1  2002/03/26  17:07:42  adamswa
 * J. McCarter
 *
*/

#include <cmath>

/* 1976 US standard atmospheric model   */
int us76_nasa2002(
   double  z,   /* altitude (km)        */
   double* d,   /* density (kg/m^3)     */
   double* p,   /* pressure (Pa)        */
   double* t,   /* temperature (dKelvin)*/
   double* s    /* speed of sound (m/s) */
)
{
   double zs[49] = {    /* altitude independent variable (km) */
      0.,       11.019, 20.063, 32.162, 47.35,  
      51.413,   71.802, 86.,    91.,    94.,    
      97.,      100.,   103.,   106.,   108.,   
      110.,     112.,   115.,   120.,   125.,   
      130.,     135.,   140.,   145.,   150.,   
      155.,     160.,   165.,   170.,   180.,   
      190.,     210.,   230.,   265.,   300.,   
      350.,     400.,   450.,   500.,   550.,   
      600.,     650.,   700.,   750.,   800.,   
      850.,     900.,   950.,   1000.
   };

   double tms[49] = {
      288.15,   216.65, 216.65, 228.65, 270.65, 
      270.65,   214.65, 186.95, 186.87, 187.74, 
      190.40,   195.08, 202.23, 212.89, 223.29, 
      240.00,   264.00, 300.00, 360.00, 417.23, 
      469.27,   516.59, 559.63, 598.78, 634.39, 
      666.80,   696.29, 723.13, 747.57, 790.07, 
      825.31,   878.84, 915.78, 955.20, 976.01, 
      990.06,   995.83, 998.22, 999.24, 999.67, 
      999.85,   999.93, 999.97, 999.99, 999.99, 
      1000.,    1000.,  1000.,  1000.
   };

   double wms[49] = {
      28.9644,  28.9644,        28.9644,        28.9644,        28.9644,
      28.9644,  28.9644,        28.9522,        28.8890,        28.7830,
      28.6200,  28.3950,        28.1040,        27.7650,        27.5210,
      27.2680,  27.0200,        26.6800,        26.2050,        25.8030,
      25.4360,  25.0870,        24.7490,        24.4220,        24.1030,
      23.7920,  23.4880,        23.1920,        22.9020,        22.3420,
      21.8090,  20.8250,        19.9520,        18.6880,        17.7260,
      16.7350,  15.9840,        15.2470,        14.3300,        13.0920,
      11.5050,  9.7180,         7.9980,         6.5790,         5.5430,
       4.8490,  4.4040,         4.1220,         3.9400
   };

   double ps[49] = {
      1013.25,          226.32,         54.7487,        8.68014,        
      1.10905,          0.66938,        0.039564,       3.7338e-03,
      1.5381e-03,       9.0560e-04,     5.3571e-04,     3.2011e-04,
      1.9742e-04,       1.2454e-04,     9.3188e-05,     7.1042e-05,
      5.5547e-05,       4.0096e-05,     2.5382e-05,     1.7354e-05,
      1.2505e-05,       9.3568e-06,     7.2028e-06,     5.6691e-06,
      4.5422e-06,       3.6930e-06,     3.0395e-06,     2.5278e-06,
      2.1210e-06,       1.5271e-06,     1.1266e-06,     6.4756e-07,
      3.9276e-07,       1.7874e-07,     8.7704e-08,     3.4498e-08,
      1.4518e-08,       6.4468e-09,     3.0236e-09,     1.5137e-09,
      8.2130e-10,       4.8865e-10,     3.1908e-10,     2.2599e-10,     
      1.7036e-10,       1.3415e-10,     1.0873e-10,     8.9816e-11,
      7.5138e-11
   };

   double ro  = 6356.766;       /* radius of Earth - km */
   double go  = 9.80665;        /* nominal gravitational acceleration - m/s^2  */
   double wmo = 28.9644;        /* molecular weight - ND */
   double rs  = 8314.32;        /* universal gas constant (m^2/s^2) (gram/mole) / (degree Kelvin) */

   double alp0;         /*  */
   double alp1;         /*  */
   double alp2;         /*  */
   double alpa;         /*  */
   double alpb;         /*  */
   double g;            /*  */
   double ht;           /*  */
   double wm;           /*  */
   double wma;          /*  */
   double wmb;          /*  */
   double xi;           /*  */
   double z0;           /*  */
   double z1;           /*  */
   double z2;           /*  */
   double zl;           /* z-lower (altitude below z in table) */
   double zu;           /* z-upper (altitude above z in table) */

   int    i,j;          /*  */

   /* check to see if input altitude is in range, return 1 if not */
   if(z < 0. || z > 1000.) {
      *t = 0.;
      *p = 0.;
      *d = 0.;
      *s = 0.;
      return 1;
   }

   /* bisection search for i such that zs[i] <= z < zs[i+1] */
   {
      int upper=48;
      int test;
      i = 0;
      while ( upper-i > 1 ) {
         test = (i+upper) >> 1;
         if ( z > zs[test] ) i = test; else upper = test;
      }
   }

   if( i < 7 ) {

      zl = ro * zs[i  ]/(ro + zs[i  ]);
      zu = ro * zs[i+1]/(ro + zs[i+1]);
      wm = wmo;
      ht = (ro * z)/(ro + z);
      g = (tms[i+1]-tms[i])/(zu-zl);

      if(g < 0. || g > 0.) {
        *p=ps[i]*pow((tms[i]/(tms[i]+g*(ht-zl))),((go*wmo)/(rs*g*0.001)))*100.;
      } else {
        *p=ps[i] * exp(-(go*wmo*(ht*1000.-zl*1000.))/(rs*tms[i])) * 100.;
      }
      *t = tms[i] + g * (ht-zl);
   
   } else {
   
      if(i == 7) {
         *t = tms[8];
      }

      if(i >= 8 && i < 15) {
         *t = 263.1905-76.3232 * sqrt(1.-pow((z-91.)/19.9429,2.));
      }

      if(i >= 15 && i < 18) {
          *t = 240. + 12. * (z-110.);
      }

      if(i >= 18) {
         xi = (z-120.) * (ro + 120.)/(ro + z);
         *t = 1000.-640. * exp(-0.01875 * xi);
      }

      j = i;

      if (i == 47) j = i-1;

      z0 = zs[j  ];
      z1 = zs[j+1];
      z2 = zs[j+2];
      wma = wms[j  ] * (z-z1) * (z-z2)/((z0-z1) * (z0-z2)) + 
            wms[j+1] * (z-z0) * (z-z2)/((z1-z0) * (z1-z2)) + 
            wms[j+2] * (z-z0) * (z-z1)/((z2-z0) * (z2-z1));
      alp0 = log(ps[j]);
      alp1 = log(ps[j+1]);
      alp2 = log(ps[j+2]);
      alpa = alp0 * (z-z1) * (z-z2)/((z0-z1) * (z0-z2)) + 
             alp1 * (z-z0) * (z-z2)/((z1-z0) * (z1-z2)) + 
             alp2 * (z-z0) * (z-z1)/((z2-z0) * (z2-z1));
      alpb = alpa;
      wmb = wma;

      if(i!=7 && i!=47) {
         j = j-1;
         z0 = zs[j  ];
         z1 = zs[j+1];
         z2 = zs[j+2];
         alp0 = log(ps[j  ]);
         alp1 = log(ps[j+1]);
         alp2 = log(ps[j+2]);
         alpb = alp0 * (z-z1) * (z-z2)/((z0-z1) * (z0-z2)) + 
                alp1 * (z-z0) * (z-z2)/((z1-z0) * (z1-z2)) + 
                alp2 * (z-z0) * (z-z1)/((z2-z0) * (z2-z1));
         wmb =  wms[j  ] * (z-z1) * (z-z2)/((z0-z1) * (z0-z2)) + 
                wms[j+1] * (z-z0) * (z-z2)/((z1-z0) * (z1-z2)) + 
                wms[j+2] * (z-z0) * (z-z1)/((z2-z0) * (z2-z1));
      }

      *p = 100. * exp((alpa + alpb)/2.);
      wm = (wma + wmb)/2.;
   
   }
   
   *d = (wm * *p)/(rs * *t);
   *s = sqrt(1.4 * *p / *d);

   return 0;  /* normal return, altitude in range */
}



