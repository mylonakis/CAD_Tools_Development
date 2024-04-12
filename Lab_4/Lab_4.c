/*
    Author: Mylonakis Emmanouil
    Registration Number: 2015030079
    Date: 09/04/2021
    Operating System: Windows 10.

    Description: Exercise 4. Create 2 Datasets by spliting one with DAG colouring technic.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

/* ========== FOR MAIN  FUNCTIONALITY ========== */
void initiate_DAG(int x); //Initial state of graph.
void find_correct_splitting(); //Decides which split is "illegal".
int find_splitting_point();    //It Splits the Graph.
void define_new_INs_OUTs(int point); //New Inputs/Outputs for the derived files.
void split_INPUT_OUTPUT(int point); //Split INPUT/OUTPUTS from original file.
void derive_files(FILE* fp, char *name, int point);    //It Splits the Graph.
bool is_node_D_or_S_before(int node, int until); //Checks if node is Drain Or Source.
bool is_node_D_or_S_after(int node, int from);   //Checks if node is Drain Or Source.


/* ========== UTILITY FUNCTIONS ========== */
void checkInput(int argc, char *argv[]);      // Checks User's Input Correcteness.
void usage();                                //Prints a help message for user.
void my_test();                                //Prints a help message for user.
void store_RAILS(char *buff);                //Update infos from section ## RAILS.
void store_circuit_infos(FILE *fp);          //Stores from file every info until the section ## TESTBENCH.
int call_strtok(char *line, int **array);    // Calls repeattadly strtok() function. Allocate array's size properly and returns the number of rows.
bool is_exist_in(int *Array, int size, int value);
void free_pointer(); //free memory before program terminate.
void create_every_vector(FILE *fp, int quantity, int lenght);

int VCC, GND;                  // Section ## RAILS, store nodes for VCC and GNB.
int *INPUT = NULL;             // Section ## INPUTS, store input nodes dynamically.
int *OUTPUT = NULL;            // Section ## OUTPUTS, store output nodes dynamically.
int **DAG = NULL;              // Section ## NETLIST, store infos for every transistor dynamically.
int numOfInputs, numOfOutputs; //Number of Inputs and Outputs our circuit has.
int numOfMos = 0;              //Number of Transistor in our netlist.

int *new_IN1 = NULL, *new_IN2 = NULL; //Inputs for first and second circuit
int in1, in2; //Number of Inputs for first and second circuit respectively.
int *new_OUT1 = NULL, *new_OUT2 = NULL; //Outputs for first and second circuit
int out1, out2; //Number of Outputs for first and second circuit respectively.

int main(int argc, char *argv[])
{
    checkInput(argc, argv); //Check input's correctness.

    FILE* fp = fopen(argv[1], "r");
    
    if(fp == NULL)
    {
        printf("Error: fopen failed :(\n");
        exit(1);
    }

    store_circuit_infos(fp);
    initiate_DAG(1);//Initiate colors with candidate splitting-nodes
    /*
        Remove the node which is hilighted as splitting because of the initiate
        as we will decide that the specific split is "illegal".
    */
    find_correct_splitting();
    int point = find_splitting_point();

    if (DAG[point][5] == 'B') //If we split before that transistor.
        point--;             //We don't include that transistor at the first file.

    define_new_INs_OUTs(point);
    derive_files(fp, argv[1], point);
    free_pointer();
    fclose(fp);

    return 0;
}

/* ========== FOR MAIN  FUNCTIONALITY ========== */
void initiate_DAG(int x)
{   
    /* At first, let's find every possible split point.
      As condition, we'll find every connection like:
      (S,S,...,D,D,..) --> (G)
    */
    
    if(x>=(numOfMos-2))
        return;

    initiate_DAG(++x);//Recursive call.
    
    //After the end of recursive calls, we implement our logic.
    int gate = DAG[x][1]; //Store gate-node;
    bool isFound = false;
    for(int i=0; i<x; i++)
    {
        isFound = is_node_D_or_S_before(gate, x); //Check if node-gate is also a Drain or Source at a previous node.
        if(isFound)
        {
            DAG[x][4] = 'S';//If node is found, he is candidate for splitting the circuit.
            break;
        }
    }
    return;
}

