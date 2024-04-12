/*
    Author: Mylonakis Emmanouil
    Registration Number: 2015030079
    Date: 02/03/2021
    Operating System: Windows 10.

    Description: Exercise 1. 
                 Newton-Raphson's method emplemented with two different ways:
                 a) Calculating function's derivation analytically.
                 b) Calculating function's derivation arithmetically via Dy/Dx.
                 Polynomial up to degree 5.
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

void question_a(int, float *); //Method for request A. Calculate derivation analytically
void question_b(int, float *); //Method for request D. Calculate derivation aritmetically via Dy/Dx.
void print_stats(int*);
float funct_calc(int, float *, float); //Method for calculation the value of polynomial for a certain x_0
float deriv_calc(int, float*, float); // Method for calculation the value of polynomial's derivation for a certain x_0

int stats[4]; //To store statistics like the number of loops, adds/sub, muls and divs.

int main()
{
    // ================================ Drive Code =============================================//
    int degree=-1;
    //Ask user's input. If it's wrong then try again.
    while(degree <=0 || degree >5)
    {
        printf("Enter polynomial's degree:");
        scanf("%d", &degree);
        
        if(degree<=0 || degree>5)
            printf("Wrong input. Try again in range (0,5].\n");
    }

    //Buffer for the coefficients user's desire.
    char coef[256];

    getchar(); /*<-- This is tricky. scanf as final step for its operation
                     puts the char '\n' back to stdin's buffer. fgets consumes the character \n
                     so won't work after scanf's call. By using getchar we consume \n in order
                     for fgets works properly.
               */

    //Ask user for coefficients.                
    printf("Enter polynomial's coefficients:");
    fgets(coef, sizeof(coef), stdin);

    //Code below seperate the user's input string with delimeter space (" ").
    char delim[] = " ";
    char *token;
    float C[6]; //To store coefficients. Up to 6 as long as our polynomial has degree up to 5.
    int i = 0;

    token = strtok(coef, delim);//We receive the first coefficient. 
    //In loop below, we store the rest coefficients.
    while(token != NULL)
    {
        C[i] = atof(token); //alphanumeric to float.
        token = strtok(NULL, delim); // Null because strtok func stores and remembers the last string we used for seperation.
        i++;
    }

    // ================= Main Functionality  ================= //
    
    printf("========== Question a. ==========\n");
    question_a(degree, C);
    printf("========== Question b. ==========\n");
    question_b(degree, C);

    return 0;
}


void question_a(int deg, float *C)
{   
   int max = 20;
   float error = 0.001; //Tolerance 10^(-3).
   float x_0 = 2.5; //Assuming that in our circuit exists voltage in range [0-5] Volt, we choose x_0 = 2.5 for loop's starting.
   float funct;     //For f(x)
   float deriv;     //For f'(x)
   float h;         //The step h for the new x.

   stats[0] = 0; //0 -> #loops
   stats[1] = 0; //1 -> #adds/subs 
   stats[2] = 0; //2 -> #muls
   stats[3] = 0; //3 -> #divs
   
   printf("Starting with x_0 = 2.5\n");
   while(max>0)
   {
        stats[0]++;

        funct = funct_calc(deg, C, x_0);     // Step 1. Calculate f(x).
        deriv = deriv_calc(deg, C, x_0);     // Step 2. Calculate f'(x).
        h = -(funct / deriv);                // Step 3. Calculate h = - f(x)/f'(x).

        stats[2]++; // For the multiplication with -1.
        stats[3]++;

        x_0 = x_0 + h; //Step 4. Calculate x_(n+1) = x_n + h.
        stats[1]++;

        printf("Loop %d. x_0 = %f => P(x_0) = %f\n", stats[0], x_0, funct);

        if(fabs(h)<error) break;
                
        max--;
   }

   printf("Final. x_0 = %f => P(x_0) = %f\n",  x_0, funct);
   print_stats(stats);
}

