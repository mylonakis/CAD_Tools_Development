/*
    Author: Mylonakis Emmanouil
    Registration Number: 2015030079
    Date: 10/05/2021
    Operating System: Windows 10.

    Description: Exercise 5. Basic principles of Formal Verification Method.
                 Derive a netlist of gate from a flat netlist with transistor.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

/* ========== FOR MAIN  FUNCTIONALITY ========== */
void split_graph_into_gates();
bool is_in_gate_NOT(int mos, int offset, FILE *fp);
bool is_in_gate_NAND(int mos, int offset, FILE *fp);
bool is_in_gate_NOR(int mos, int offset, FILE *fp);
void derive_subgraphs_file(char *Gate, int offset);

/* ========== UTILITY FUNCTIONS ========== */
void checkInput(int argc, char *argv[]);      // Checks User's Input Correcteness.
void usage();                                //Prints a help message for user.
void my_test();                                //Prints a help message for user.
void store_RAILS(char *buff);                //Update infos from section ## RAILS.
void store_circuit_infos(FILE *fp);          //Stores from file every info until the section ## TESTBENCH.
int call_strtok(char *line, int **array);    // Calls repeattadly strtok() function. Allocate array's size properly and returns the number of rows.
void free_pointer(); //free memory before program terminate.

int VCC, GND;                  // Section ## RAILS, store nodes for VCC and GNB.
int *INPUT = NULL;             // Section ## INPUTS, store input nodes dynamically.
int *OUTPUT = NULL;            // Section ## OUTPUTS, store output nodes dynamically.
int **DAG = NULL;              // Section ## NETLIST, store infos for every transistor dynamically.
int numOfInputs, numOfOutputs; //Number of Inputs and Outputs our circuit has.
int numOfMos = 0;              //Number of Transistor in our netlist.

int Gate_input[2];
int Gate_output;

FILE *fp_out = NULL;

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

    
    fp_out = fopen("Subgraphs.txt", "w");
    
    split_graph_into_gates();
    
    fclose(fp_out);
    
    //Lets find which transistors dont participate in gate connection
    for(int i=0; i<numOfMos; i++)
    {
        if(DAG[i][4] == -1)
        {
            printf("U%d doesn't belong in a gate.", DAG[i][5] );
        }
    }
    //my_test();
    free_pointer();
    fclose(fp);

    return 0;
}

/* ========== FOR MAIN  FUNCTIONALITY ========== */
void split_graph_into_gates()
{
    int offset = 1;

    FILE *fp = NULL;
    fp = fopen("Gates_Netlist.txt", "w");
    fprintf(fp, "## LIBRARY\n");
    fprintf(fp, "GATES.LIB\n");
    fprintf(fp, "## RAILS\n");
    fprintf(fp, "## INPUTS\n");
    for(int i=0; i<numOfInputs; i++)
    {
        if(i == (numOfInputs-1))
            fprintf(fp, "%d\n", INPUT[i]);
        else
            fprintf(fp, "%d, ", INPUT[i]);
    }

    fprintf(fp, "## OUTPUTS\n");
    for (int i = 0; i < numOfOutputs; i++)
    {
        if (i == (numOfOutputs-1))
            fprintf(fp, "%d\n", OUTPUT[i]);
        else
           fprintf(fp, "%d, ", OUTPUT[i]);
    }
    fprintf(fp, "## NETLIST\n");

    for(int i=0; i<numOfMos; i++)
    {
        if(DAG[i][4] != -1)
            continue;
        
        if(is_in_gate_NOT(i, offset, fp))
        {
            offset++;
            continue;
        }

        if(is_in_gate_NAND(i, offset, fp))
        {
            offset++;
            continue;
        }

        if(is_in_gate_NOR(i, offset, fp))
        {
            offset++;
            continue;
        }
    }

    fclose(fp);
}

