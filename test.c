#include <stdio.h>
#include "rtree.h"
#include <math.h>
#include <stdlib.h>

#define min_width       65.0
#define min_space       65.0
#define max_fill_width  1300.0
#define min_density     0.4
#define max_density     1.0
#define window_width    10000
#define layer           9
#define alpha           0.9
static RTREEMBR chip_boundary;


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



void dummymetalinsert(RTREENODE **node, RTREEMBR *window, double fill_width, double space);
void layer_dummy_insert(RTREENODE *root);

void read(){
    net *tmp, *rear=NULL;
    FILE *fp;
    char buffer[256];
    char *delim = " ";
    fp = fopen("circuit2.cut","r");     /* open file pointer */
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

    RTREENODE* root[layer];
    for(int i=0; i<layer; i++)
        root[i] = RTreeCreate();

    read();
    net *point = front;

    printf("finished reading\n");
    while(point!=NULL){
        RTREEMBR test_rects[]={point->xmin, point->ymin, point->xmax, point->ymax};
        //printf("%f\n", point->num);;
        RTreeInsertRect(&test_rects, point->num, &root[point->layer_num - 1], 0);
        point=point->next;
    }
    printf("finished building rtree\n");

    for(int i=0; i<layer; i++){
        layer_dummy_insert(root[i]);
        printf("Layer %d completed\n", i);;
    }

    printf("completed\n");
    //PrintAllTheLeaves(root);
    //RTreePrintNode( root, 0);
    for(int i=0; i<layer; i++)
        RTreeDestroy (root[i]);
    return 0;
}

void layer_dummy_insert(RTREENODE *root){
    RTREEMBR window_rect = {chip_boundary.bound[0], chip_boundary.bound[1], chip_boundary.bound[0] + window_width, chip_boundary.bound[1] + window_width};
    while(window_rect.bound[3] <= chip_boundary.bound[3]){
        while(window_rect.bound[2] <= chip_boundary.bound[2]){
            //printf("Window rect : %f %f %f %f\n", window_rect);
            //printf("Initial window rect density : %f\n", RTreeSearchDensity(root, &window_rect));
            double  fill_width = max_fill_width;
            while(RTreeSearchDensity(root, &window_rect) <= min_density && fill_width >= min_width){
                //printf("Dummy metal fill width : %f\n", fill_width);
                dummymetalinsert(&root, &window_rect, fill_width, min_space);
                fill_width = ceil(fill_width * alpha);
            }
            if(fill_width < min_width && RTreeSearchDensity(root, &window_rect) <= min_density)
                printf("%f %f %f %f density insufficient", window_rect);
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

void dummymetalinsert(RTREENODE **node, RTREEMBR *window, double fill_width, double space){
    double  x = window->bound[0];
    double  y = window->bound[1];
    int i = 0;
    while(y + 2 * space + fill_width <= window->bound[3]){
        while(x + 2 * space + fill_width <= window->bound[2]){
            RTREEMBR outer_rect = {x, y, x + 2 * space + fill_width, y + 2 * space + fill_width};
            if(!RTreeLeafOverlap(*node, &outer_rect)){
                RTREEMBR inner_rect = {x + space, y + space, x + space + fill_width, y + space + fill_width};
                RTreeInsertRect(&inner_rect, -1, node, 0);
                //printf("%f, %f, %f, %f\n", inner_rect);
            }
            x = x + space ;
        }
        y = y + space ;
        i++;
        if(i%2) x = window->bound[0] + fill_width / 2.0;
        else    x = window->bound[0];

    }
}