void question_b(int deg, float *C)
{
    int max = 20;
    float error = 0.001; //Tolerance 10^(-3).
    float x_0 = 2.5;     //Assuming that in our circuit exists voltage in range [0-5] Volt, we choose x_0 = 2.5 for loop's starting.
    float dx = 0.000001;     //The small delta.
    float funct1;        //For f(x)
    float funct2;        //For f(x+dx)
    float deriv;         //For f'(x)
    float h;             //The step h for the new x.

    stats[0] = 0; //0 -> #loops
    stats[1] = 0; //1 -> #adds/subs
    stats[2] = 0; //2 -> #muls
    stats[3] = 0; //3 -> #divs

    printf("Starting with x_0 = 2.5\n");
    while (max > 0)
    {
        stats[0]++;

        funct1 = funct_calc(deg, C, x_0);      // Step 1. Calculate f(x).
        funct2 = funct_calc(deg, C, x_0 + dx); // Step 2. Calculate f(x+dx).
        stats[1]++;                            //One addition x+dx.
        
        deriv = (funct2 - funct1) / dx;        // Step 3. Calculate f'(x) = (f(x+dx)- f(x)) / dx.
        stats[1]++;                            //One substraction.
        stats[3]++;                            //One division.
        
        h = -(funct1 / deriv);
        stats[2]++; // For the multiplication with -1.
        stats[3]++;

        x_0 = x_0 + h; //Step 4. Calculate x_(n+1) = x_n + h.
        stats[1]++;

        printf("Loop %d. x_0 = %f => P(x_0) = %f\n", stats[0], x_0, funct1);

        if (fabs(h) < error) break;

        max--;
    }

    printf("Final. x_0 = %f => P(x_0) = %f\n", x_0, funct1);
    print_stats(stats);
    printf("Bye ! Press any key to terminate ...\n");
    getchar();
}

//Method for calculation the value of polynomial for a certain x_0.
float funct_calc(int deg, float* C, float x)
{
    float funct_new = 0;
    float pow_x;

    for(int i=0; i<=deg; i++) //Loop for the number of coefficients. If our polunomial has degree=deg then it has deg+1 coefficients.
    {
        pow_x = powf(x, (float)(deg-i)); //x in power of deg-i.If deg=5 then in first loop deg-i=5, in second deg-i=4 ... downto deg-i=0. 
        if(deg-i > 1)               //if power of x is greater than 1. Let (deg-i)=a, we calculated x^a.
            stats[2] += (deg-i)-1;  //we need a-1 multiplications wich is equivelant to (deg-i)-1
        
        funct_new += C[i]*pow_x; //Multiply the respective coefficient with the calculated power. Add result to the previous one.
        stats[1]++; //Update statistics.
        stats[2]++;
    }
    /*                                   deg
                                         ----
                                         \     a_i * x^(b_i) , where a_i are the coefficients from array C[i]
      To sum up the above logic is like: /                     and b_i = deg-i.
                                         ----
                                         i=0 
      
    */
    return funct_new; //Return the evaluation of polynomial.
}

// Method for calculation the value of polynomial's derivation for a certain x_0.
float deriv_calc(int deg, float* C, float x)
{
    float deriv_new = 0;
    float pow_x;

    for (int i = 0; i < deg; i++)
    {
        pow_x = powf(x, (deg-i)-1);
        if ((deg - i)-1 > 1)
            stats[2] += (deg-i)-2;

        deriv_new += ((float)(deg - i)) * C[i] * pow_x;
        stats[1]+=2; //Also for deg-i.
        stats[2]+=2;
    }

    /*                                   deg
                                         ----
                                         \     a_i * b_i* x^(b_i-1) , where a_i are the coefficients from array C[i]
      To sum up the above logic is like: /                            and b_i = deg-i.
                                         ----
                                         i=0 
      Exact the same logic with the above method funct_calc(...).
    */

    return deriv_new;//Return evaluation.
}

void print_stats(int *S)
{
    printf("Number of Loops:           %d\n", stats[0]);
    printf("Number of Adds/Subtracts:  %d\n", stats[1]);
    printf("Number of Multiplications: %d\n", stats[2]);
    printf("Number of Divisions:       %d\n", stats[3]);
}