bool is_in_gate_NOT(int mos, int offset, FILE *fp)
{    
    /*
    In order for this transistor to belong in the same Gate-Not with another transistor,
    we check the below conditions according to the library file:
    1. Common Node Gate.
    2. 2 transistors PMOS, NMOS in series connection.
    3. Source of PMOS is VCC-Node.
    4. Source of NMOS is GND-Node.
    */
    
    // Condition 1.
    int mos2 = -1;
    for(int i=0; i<numOfMos; i++)
    {
        //Dont check with the same transistor
        //or if already participates in a gate. Continue with the next one.
        if(i==mos || DAG[i][4] != -1)
            continue;

        //Common Gate and different type of CMOS. We found it so break.
        if(DAG[i][1] == DAG[mos][1] && DAG[i][0] != DAG[mos][0])
        {
            mos2 = i;
            break;
        }
    }
    //If transistor not found.
    if(mos2 == -1)
        return false;

    if(DAG[mos][0] == 'P')
    {
        //In series connected
        if(DAG[mos][3] != DAG[mos2][2])
            return false;

        //Condition 3,4.
        if(DAG[mos][2] != VCC || DAG[mos2][3] != GND)
            return false;

        Gate_output = DAG[mos][3];
    }
    else
    {
        //In series connected
        if(DAG[mos][2] != DAG[mos2][3])
            return false;

        //Condition 3,4.
        if(DAG[mos][3]!=GND || DAG[mos2][2]!=VCC)
            return false;

        Gate_output = DAG[mos][2];
    }

    DAG[mos][4] = offset;
    DAG[mos2][4] = offset;

    Gate_input[0] = DAG[mos][1];
    Gate_input[1] = -1;
    
    fprintf(fp, "G%d NOT ; IN %d ; OUT %d\n", offset, DAG[mos][1], DAG[mos][2]);
    derive_subgraphs_file("NOT", offset);
    return true;
}

bool is_in_gate_NAND(int mos, int offset, FILE *fp)
{
    int pmos1 = -1;
    int pmos2 = -1;
    int nmos1 = -1;
    int nmos2 = -1;

    if(DAG[mos][0] == 'P')//If the current transistor is PMOS
    {
        pmos1 = mos;

        //In NAND gate we have 2 PMOS in parallel conenction. Let's find the other one.        
        for(int i=0; i<numOfMos; i++)
        {
            if(i==pmos1 || DAG[i][4] != -1)
                continue;

            //In parallel connection we have common sources and drains.
            if(DAG[i][0]=='P' && DAG[i][2]==DAG[pmos1][2] && DAG[i][3]==DAG[pmos1][3])
            {
                pmos2 = i;
                break;
            }
        }
        //We couldn't find the other paraller PMOS.
        if(pmos2 == -1)
            return false;
        
        //These 2 paraller PMOS are connected in series with one NMOS. Let's find it.
        for(int i=0; i<numOfMos; i++)
        {
            if(DAG[i][0]=='P' || DAG[i][4] != -1)
                continue;
            
            if(DAG[pmos1][3] == DAG[i][2] && DAG[pmos2][3] == DAG[i][2])
            {
                nmos1 = i;
                break;
            }
        }

        //We couldn't find the NMOS.
        if (nmos1 == -1)
            return false;
        
        //This NMOS is in series connection with one more NMOS.
        for(int i=0; i<numOfMos; i++)
        {
            if(i==nmos1 || DAG[i][0]=='P' || DAG[i][4] != -1)
                continue;

            if (DAG[nmos1][3] == DAG[i][2])
            {
                nmos2 = i;
                break;
            }
        }

        //We couldn't find the NMOS.
        if(nmos2 == -1)
            return false;

        /* //If these 2 NMOS have the same gate-node, then isn't acceptable.
        if(DAG[nmos1][1] == DAG[nmos2][1])
            return false; */

        if(DAG[pmos1][2]!=VCC || DAG[pmos2][2]!=VCC || DAG[nmos2][3]!=GND)
            return false;

        DAG[pmos1][4] = offset;
        DAG[pmos2][4] = offset;
        DAG[nmos1][4] = offset;
        DAG[nmos2][4] = offset;

        Gate_input[0] = DAG[pmos1][1];
        Gate_input[1] = DAG[pmos2][1];
        Gate_output = DAG[pmos1][3];

        fprintf(fp, "G%d NAND_2 ; IN %d, %d ; OUT %d\n", offset, DAG[pmos1][1], DAG[pmos2][1], DAG[pmos1][3]);

        derive_subgraphs_file("NAND_2", offset);
        return true;
    }

    return false;
}

