/*
    Author: Mylonakis Emmanouil
    Registration Number: 2015030079
    Date: 26/05/2021
    Operating System: Windows 10.

    Description: Exercise 6. Formal Verification Methods and Model Checking.
                             We check if 2 circuits are logical equivelant or not.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

/* ========== FOR MAIN FUNCTIONALITY ========== */
void basic_checks(); // Basic and important checks in order for the algorithm to works properly.
int final_check(); //Check every path if is isomorphic.
void print_problem(); //Just for the report.
void update_equal_nodes(int index0, int index1); // Find equal nodes to print them in report.
void find_all_paths(int layer); //Finds every single path in the DAG.
int find_starting_points(int layer, int *S); // Finds every strating point in array DAG.
void DFS(int layer, int index, int *path, int path_index); // Depth-First-Search algorithm.
void store_existed_path_in_list(int *path, int layer);    //Stores the path was found by DFS in the list.
int find_all_neighbors(int layer, int node, int *array);   // Finds every neighbor for a specific node.
bool is_this_destination_node(int index, int layer); // Check if we reach our destination/output node.
void output_report(bool isEqual, char txt[]); // Derives output file.

/* ========== UTILITY FUNCTIONS ========== */
void checkInput(int argc, char *argv[]);     // Checks User's Input Correctness.
void open_files(int argc, char *argv[]);     // Open Input files for reading.
void usage();                                //Prints a help message for user.
void my_test();
void store_RAILS(char *buff);                     //Update infos from section ## RAILS.
void store_circuit_infos(FILE *fp, int index);    //Stores from file every info until the section ## TESTBENCH.
void store_DAG(char *buff, int ***DAG);           //Store netlist from file.
int call_strtok(char *line, int **array);         // Calls repeatedly strtok() function. Allocate array's size properly and returns the number of rows.
void free_pointer(); //free memory before program terminate.

int **INPUT = NULL;                   // Section ## INPUTS, store input nodes dynamically.
int **OUTPUT = NULL;                  // Section ## OUTPUTS, store output nodes dynamically.
int ***DAG = NULL;                    // Section ## NETLIST, store infos for every gate dynamically.
/*
    DAG is 3D array. Imagine 2 layers of 2D array.
    index=0 is the first layer for the original file.
    index=1 is the second layer for the file under examination.
    For Each layer the rows represent:
    1st: Number X of Gate "GX".
    2st: The kind of Gate (NOR, NAND, NOT, etc).
    3st: 1st input.
    4st: 2nd input (if exists, in case of NOT set with -1)
    5st: The output.
*/

int numOfInputs[2], numOfOutputs[2];  // Number of Inputs and Outputs our circuit has.
int numOfGates[2] = {0, 0};          // Number of Gates in our netlist.
int list_of_paths[2][1024][1024];   // Layer 0 for the original file, layer 1 for the file under examination
int num_of_paths[2] = {0, 0};      //Number of paths in the abose list.
char str_path[2][1024][1024];      // String encode of the paths
int equal_nodes[2][1024];          //Equivalent nodes.
int num_equal_nodes = 0;           //Number of equivalent nodes.

int main(int argc, char *argv[])
{
    checkInput(argc, argv); //Check input's correctness.
    //Since the user's input is correct, we initiate some memory.
    //Infos for the original file and the file under examination.
    INPUT = (int **)malloc(sizeof(int)*2); //Two 1-dimensional arrays for inputs
    OUTPUT = (int **)malloc(sizeof(int)*2); //Two 1-dimensional arrays for outputs.
    DAG = (int ***)malloc(sizeof(int) * 2); //Two 2-dimensional arrays for DAG.
    
    open_files(argc, argv);
    basic_checks(); 
    find_all_paths(0);
    find_all_paths(1);

    int count_corrected = final_check();
    if (count_corrected == num_of_paths[0] && num_of_paths[0] == num_of_paths[1])
        output_report(true, NULL);
    else
        print_problem();

    //my_test();
    free_pointer();

    return 0;
}

/* ========== FOR MAIN FUNCTIONALITY ========== */
void find_all_paths(int layer)
{
    int path[1024];
    int S[100];
    
    // Find every starting point.
    int count = find_starting_points(layer, S);

    // For every starting point call DFS algorithm.
    for(int i=0; i<count; i++)
        DFS(layer, S[i], path, 0);
}