void find_correct_splitting()
{
    /*
        We check for the specific gate-node if S/D of the same transistor
        participates in a connection as S/D, before or after from the currect 
        point of netlist.
        If that is true, we can't split at this point.
    */
    for(int x=1; x<numOfMos-1; x++)
    {
        if(DAG[x][4] == 'S') //If is marked as splitting point.
        {
            int pin1 = DAG[x][2]; //Source or Drain
            int pin2 = DAG[x][3]; //Source or Drain

            //Check if node exists before.
            bool isBefore1, isBefore2;
            bool isAfter1, isAfter2;

            //We don't take into considaration the nodes for VCC or GND.
            if(pin1 == VCC || pin1 == GND)
            {
                isBefore1 = false;
                isAfter1 = false;
            }
            else
            {
                isBefore1 = is_node_D_or_S_before(pin1, x);
                isAfter1 = is_node_D_or_S_after(pin1, x);
            }

            if (pin2 == VCC || pin2 == GND)
            {
                isBefore2 = false;
                isAfter2 = false;
            }
            else
            {
                isBefore2 = is_node_D_or_S_before(pin2, x);
                isAfter2 = is_node_D_or_S_after(pin2, x);
            }

            if(!isBefore1 && !isBefore2)
                DAG[x][5] = 'B'; //we can split before that transistor.
            else if (!isAfter1 && !isAfter2)
                DAG[x][5] = 'A'; //we can split after that transistor.
            else
                DAG[x][4] = -1; //Discard that splitting one.
        }
    }
}
int find_splitting_point()
{
    int candidate_point = ceil(numOfMos/2)-1;//Pointing to the middle transistor of netlist.   

    //If graph is "split-able" at this point, then return it.
    if(DAG[candidate_point][4] == 'S')
        return candidate_point;

    /* othrewise,
       1. find the distance until a splitting point before
       2. find the distance until a splitting point after
       3. Return the point for which the shortest distance is travelled/covered.
    */

    int x = candidate_point;
    int y = candidate_point;
    
    bool isFoundX = false;
    bool isFoundY = false;

    while(x>0) 
    {
        if(DAG[x][4] == 'S')
        {
            isFoundX = true;
            break;
        }
        x--;
    }
    
    while(y<numOfMos)
    { 
        if(DAG[y][4] == 'S')
        {
            isFoundY = true;
            break; 
        }
        y++;
    }
    
    if(isFoundX && !isFoundY)
        return x;
    
    if(!isFoundX && isFoundY)
        return y;
    
    int dist1 = abs(candidate_point - x);
    int dist2 = abs(candidate_point - y);

    return( (dist1<=dist2) ? x:y );
}

bool is_node_D_or_S_before(int node, int until)
{
    for (int i = 0; i < until; i++)
    {
        //if node is Source or Drain return true.
        if((node==DAG[i][2]) || (node==DAG[i][3]))
            return true;        
    }
    //Otherwise false. 
    return false;
}

bool is_node_D_or_S_after(int node, int from)
{
    for (int i = (from+1); i < numOfMos; i++)
    {
        //if node is Source or Drain return true.
        if((node==DAG[i][2]) || (node==DAG[i][3]))
            return true;        
    }
    //Otherwise false. 
    return false;
}