bool is_in_gate_NOR(int mos, int offset, FILE *fp)
{
    int pmos1 = -1;
    int pmos2 = -1;
    int nmos1 = -1;
    int nmos2 = -1;

    if (DAG[mos][0] == 'N') //If the current transistor is NMOS
    {
        nmos1 = mos;
        //In NOR gate we have 2 NMOS in parallel conenction. Let's find the other one.
        for (int i = 0; i < numOfMos; i++)
        {
            if (i == nmos1 || DAG[i][4] != -1)
                continue;

            //In parallel connection we have common sources and drains.
            if (DAG[i][0] == 'N' && DAG[i][2] == DAG[nmos1][2] && DAG[i][3] == DAG[nmos1][3])
            {
                nmos2 = i;
                break;
            }
        }
        //We couldn't find the other paraller NMOS.
        if (nmos2 == -1)
            return false;

        //These 2 paraller NMOS are connected in series with one PMOS. Let's find it.
        for (int i = 0; i < numOfMos; i++)
        {
            if (DAG[i][0] == 'N' || DAG[i][4] != -1)
                continue;

            if (DAG[nmos1][2] == DAG[i][3] && DAG[nmos2][2] == DAG[i][3])
            {
                pmos2 = i;
                break;
            }
        }
        //We couldn't find the PMOS.
        if (pmos2 == -1)
            return false;

        //This PMOS is in series connection with one more PMOS.
        for (int i = 0; i < numOfMos; i++)
        {
            if (i == pmos2 || DAG[i][0] == 'N' || DAG[i][4] != -1)
                continue;

            if (DAG[pmos2][2] == DAG[i][3])
            {
                pmos1 = i;
                break;
            }
        }

        //We couldn't find the NMOS.
        if (pmos1 == -1)
            return false;
        /* //If these 2 NMOS have the same gate-node, then isn't acceptable.
        if (DAG[pmos1][1] == DAG[pmos2][1])
            return false; */
        if (DAG[pmos1][2] != VCC || DAG[nmos1][3] != GND || DAG[nmos2][3] != GND)
            return false;

        DAG[pmos1][4] = offset;
        DAG[pmos2][4] = offset;
        DAG[nmos1][4] = offset;
        DAG[nmos2][4] = offset;

        Gate_input[0] = DAG[nmos1][1];
        Gate_input[1] = DAG[nmos2][1];
        Gate_output = DAG[nmos1][2];
        
        fprintf(fp, "G%d NOR_2 ; IN %d, %d ; OUT %d\n", offset, DAG[nmos1][1], DAG[nmos2][1], DAG[nmos1][2]);

        derive_subgraphs_file("NOR_2", offset);
        return true;
    }

    return false;
}

void derive_subgraphs_file(char *Gate, int offset)
{

    fprintf(fp_out, "--SUBGRAPH %d - GATE:%s\n", offset, Gate);
    fprintf(fp_out, "## RAILS\n");
    fprintf(fp_out, "VCC %d ; GND %d\n", VCC, GND);

    fprintf(fp_out, "## INPUTS\n");
    int i=0;
    while(Gate_input[i] != -1 && i < 2)
    {
        fprintf(fp_out, "%d ; ", Gate_input[i]);
        i++;
    }
    fprintf(fp_out, "\n");

    fprintf(fp_out, "## OUTPUTS\n");
    fprintf(fp_out, "%d\n", Gate_output);

    fprintf(fp_out, "## NETLIST\n");
    for(i=0; i<numOfMos; i++)
    {
        if(DAG[i][4] == offset)
        {
            fprintf(fp_out, "U%d ", DAG[i][5]);
            if(DAG[i][0] == 'P')
                fprintf(fp_out, "PMOS ");
            else
                fprintf(fp_out, "NMOS ");

            fprintf(fp_out, "%d %d %d\n", DAG[i][1], DAG[i][2], DAG[i][3]);
        }
    }
    fprintf(fp_out, "\n");
}
/* ========== UTILITY FUNCTIONS ========== */

void store_circuit_infos(FILE *fp)
{
    char *lineBuffer = NULL;
    char tmp_line[64];
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
            int U_token;

            sprintf(tmp_line, "%s", lineBuffer);
            token = strtok(tmp_line, delims);
            U_token = atoi(strtok(token, "U"));

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
            DAG[numOfMos][5] = U_token;
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
        printf("Simulating File \"%s\"... \n", argv[1]);
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
    printf("VCC: %d\n", VCC);
    printf("GND: %d\n", GND);

    for(int i=0; i<numOfInputs; i++)
        printf("INPUT[%d] = %d\n", i, INPUT[i]);

    for (int i = 0; i < numOfOutputs; i++)
        printf("OUTPUT[%d] = %d\n", i, OUTPUT[i]);

    for(int i=0; i<numOfMos; i++)
    {
        printf("U%d ", DAG[i][5]);
        if(DAG[i][0] == 'P')
            printf("P ");
        else
            printf("N ");

        printf("%d %d %d %d", DAG[i][1], DAG[i][2], DAG[i][3], DAG[i][4]);
        printf("\n");
    } 

    getchar();
}