void DFS(int layer, int index, int *path, int p_index)
{
    if(is_this_destination_node(index, layer)) // If we reach destination.
    {
        path[p_index] = index; //Push last node.
        path[p_index+1] = -1; //Store -1 as terminating value
        store_existed_path_in_list(path, layer); //Add this path in the list.
        return;
    }
   
    path[p_index] = index; //Push current node in the path.

    int NB[10];                                                          // For neighbors
    int neighbors = find_all_neighbors(layer, DAG[layer][index][4], NB); // For current node find every neighbor.

    for(int i=0; i<neighbors; i++)
    {
        p_index++; // Increase index for path array.
        DFS(layer, NB[i], path, p_index); // Call recursion.
        path[p_index] = -1; // Pop last node.
        p_index--; //Decrease index for path array.
    }

    return;
}

void store_existed_path_in_list(int *path, int layer)
{
    //For indexing the nodes.
    int i;
    int arleady_counted = num_of_paths[layer];
    //For converting path to string.
    char gate;
    char temp[2];
    int node;

    // Set first character with the terminal value of string in order to strncat works properly.
    str_path[layer][arleady_counted][0] = '\0';

    //For every node in current path.
    for(i=0; path[i] != -1; i++)
    {
        //Store node in list of paths.
        list_of_paths[layer][arleady_counted][i] = path[i];

        node = path[i]; // Node value.
        gate = (char) DAG[layer][node][1]; //Index with node value to retrieve the color of node.
        temp[0] = gate; //Color of node.
        temp[1] = '\0'; // Terminal value of string.
        // Temp string concatenated with str_path.
        strncat(str_path[layer][arleady_counted], temp, strlen(temp));    
    }

    //Terminating value for the path
    list_of_paths[layer][arleady_counted][i] = -1;//path[i]; // Stores -1.
    num_of_paths[layer]++;
}

bool is_this_destination_node(int index, int layer)
{
    //Check if node is output of the circuit.
    for(int i=0; i<numOfOutputs[layer]; i++)
    {
        if(DAG[layer][index][4] == OUTPUT[layer][i])
            return true;
    }

    return false;
}

int find_all_neighbors(int layer, int node, int *NB)
{
    // Find every adjacent node.
    // The value of variable node is an output of a gate.
    int count = 0;
    for(int i=0; i<numOfGates[layer]; i++)
    {
        //Check if output of the current gate is also an input in other gate.
        if(DAG[layer][i][2] == node || DAG[layer][i][3] == node)
        {
            NB[count] = i;
            count++;
        }
    }

    return count;
}

int find_starting_points(int layer, int *S)
{
    int count = 0;
    //Find every starting point in netlist-DAG.
    for(int i=0; i<numOfGates[layer]; i++)
    {
        //Loop through the inputs of circuit.
        for(int j=0; j<numOfInputs[layer]; j++)
        {
            //If the first input of the gate is also input of the circuit.
            if(DAG[layer][i][2] == INPUT[layer][j])
            {
                S[count] = i;
                count++;
            }

            //If the second input of the gate is also an input of the circuit, but has to have different value of the other input.
            if(DAG[layer][i][3] == INPUT[layer][j] && DAG[layer][i][3] != DAG[layer][i][2])
            {
                S[count] = i;
                count++;
            }
        }
    }

    return count;
}

int final_check()
{
    int count=0;

    //For every path in golden standard.
    for(int i=0; i<num_of_paths[0]; i++)
    {
        //For every path in the file under examination.
        for(int j=0; j<num_of_paths[1]; j++)
        {            
            if(str_path[1][j][0] == 'C') // If already counted, continue to the next loop.
                continue;
            // If encoded string paths are equal.
            if(strcmp(str_path[0][i], str_path[1][j]) == 0)
            {
                str_path[1][j][0] = 'C'; // We found an equal path.
                str_path[0][i][0] = 'C';
                if(num_equal_nodes < numOfGates[0])
                    update_equal_nodes(i, j);
                
                count++; // Count +1.
                break; // Break from this loop to check the next one.
            }
        }
    }

    return count;
}

void print_problem()
{
    char text[100];
    char temp[100];

    sprintf(text, "ORIGINAL FILE: Number of paths: %d\n", num_of_paths[0]);
    sprintf(temp, "FILE UNDER EXAMINATION: Number of paths: %d\n", num_of_paths[1]);
    strncat(text, temp, strlen(temp));    

    output_report(false, text);
    exit(1);
}