void define_new_INs_OUTs(int point)
{

    new_IN1 = malloc(sizeof(int) * numOfInputs);
    new_IN2 = malloc(sizeof(int)* numOfInputs);
    new_OUT1 = malloc(sizeof(int)* numOfOutputs);
    new_OUT2 = malloc(sizeof(int) * numOfOutputs);
    in1=0;
    in2=0;
    out1=0;
    out2=0;

    split_INPUT_OUTPUT(point);
    
    /*Because of the  splitting point, we have to define new output for the first file
      and new input the for the second. */    
    bool isFound = false;
    for(int i=point; i>=0; i--)//For splitting point and before.
    {
        for(int j=(point+1); j<numOfMos; j++)//For splitting point and after.
        {
            //Check if a source/drain-node in the first circuit is also a gate-node into second.
            if( (DAG[i][2]==DAG[j][1]) || (DAG[i][3]==DAG[j][1]))
            {
                //if not already exists in new array, the store it.
                if(!is_exist_in(new_OUT1, out1, DAG[j][1]))
                {
                    new_OUT1[out1] = DAG[j][1];
                    out1++;
                    isFound = true;
                }

                if (!is_exist_in(new_IN2, in2, DAG[j][1]))
                {
                    new_IN2[in2] = DAG[j][1];
                    in2++;
                    isFound = true;
                }                
                //isFound = true;
                //break;
            }

            if(isFound)
                break;
        }
    }

}

void split_INPUT_OUTPUT(int point)
{
    //Gate nodes are candidates for inputs and Source or Drain for outputs.
    //Search for Input or Output nodes from original file in whole netlist-DAG.
    //If they are found before splitting point => Store in new_IN1/new_OUT1
    //else if they are foud after => Store in new_IN2/new_OUT2
    for(int i=0; i<numOfMos; i++)//In whole netlist
    {
        int gate = DAG[i][1]; //Store gate-node
        int pin1 = DAG[i][2]; //Store Source/Drain
        int pin2 = DAG[i][3];
        if(is_exist_in(INPUT, numOfInputs, gate))//If Gate-Node is exist in original INPUTS
        {
            //If gate-node is before splitting point and not is exist in new_IN1 array so we wont have duplicates.
            if(i<=point && !is_exist_in(new_IN1, in1, gate))
            {
                new_IN1[in1] = gate; //Store
                in1++; //Increase size of new_IN1
            }//otherwise is after the splitting point.
            else if(i>point && !is_exist_in(new_IN2, in2, gate))
            {
                new_IN2[in2] = gate;
                in2++;
            }            
        }
        //Exact the same logic as above, but for original OUTPUT and Source/Drain pins.
        if(is_exist_in(OUTPUT, numOfOutputs, pin1))
        {
            if (i<=point && !is_exist_in(new_OUT1, out1, pin1))
            {
                new_OUT1[out1] = pin1;
                out1++;
            }
            else if (i>point && !is_exist_in(new_OUT2, out2, pin1))
            {
                new_OUT2[out2] = pin1;
                out2++;
            }
        }
        else if (is_exist_in(OUTPUT, numOfOutputs, pin2))
        {
            if (i<=point && !is_exist_in(new_OUT1, out1, pin2))
            {
                new_OUT1[out1] = pin2;
                out1++;
            }
            else if (i>point && !is_exist_in(new_OUT2, out2, pin2))
            {
                new_OUT2[out2] = pin2;
                out2++;
            }
        }
    }
}


