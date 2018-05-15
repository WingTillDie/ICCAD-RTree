#include <stdio.h>
#include "rtree.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define min_width       65.0
#define min_space       65.0
#define max_fill_width  1300.0
#define min_density     0.4
#define max_density     1.0
#define window_width    10000
#define layer           9
#define alpha            -0.075


static  RTREEMBR chip_boundary;
static  double  fill_id = -1.0;
static  int now_layer;

typedef struct netlist{
double num;
double xmin;
double ymin;
double xmax;
double ymax;
double no_use1;
int layer_num;
char* no_use3;
struct netlist *next;
}net;
net *front=NULL;

void dummymetalinsert(RTREENODE **node, RTREEMBR *window, double fill_width, double space, FILE *fptr);
void layer_dummy_insert(RTREENODE *root, FILE *fptr);

void read(char  file_name[]){
    net *tmp, *rear=NULL;
    FILE *fp;
    char buffer[256];
    char *delim = " ";
    fp = fopen(file_name, "r");     /* open file pointer */
    if(fp){// if file exist...
        fgets(buffer, 256, fp);
        chip_boundary.bound[0] = atof(strtok(buffer, delim));
        chip_boundary.bound[1] = atof(strtok(NULL, delim));
        chip_boundary.bound[2] = atof(strtok(NULL, delim));
        chip_boundary.bound[3] = atof(strtok(NULL, ":"));
        while(fgets(buffer, 256, fp)!=NULL){
            tmp = (net*)malloc(sizeof(net));
            tmp->next = NULL;
            tmp->num = atof(strtok(buffer, delim));
            tmp->xmin = atof(strtok(NULL, delim));
            tmp->ymin = atof((strtok(NULL, delim)));
            tmp->xmax = atof((strtok(NULL, delim)));
            tmp->ymax = atof((strtok(NULL, delim)));
            tmp->no_use1 = atof((strtok(NULL, delim)));
            tmp->layer_num = atoi((strtok(NULL, delim)));
            tmp->no_use3 = strtok(NULL, delim);
            if(front == NULL){
                front = tmp;
                rear = tmp;
            }
            else{
                rear->next = tmp;
                rear = tmp;
            }
        }
    }
    fclose(fp);
}


int main()
{
    char    filename[] = "circuit2";
    char    input_file_name[20];
    char    output_file_name[20];
    strcpy(input_file_name, filename);
    strcpy(output_file_name, filename);
    strcat(input_file_name, ".cut");
    strcat(output_file_name, ".out");

    RTREENODE* root[layer];
    for(int i=0; i<layer; i++)
        root[i] = RTreeCreate();

    read(input_file_name);
    net *point = front;

    printf("finished reading\n");
    while(point!=NULL){
        RTREEMBR test_rects ={point->xmin, point->ymin, point->xmax, point->ymax};
        RTreeInsertRect(&test_rects, point->num, &root[point->layer_num - 1], 0);
        point=point->next;
    }
    printf("finished building rtree\n");
    FILE *fPtr;
    fPtr = fopen(output_file_name,"w");
    for(int i=0; i<layer; i++){
        now_layer = now_layer + 1;
        layer_dummy_insert(root[i], fPtr);
        printf("Layer %d completed\n", i+1);;
    }
    fclose(fPtr);
    printf("completed\n");
    //PrintAllTheLeaves(root);
    //RTreePrintNode( root, 0);
    for(int i=0; i<layer; i++)
        RTreeDestroy (root[i]);
    return 0;
}

void layer_dummy_insert(RTREENODE *root, FILE *fPtr){
    RTREEMBR window_rect = {chip_boundary.bound[0], chip_boundary.bound[1], chip_boundary.bound[0] + window_width, chip_boundary.bound[1] + window_width};
    while(window_rect.bound[3] <= chip_boundary.bound[3]){
        while(window_rect.bound[2] <= chip_boundary.bound[2]){
            //printf("Window rect : %f %f %f %f\n", window_rect.bound[0], window_rect.bound[1], window_rect.bound[2], window_rect.bound[3]);
            //printf("Initial window rect density : %f\n", RTreeSearchDensity(root, &window_rect));
            double  fill_width = max_fill_width;
            while(RTreeSearchDensity(root, &window_rect) <= min_density && fill_width >= min_width){
                //printf("Dummy metal fill width : %f Inserted window density : %.3f\n", fill_width, RTreeSearchDensity(root, &window_rect));
                dummymetalinsert(&root, &window_rect, fill_width, min_space, fPtr);
                fill_width = ceil(fill_width * exp(alpha));
            }
            if(fill_width < min_width && RTreeSearchDensity(root, &window_rect) < min_density)
                printf("%f %f %f %f density insufficient\n", window_rect);
            //printf("Dummy metal fill width : %.0f Inserted window density : %.3f Window : %.0f %.0f %.0f %.0f\n", fill_width, RTreeSearchDensity(root, &window_rect), window_rect);
            window_rect.bound[0] = window_rect.bound[0] + window_width / 2;
            window_rect.bound[2] = window_rect.bound[0] + window_width;
        }
        window_rect.bound[1] = window_rect.bound[1] + window_width / 2;
        window_rect.bound[3] = window_rect.bound[1] + window_width;
        window_rect.bound[0] = chip_boundary.bound[0];
        window_rect.bound[2] = window_rect.bound[0] + window_width;
    }
}

void dummymetalinsert(RTREENODE **node, RTREEMBR *window, double fill_width, double space, FILE *fPtr){
    double  x = window->bound[0];
    double  y = window->bound[1];
    while(y + 2 * space + fill_width <= window->bound[3]){
        while(x + 2 * space + fill_width <= window->bound[2]){
            RTREEMBR outer_rect = {x, y, x + 2 * space + fill_width, y + 2 * space + fill_width};
            if(!RTreeLeafOverlap(*node, &outer_rect)){
                RTREEMBR inner_rect = {x + space, y + space, x + space + fill_width, y + space + fill_width};
                RTreeInsertRect(&inner_rect, /*-1*/fill_id, node, 0);
                fprintf(fPtr, "%.f %.f %.f %.f %.f 0 %d fill\n", -fill_id, inner_rect.bound[0], inner_rect.bound[1], inner_rect.bound[2], inner_rect.bound[3], now_layer);
                fill_id--;
                x = x + space + fill_width;
            }
            else x = x + space ;
        }
        y = y + space ;
        x = window->bound[0];
    }
}