void update_equal_nodes(int index0, int index1)
{
    int count=0;
    int node0, node1;
    //index0 and index1 are indexing in equivalent paths so.
    for(int i=0; list_of_paths[0][index0][i]!=-1; i++)
    {
        count=0;
        node0 = list_of_paths[0][index0][i]; //Node for golden standard.
        node1 = list_of_paths[1][index1][i]; //Node for the file under examination.

        //Check in node already exist in order to update distinctively.
        while(count<num_equal_nodes)
        {
            if(equal_nodes[0][count] != node0)
                count++;
            else
                break;            
        }
        //If node doesn't exist the store it.
        if(count == num_equal_nodes)
        {
            equal_nodes[0][count] = node0;
            equal_nodes[1][count] = node1;
            num_equal_nodes++;
        }
    }
}

void basic_checks()
{
    char txt[1024];
    char txt_temp[1024];

    sprintf(txt, "Circuits are not equivalent.\n");
    // Check number of gates for each graph.
    if (numOfGates[1] < numOfGates[0])
    {
        sprintf(txt_temp, "PROBLEM: Less number of Gates than original file.\n");
        strncat(txt, txt_temp, strlen(txt_temp));
        output_report(false, txt);
        exit(1);
    }
    else if(numOfGates[1] > numOfGates[0])
    {
        sprintf(txt_temp, "PROBLEM: More number of Gates than original file.\n");
        strncat(txt, txt_temp, strlen(txt_temp));
        output_report(false, txt);
        exit(1);
    }

    if(numOfInputs[0] != numOfInputs[1])
    {
        sprintf(txt_temp, "Number of inputs between circuits are not the same.\n");
        strncat(txt, txt_temp, strlen(txt_temp));
        output_report(false, txt);
        exit(1);
    }

    if(numOfOutputs[0] != numOfOutputs[1])
    {
        sprintf(txt_temp, "Number of outputs between circuits are not the same.\n");
        strncat(txt, txt_temp, strlen(txt_temp));
        output_report(false, txt);
        exit(1);
    }

    // Count number of gates according to their type for each graph.    
    int C0[3] = {0,0,0}; // For the original file.
    int C1[3] = {0,0,0}; // For the file under examination.
    for(int i=0; i<numOfGates[0]; i++)
    {
        if(DAG[0][i][1] == 'O')
            C0[0]++;
        else if(DAG[0][i][1] == 'A')
            C0[1]++;
        else if(DAG[0][i][1] == 'N')
            C0[2]++;
        
        if(DAG[1][i][1] == 'O')
            C1[0]++;
        else if(DAG[1][i][1] == 'A')
            C1[1]++;
        else if(DAG[1][i][1] == 'N')
            C1[2]++;
    }

    // Check what we have counted.
    //For Gate NOR.
    sprintf(txt_temp, "Generally, number of Gates-Nodes are equal, but:\n");
    strncat(txt, txt_temp, strlen(txt_temp));
    int count_problems = 0;
    if(C1[0] < C0[0])
    {
        count_problems++;
        sprintf(txt_temp, "PROBLEM %d: %d Less NOR_2-Gates than original file.\n", count_problems, C0[0]-C1[0]);
        strncat(txt, txt_temp, strlen(txt_temp));
    }
    else if(C1[0] > C0[0])
    {
        count_problems++;
        sprintf(txt_temp, "PROBLEM %d: %d More NOR_2-Gates than original file.\n", count_problems, C1[0]-C0[0]);
        strncat(txt, txt_temp, strlen(txt_temp));
    }

    //For Gate NAND.
    if(C1[1] < C0[1])
    {
        count_problems++;
        sprintf(txt_temp, "PROBLEM %d: %d Less NAND_2-Gates than original file.\n", count_problems, C0[1]-C1[1]);
        strncat(txt, txt_temp, strlen(txt_temp));
    }
    else if(C1[1] > C0[1])
    {
        count_problems++;
        sprintf(txt_temp, "PROBLEM %d: %d Μore NAND_2-Gates than original file.\n", count_problems, C1[1]-C0[1]);
        strncat(txt, txt_temp, strlen(txt_temp));
    }

    //For Gate NOT.
    if(C1[2] < C0[2])
    {
        count_problems++;
        sprintf(txt_temp, "PROBLEM %d: %d Less NOT-Gates than original file.\n", count_problems, C0[2]-C1[2]);
        strncat(txt, txt_temp, strlen(txt_temp));
    }
    else if(C1[2] > C0[2])
    {
        count_problems++;
        sprintf(txt_temp, "PROBLEM %d: %d More NOT-Gates than original file.\n", count_problems, C1[2]-C0[2]);
        strncat(txt, txt_temp, strlen(txt_temp));
    }

    if(count_problems>0)
    {
        output_report(false, txt);
        exit(1);
    }
}