void derive_files(FILE* fp, char *name, int point)
{
    char file1[64], file2[64];

    sprintf(file1, "first_%s", name); // Name new files.
    sprintf(file2, "second_%s", name);

    FILE *fp1 = fopen(file1, "w"); //Create or clear files for appending.
    FILE *fp2 = fopen(file2, "w");
    fseek(fp, 0, SEEK_SET); //seek input's file pointer back to start.

    //## RAILS
    fprintf(fp1, "## RAILS\n");
    fprintf(fp2, "## RAILS\n");
    fprintf(fp1, "VCC %d ; GND %d\n", VCC, GND);
    fprintf(fp2, "VCC %d ; GND %d\n", VCC, GND);
    //## INPUTS
    fprintf(fp1, "## INPUTS\n");
    fprintf(fp2, "## INPUTS\n");
    
    for(int i=0; i<in1; i++)
        fprintf(fp1, "%d ; ", new_IN1[i]);
    
    for(int i=0; i<in2; i++)
        fprintf(fp2, "%d ; ", new_IN2[i]);
    
    fprintf(fp1, "\n");
    fprintf(fp2, "\n");
    //## OUTPUTS
    fprintf(fp1, "## OUTPUTS\n");
    fprintf(fp2, "## OUTPUTS\n");
    
    for(int i=0; i<out1; i++)
        fprintf(fp1, "%d ; ", new_OUT1[i]);
    
    for(int i=0; i<out2; i++)
        fprintf(fp2, "%d ; ", new_OUT2[i]);
    
    fprintf(fp1, "\n");
    fprintf(fp2, "\n");
    //## NETLIST
    fprintf(fp2, "## NETLIST\n");

    char *lineBuffer = NULL;
    size_t len = 0;
    ssize_t read;
    
    bool flag = false;
    int checkpoint = -1;
    while ((read = getline(&lineBuffer, &len, fp)) != -1)
    {
        if(strcmp(lineBuffer, "## TESTBENCH\n") == 0)
            break;

        if(strcmp(lineBuffer, "## NETLIST\n") == 0)
            flag = true;
        
        if(flag)
        { 
            if (checkpoint<=point)
            {
                fprintf(fp1, "%s", lineBuffer);
                checkpoint++;
            }
            else
                fprintf(fp2, "%s", lineBuffer);
        }
    }
    //## TESTBENCH
    fprintf(fp1, "## TESTBENCH\n");
    fprintf(fp2, "## TESTBENCH\n");

    //## TEST_IN
    fprintf(fp1, "## TEST_IN\n");
    fprintf(fp2, "## TEST_IN\n");

    for (int i = 0; i < in1; i++)
        fprintf(fp1, "%d ; ", new_IN1[i]);

    for (int i = 0; i < in2; i++)
        fprintf(fp2, "%d ; ", new_IN2[i]);

    fprintf(fp1, "\n");
    fprintf(fp2, "\n");
    //## TEST_OUT
    fprintf(fp1, "## TEST_OUT\n");
    fprintf(fp2, "## TEST_OUT\n");

    for (int i = 0; i < out1; i++)
        fprintf(fp1, "%d ; ", new_OUT1[i]);

    for (int i = 0; i < out2; i++)
        fprintf(fp2, "%d ; ", new_OUT2[i]);

    fprintf(fp1, "\n");
    fprintf(fp2, "\n");

    //Let's create every Vector :).
    int num_of_vec_1 = pow(2, in1);
    int num_of_vec_2 = pow(2, in2);
    int length_vec_1 = in1;
    int length_vec_2 = in2;

    create_every_vector(fp1, num_of_vec_1, length_vec_1);
    create_every_vector(fp2, num_of_vec_2, length_vec_2);

    //## END
    fprintf(fp1, "## END_TEST\n");
    fprintf(fp2, "## END_TEST\n");
    fprintf(fp1, "## END_SIMULATION\n");
    fprintf(fp2, "## END_SIMULATION\n");

    fclose(fp1);
    fclose(fp2);
}

void create_every_vector(FILE *fp, int quantity, int length)
{
    int *binary = (int *)malloc(sizeof(int) * length);

    for (int i = 0; i < quantity; i++)
    {
        
        fprintf(fp, "## TEST_VECTORS\n");
        int dec_num = i;
        int bin_index = length - 1;
        for (int j = 0; j < length; j++)
        {
            binary[bin_index] = dec_num % 2;
            bin_index--;
            dec_num = dec_num / 2;
        }

        for (int j = 0; j < length; j++)
            fprintf(fp, "%d ; ", binary[j]);
        
        fprintf(fp, "\n## SIMULATE\n");
    }

    free(binary);
}
/* ========== UTILITY FUNCTIONS ========== */

