#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "DummyInsert.h"

/*
void printTest(char *a)
{
	printf("%s ", a);
	//cout << a << " ";
}*/

#if 0
class file
{
public:
	ifstream fp;
	getline(char * c, int i)
	{
		if(fp.getline(c, i))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	file(const char *filename)
	{
		fp.open(filename);
	}
	~file()
	{
		fp.close();
	}
};
#endif

void process(char filename[]);
void Insertion_sort(int id);

int main(int argc, char **argv)
{
	/*
	char a[100];
	printf("Input the filename(ex: circuit1.conf, circuit2.config...)\n");
	scanf("%s", &a);
	printf("%s", a);
	process(a);
	*//*
if(argc==2)
process(argv[1]);
else
printf("USAGE: ./cada025 <config filename>\ne.g. ./cada025 circuit1.conf\n");*/


	process("circuit1.conf");
	process("circuit2.config");
	process("circuit3.config");
	process("circuit4.config");
	process("circuit5.config");

	return 0;

}


void process(char filename[])
{
	total_layer = 0;
	FILE *fp=fopen(filename, "r");
	//file fp("input.conf");
	//ifstream designFile, outputFile, rule_fileFile, process_fileFile;
	FILE *ruleFile, *processFile;
	char line[10000];
	char *character;
	char design[20];
	char *critical;
	char output[20], rule_file[20], process_file[20];
	char *get_first;
	int index, i;
	if(fp==NULL)
	{
		fprintf(stderr, "Open config file %s failed, please ensure that the file exist.\n", filename);
		exit(EXIT_FAILURE);
	}
	while(/*fp.getline(line, 999)*/fgets(line, sizeof(line), fp))
	{
		if(line[0]==';'||line[0]==' ')
		{
			continue;
		}
		strtok(line, ":");
		switch(line[0])
		{
		case 'd':
			//strcpy(design, strtok(NULL, ": "));
			//character = strtok(NULL, ": ");
			//printf("%s", strtok(NULL, ": "));
			//cout <<strtok(NULL, ": ");
			//strtok(NULL, "<");
			//strtok(NULL, "\n")+1;
			strcpy(design, strtok(NULL, "\n")+1);
			//printf("%s", strtok(NULL, "\n")+1);//Windows only
			//designFile=fopen(strtok(NULL, "\r")+1, "r");
			//designFile.open(strtok(NULL, ">"));
			break;
		case 'o':
			//strtok(NULL, "<");
			//strcpy(output, strtok(NULL, ": "));
			//strtok(NULL, ">");
			//printf("%s", strtok(NULL, "\n")+1);
			strcpy(output, strtok(NULL, "\n")+1);
			//outputFile.open(strtok(NULL, ">"));
			break;
		case 'r':
			//strtok(NULL, "<");
			//strcpy(rule_file, strtok(NULL, ": "));
			strcpy(rule_file, strtok(NULL, "\n")+1);
			ruleFile=fopen(rule_file, "r");
			if(ruleFile==NULL)
			{
				fprintf(stderr, "Open rule file %s failed, please ensure that the file exist.\n", rule_file);
				exit(EXIT_FAILURE);
			}
			//strtok(NULL, ">");
			//rule_fileFile.open(strtok(NULL, ">"));
			break;
		case 'p':
			//strcpy(process_file, strtok(NULL, ": "));
			if( line[1]=='r')
			{
				//strtok(NULL, "<");
				strcpy(process_file, strtok(NULL, "\n")+1);
				processFile=fopen(process_file, "r");
				if(processFile==NULL)
				{
					fprintf(stderr, "Open process file %s failed, please ensure that the file exist.\n", process_file);
					exit(EXIT_FAILURE);
				}
				//strtok(NULL, ">");
				//process_fileFile.open(strtok(NULL, ">"));
				break;
			}
			else if( line[1]=='o')
				break;
		case 'c':
			critical_net_head = NULL;
			while((critical = strtok(NULL, " \n")) != NULL)
			{
				Insertion_sort(atoi(critical));
			}
			break;
		case 'g':
			break;
		default:
			break;
		}
	}
	fclose(fp);
	//rtree(design, output);

	//printf("%s", design);
	//printf("%s\n%s\n%s\n%s", design, output, rule_file, process_file);
	//fgets(line, sizeof(line), ruleFile);

	while(fgets(line, sizeof(line), ruleFile))
	{
		if(line[0]==';'||line[0]==' ')
		{
			continue;
		}
		if(line[0]=='\n')
		{
			break;
		}
		total_layer++;
		index=atoi(strtok(line, " "))-1;
		strtok(NULL, " ");//Type
		//printf("%s", strtok(NULL, " "));
		//printf("%f", atof(strtok(NULL, " ")));
		min_width[index]=atof(strtok(NULL, " "));
		min_space[index]=atof(strtok(NULL, " "));
		max_fill_width[index]=atof(strtok(NULL, " "));
		min_density[index]=atof(strtok(NULL, " "));
		max_density[index]=atof(strtok(NULL, " "));
	}
	/*
	int i=0;
	printf("%f %f %f %f %f\n", min_width[i], min_space[i], max_fill_width[i], min_density[i], max_density[i]);
	*/
	while(fgets(line, sizeof(line), processFile))
	{
		//printf("%s", line);
		if(line[0]==';'||line[0]==' '||line[0]=='\n')
		{
			continue;
		}
		if(line[0]=='w')
		{
			strtok(line, " ");
			window_width=atof(strtok(NULL, "\n"));
			break;
		}
	}
	/*
	for(i=0; i<total_layer; i++){
	    printf("%f %f %f %f %f\n", min_width[i], min_space[i], max_fill_width[i], min_density[i], max_density[i]);
	}*/

	fclose(ruleFile);
	fclose(processFile);
	rtree(design, output);
	/*
	fgets(line, sizeof(line), designFile);
	printf("%s ", line);
	*/

	//cout << design;
	/*
	printTest(design);
	printTest(output);
	printTest(rule_file);
	printTest(process_file);
	*/

	/*
	designFile.getline(line, 999);
	cout << line << endl;
	designFile.close();


	outputFile.getline(line, 999);
	cout << line << endl;
	outputFile.close();

	rule_fileFile.getline(line, 999);
	cout << line << endl;
	rule_fileFile.close();

	process_fileFile.getline(line, 999);
	cout << line << endl;
	process_fileFile.close();
	*/

	//auto close file?
	//p();
}

void Insertion_sort(int id)
{
	critical_net	*tmp;
	tmp = (critical_net*)malloc(sizeof(critical_net));
	tmp->id = id;
	tmp->next = NULL;
	if(critical_net_head == NULL)
	{

		critical_net_head = tmp;
	}
	else if(tmp->id > critical_net_head->id)
	{
		tmp->next = critical_net_head;
		critical_net_head = tmp;
	}
	else
	{
		critical_net  *prev = critical_net_head;
		critical_net  *current = critical_net_head->next;
		while(current != NULL && current->id > tmp->id)
		{
			prev = current;
			current = current->next;
		}
		prev->next = tmp;
		tmp->next = current;
	}
}