void output_report(bool isEqual, char *txt)
{
    FILE *fp_out = fopen("REPORT.txt", "w");

    if(isEqual)
    {
        fprintf(fp_out, "====================== Circuits are equivalent !!! ======================\n\n");
        int node0, node1;

        fprintf(fp_out, "|-----------------------------------------------------------------------|\n");
        fprintf(fp_out, "|            Original               |            Εxamined               |\n");
        fprintf(fp_out, "|        [Node-Gate]:[Edges]        |        [Node-Gate]:[Edges]        |\n");
        fprintf(fp_out, "|-----------------------------------------------------------------------|\n");

        int len_1 = 35;
        char temp[200];
        for (int i = 0; i < num_equal_nodes; i++)
        {
            node0 = equal_nodes[0][i];
            node1 = equal_nodes[1][i];

            if(DAG[0][node0][1] == 'N')
            {
                fprintf(fp_out, "| G%d NOT:%d->%d", DAG[0][node0][0], DAG[0][node0][2], DAG[0][node0][4]);
                sprintf(temp, "| G%d NOT:%d->%d", DAG[0][node0][0], DAG[0][node0][2], DAG[0][node0][4]);
            }
            else if(DAG[0][node0][1] == 'O')
            {
                fprintf(fp_out, "| G%d NOR_2:(%d,%d)->%d", DAG[0][node0][0], DAG[0][node0][2], DAG[0][node0][3], DAG[0][node0][4]);
                sprintf(temp, "| G%d NOR_2:(%d,%d)->%d", DAG[0][node0][0], DAG[0][node0][2], DAG[0][node0][3], DAG[0][node0][4]);
            }
            else if(DAG[0][node0][1] == 'A')
            {
                fprintf(fp_out, "| G%d NAND_2:(%d,%d)->%d", DAG[0][node0][0], DAG[0][node0][2], DAG[0][node0][3], DAG[0][node0][4]);
                sprintf(temp, "| G%d NAND_2:(%d,%d)->%d", DAG[0][node0][0], DAG[0][node0][2], DAG[0][node0][3], DAG[0][node0][4]);
            }

            for (int j = 0; j < (len_1 - (strlen(temp) - 1)); j++)
                fprintf(fp_out, " ");

            if (DAG[1][node1][1] == 'N')
            {
                fprintf(fp_out, "| G%d:%d->%d", DAG[1][node1][0], DAG[1][node1][2], DAG[1][node1][4]);
                sprintf(temp, "| G%d:%d->%d", DAG[1][node1][0], DAG[1][node1][2], DAG[1][node1][4]);
            }
            else if(DAG[1][node1][1] == 'O')
            {
                fprintf(fp_out, "| G%d NOR_2:(%d,%d)->%d", DAG[1][node1][0], DAG[1][node1][2], DAG[1][node1][3], DAG[1][node1][4]);
                sprintf(temp, "| G%d NOR_2:(%d,%d)->%d", DAG[1][node1][0], DAG[1][node1][2], DAG[1][node1][3], DAG[1][node1][4]);
            }
            else if(DAG[1][node1][1] == 'A')
            {
                fprintf(fp_out, "| G%d NAND_2:(%d,%d)->%d", DAG[1][node1][0], DAG[1][node1][2], DAG[1][node1][3], DAG[1][node1][4]);
                sprintf(temp, "| G%d NAND_2:(%d,%d)->%d", DAG[1][node1][0], DAG[1][node1][2], DAG[1][node1][3], DAG[1][node1][4]);
            }

            for (int j = 0; j < (len_1 - (strlen(temp) - 1)); j++)
                fprintf(fp_out, " ");

            fprintf(fp_out, "|\n");
        }
        fprintf(fp_out, "|-----------------------------------------------------------------------|\n");
        fclose(fp_out);
        return;
    }

    fprintf(fp_out, "======== Circuits are not equivalent !!! ========\n\n");
    fprintf(fp_out, "%s\n", txt);
    fclose(fp_out);
}