bool is_exist_in(int *Array, int size, int value)
{
    for(int i=0; i<size; i++)
    {
        if(Array[i] == value)
            return true;
    }

    return false;
}
void store_circuit_infos(FILE *fp)
{
    char *lineBuffer = NULL;
    size_t len = 0;
    ssize_t read;

    bool rails = false;
    bool inputs = false;
    bool outputs = false;
    bool netlist = false;

    char *token;
    char delims[] = "',', ';', ' '";

    while ((read = getline(&lineBuffer, &len, fp)) != -1)
    {
        if (strcmp(lineBuffer, "## TESTBENCH\n") == 0)
            break;

        lineBuffer[strlen(lineBuffer) - 1] = '\0'; //Strings end with char \0. Overwriting char '\n'.

        if (rails)
        {
            store_RAILS(lineBuffer);
            rails = false;
        }

        if (inputs)
        {
            numOfInputs = call_strtok(lineBuffer, &INPUT);
            inputs = false;
        }

        if (outputs)
        {
            numOfOutputs = call_strtok(lineBuffer, &OUTPUT);
            outputs = false;
        }

        if (netlist)
        {
            token = strtok(lineBuffer, delims);
            token = strtok(NULL, delims);

            if (numOfMos == 0)
            {
                DAG = (int **)malloc(sizeof(int));
                DAG[numOfMos] = (int *)malloc(sizeof(int) * 6);
            }
            else
            {
                DAG = (int **)realloc(DAG, sizeof(int) * 6 * (numOfMos + 1));
                DAG[numOfMos] = (int *)malloc(sizeof(int) * 6);
            }

            if (strcmp(token, "PMOS") == 0)
                DAG[numOfMos][0] = 'P';
            else if (strcmp(token, "NMOS") == 0)
                DAG[numOfMos][0] = 'N';
            else
            {
                printf("Error in section ## NETLIST.\n");
                printf("Transistor U%d: Wrong keyword for {PMOS/NMOS}.\n", numOfMos + 1);
                exit(1);
            }

            for (int i = 1; i <= 3; i++)
            {
                token = strtok(NULL, delims);
                if (token == NULL)
                {
                    printf("Error in section ## NETLIST.\n");
                    printf("Transistor U%d: GATE or SOURCE or DRAIN is missing.\n", numOfMos + 1);
                    exit(1);
                }
                DAG[numOfMos][i] = atoi(token);
            }
            DAG[numOfMos][4] = -1;
            DAG[numOfMos][5] = -1;
            numOfMos++;
        }

        token = strtok(lineBuffer, delims);
        if (strcmp(token, "##") == 0)
        {
            token = strtok(NULL, delims);
            if (strcmp(token, "RAILS") == 0)
                rails = true;

            if (strcmp(token, "INPUTS") == 0)
                inputs = true;

            if (strcmp(token, "OUTPUTS") == 0)
                outputs = true;

            if (strcmp(token, "NETLIST") == 0)
                netlist = true;
        }
    }
}

void store_RAILS(char *buff)
{
    char *token1, *token2, *token3, *token4;
    const char delims[] = "';', ',', ' '";

    token1 = strtok(buff, delims);    
    token2 = strtok(NULL, delims);  
    if(token2 == NULL)
        return;  
    token3 = strtok(NULL, delims);
    token4 = strtok(NULL, delims);

    if (strlen(token1) > 3 || strlen(token3) > 3)
    {
        printf("ERROR: File's format: Section \"## RAILS\".\n");
        printf("Possible mistakes: \n");
        printf("1. No delimeter between {VCC/GND} and {Node Number}.\n");
        printf("2. Wrong keyword for {VCC/GND}\n");
        exit(1);
    }

    int node1 = atoi(token2);
    int node2 = atoi(token4);
    
    if (strcmp(token1, "VCC") == 0)
    {
        VCC = node1;
        GND = node2;
    }
    else if (strcmp(token1, "GND") == 0)
    {
        VCC = node2;
        GND = node1;
    }
    else
    {
        printf("ERROR: File's format: Section \"## RAILS\".\n");
        printf("Wrong keyword for {VCC/GND}\n");
        exit(1);
    }
    
}

int call_strtok(char *string, int **array)
{
    char *token;
    int count = 0;
    int buff[0xff];
    char delims[] = "',', ';', ' '";
    
    token = strtok(string, delims);
    if (token == NULL)
    {
        printf("Circuit's input or output doesn't exist.:(\n");
        exit(1);
    }

    buff[count] = atoi(token);
    count++;
    while ((token = strtok(NULL, delims)) != NULL)
    {
        buff[count] = atoi(token);
        count++;
    }

    *array = (int *)malloc(sizeof(int) * count);

    for (int i = 0; i < count; i++)
        (*array)[i] = buff[i];

    free(buff);

    return count;
}

void checkInput(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Incorrect input. Please try again :-).\n");
        usage();
        exit(1);
    }
    else
    {
        //Check if file exists.
        if (access(argv[1], F_OK) != 0)
        {
            printf("File doesn't exist. Unlucky :-(.\n");
            usage();
            exit(1);
        }
        printf("Splitting File \"%s\"... \n", argv[1]);
    }
}

void usage()
{
    printf("- Only one file is accepted and has to be in the same directory. Otherwise enter full path.\n");
    printf("- Via terminal type: ./File.exe File_Path_OR_Name.\n");
    printf("\tWhere File.exe is the executable and File_Path_OR_Name is the file's name or path you wish.\n");
    printf("- If you are using an IDE Platform (ex. CodeBlocks), you have to set the arguments from Preferences before you hit Run Button.\n");
}

void free_pointer()
{
    //Free Pointers.
    if (INPUT != NULL)
        free(INPUT);

    if (OUTPUT != NULL)
        free(OUTPUT);

    if(new_IN1 != NULL)
        free(new_IN1);

    if(new_IN2 != NULL)
        free(new_IN2);

    if (new_OUT1 != NULL)
        free(new_OUT1);

    if (new_OUT2 != NULL)
        free(new_OUT2);

    for (int i = 0; i < numOfMos; i++)
    {
        if (DAG[i] != NULL)
            free(DAG[i]);
    }
    if (DAG != NULL)
        free(DAG);

}

void my_test()
{
    /* printf("VCC: %d\n", VCC);
    printf("GND: %d\n", GND);

    for(int i=0; i<numOfInputs; i++)
        printf("INPUT[%d] = %d\n", i, INPUT[i]);

    for (int i = 0; i < numOfOutputs; i++)
        printf("OUTPUT[%d] = %d\n", i, OUTPUT[i]);*/


    printf("IN1 = ");
    for(int i=0; i<in1; i++)
        printf("%d ", new_IN1[i]);
    printf("\n");

    printf("IN2 = ");
    for (int i = 0; i < in2; i++)
        printf("%d ", new_IN2[i]);
    printf("\n");

    printf("OUT1 = ");
    for (int i = 0; i < out1; i++)
        printf("%d ", new_OUT1[i]);
    printf("\n");

    printf("OUT2 = ");
    for (int i = 0; i < out2; i++)
        printf("%d ", new_OUT2[i]);
    printf("\n");

     for(int i=0; i<numOfMos; i++)
    {
        printf("U%d ", i+1);
        if(DAG[i][0] == 'P')
            printf("P ");
        else
            printf("N ");

        printf("%d %d %d ", DAG[i][1], DAG[i][2], DAG[i][3]);

        if (DAG[i][4] == 'S')
            printf("S ");
        else
            printf("-1 ");

        if (DAG[i][5] == 'B')
            printf("B");
        else if (DAG[i][5] == 'A')
            printf("A");
        else
            printf("-1");
        

        printf("\n");
    } 

    getchar();
}