/* ========== UTILITY FUNCTIONS ========== */
void open_files(int argc, char *argv[])
{
    for(int i=1; i<argc; i++)
    {
        FILE *fp = fopen(argv[i], "r");
        if(fp == NULL)
        {
            printf("Error: Open File.\n");
            exit(1);
        }

        store_circuit_infos(fp, i-1);
        fclose(fp);
    }
}

void store_circuit_infos(FILE *fp, int index)
{
    //For reading the file.
    char *lineBuffer = NULL;
    char tmp_line[64];
    size_t len = 0;
    ssize_t read;

    //For splitting the file.
    char *token;
    char delims[] = "',', ';', ' '";
    bool isNetlist = false;
    // For storing data/allocate memory.
    int dozens_of_space = 2; 
    
    //For the report.
    char msg[100];

    while ((read = getline(&lineBuffer, &len, fp)) != -1)
    {
        lineBuffer[strlen(lineBuffer) - 1] = '\0'; //Strings end with char \0. Overwriting char '\n'.
        sprintf(tmp_line, "%s", lineBuffer);

        token = strtok(lineBuffer, delims);
        if(strcmp(token, "##") == 0)
            token = strtok(NULL, delims);
        else if(!isNetlist)
            continue;
        
        if(strcmp(token, "INPUTS") == 0) // Store Inputs.
        {
            getline(&lineBuffer, &len, fp);
            numOfInputs[index] = call_strtok(lineBuffer, &(INPUT[index]));
        }

        if(strcmp(token, "OUTPUTS") == 0) // Store Outputs.
        {
            getline(&lineBuffer, &len, fp);
            numOfOutputs[index] = call_strtok(lineBuffer, &(OUTPUT[index]));
        }

        if(strcmp(token, "NETLIST") == 0)//We reach to netlist section.
        {
            isNetlist = true;
            continue;
        }

        if(isNetlist) //While we are in the netlist section store infos like.
        {            
            int G_token; // For Gate's number.

            //Allocate memory properly.
            if (numOfGates[index] == 0)
            {
                DAG[index] = (int **)malloc(sizeof(int ) * 10);//10 rows
                for(int i=0; i<10; i++)
                    DAG[index][i] = (int *)malloc(sizeof(int) * 5);//Five columns
            }
            else if ((numOfGates[index]%10) == 0)
            {
                //We found 10 more Gate. Realloc memory for rows.
                DAG[index] = (int **)realloc(DAG[index], sizeof(int) * 10 * dozens_of_space);
                for(int i=numOfGates[index]; i<(numOfGates[index]+10); i++)
                    DAG[index][i] = (int *)malloc(sizeof(int) * 5);// For the new row, allocate five new columns.
                
                dozens_of_space++;
            }

            //Store Number of Gate.
            G_token = atoi(strtok(token, "G"));

            DAG[index][numOfGates[index]][0] = G_token;

            //What kind of gate we have ?
            token = strtok(tmp_line, delims); // Token = "GX"
            token = strtok(NULL, delims);       // Token = NOR_2 or NAND_2,...
            
            if(strcmp(token, "NOR_2") == 0)
                DAG[index][numOfGates[index]][1] = 'O'; // 'O' for NOR_2
            else if(strcmp(token, "NAND_2") == 0)
                DAG[index][numOfGates[index]][1] = 'A'; // 'A' for NAND_2
            else if(strcmp(token, "NOT") == 0)
                DAG[index][numOfGates[index]][1] = 'N'; // 'N' for NOT
            else
            {
                sprintf(msg, "ERROR: Unknown gate G%d\n", G_token);
                output_report(false, msg);
                exit(1);
            }
            
            //1st Input of Gate
            token = strtok(NULL, delims); // Token = "IN".
            if(strcmp(token, "IN") != 0)
            {
                sprintf(msg, "ERROR: Wrong Format. Section Netlist G%d\nCheck Keyword IN.\n", G_token);
                output_report(false, msg);
                exit(1);
            }

            token = strtok(NULL, delims); // Token = 1st input.
            DAG[index][numOfGates[index]][2] = atoi(token);

            token = strtok(NULL, delims); // Token = 2st input ?
            if(DAG[index][numOfGates[index]][1] != 'N') //This isn't a "NOT" gate. 2nd input exists.
            {
                DAG[index][numOfGates[index]][3] = atoi(token);
                token = strtok(NULL, delims); // Token = "OUT".
            }
            else //Otherwise, 2nd input doesn't exist.
            {
                DAG[index][numOfGates[index]][3] = -1;
            }

            if(strcmp(token, "OUT") != 0)
            {
                sprintf(msg, "ERROR: Wrong Format. Section Netlist G%d\n", G_token);
                output_report(false, msg);
                exit(1);
            }

            token = strtok(NULL, delims); // Token = output.
            DAG[index][numOfGates[index]][4] = atoi(token);
            numOfGates[index]++;
        }
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
    if (argc != 3)
    {
        printf("Incorrect input. Please try again :-).\n");
        usage();
        exit(1);
    }
    else
    {
        //Check if file exists.
        if ((access(argv[1], F_OK) != 0) && (access(argv[1], F_OK) != 0))
        {
            printf("File doesn't exist. Unlucky :-(.\n");
            usage();
            exit(1);
        }
        printf("Simulating Files: \"%s\", \"%s\"... \n", argv[1], argv[1]);
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
    for(int i=0; i<2; i++)
    {
        if (INPUT[i] != NULL)
            free(INPUT[i]);
    }
    if (INPUT != NULL)
        free(INPUT);

    for(int i=0; i<2; i++)
    {
        if (OUTPUT[i] != NULL)
            free(OUTPUT[i]);
    }
    if (OUTPUT != NULL)
        free(OUTPUT);

    for(int j=0; j<2; j++)
    {
        for(int i=0; i<numOfGates[j]; i++)
        {        
            if(DAG[j][i] != NULL)
                free(DAG[j][i]);
        }

        if(DAG[j] != NULL)
            free(DAG[j]);
    }

    if(DAG != NULL)
        free(DAG);
}

void my_test()
{

    printf("Original File.\n");

    printf("INPUTS: ");
    for(int i=0; i<numOfInputs[0]; i++)
        printf("%d ", INPUT[0][i]);
    printf("\n");

    printf("OUTPUTS: ");
    for(int i=0; i<numOfOutputs[0]; i++)
        printf("%d ", OUTPUT[0][i]);
    printf("\n");

    printf("DAG\n");
    for(int i=0; i<numOfGates[0]; i++)
    {
        printf("G%d ", DAG[0][i][0]);
        
        if(DAG[0][i][1] == 'O')
            printf("NOR_2 ");
        else if(DAG[0][i][1] == 'A')
            printf("NAND_2 ");
        else
            printf("NOT ");

        printf("%d ", DAG[0][i][2]);
        printf("%d ", DAG[0][i][3]);
        printf("%d ", DAG[0][i][4]);

        printf("\n");
    }

    printf("\nChecking File.\n");

    printf("INPUTS: ");
    for (int i = 0; i < numOfInputs[1]; i++)
        printf("%d ", INPUT[1][i]);
    printf("\n");

    printf("OUTPUTS: ");
    for(int i=0; i<numOfOutputs[1]; i++)
        printf("%d ", OUTPUT[1][i]);
    printf("\n");

    printf("DAG\n");
    for(int i=0; i<numOfGates[1]; i++)
    {
        printf("G%d ", DAG[1][i][0]);

        if(DAG[1][i][1] == 'O')
            printf("NOR_2 ");
        else if(DAG[1][i][1] == 'A')
            printf("NAND_2 ");
        else
            printf("NOT ");

        printf("%d ", DAG[1][i][2]);
        printf("%d ", DAG[1][i][3]);
        printf("%d ", DAG[1][i][4]);

        printf("\n");
    }

    printf("ORIGINAL\n");
    for (int i = 0; i < num_of_paths[0]; i++)
    {
        for (int j = 0; list_of_paths[0][i][j] != -1; j++)
        {
            int node = list_of_paths[0][i][j];
            printf("G%d ", DAG[0][node][0]);
        }
        printf("\n");
        printf("%s\n", str_path[0][i]);
    }

    printf("\nCheck\n");
    for (int i = 0; i < num_of_paths[1]; i++)
    {
        for (int j = 0; list_of_paths[1][i][j] != -1; j++)
        {
            int node = list_of_paths[1][i][j];
            printf("G%d ", DAG[1][node][0]);
        }
        printf("\n");
        printf("%s\n", str_path[1][i]);
    }
    getchar();
}