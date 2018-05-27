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
#define total_layer     9
#define alpha           -0.075


static  RTREEMBR chip_boundary;
static  double  fill_id = 0;


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

void dummymetalinsert(RTREENODE **node, RTREEMBR *window, double fill_width, double space, FILE *fptr, int layer);
void layer_dummy_insert(RTREENODE *root, FILE *fptr, int layer);

void read(char  file_name[]){
    net *tmp, *rear=NULL;
    FILE *fPtr;
    char buffer[256];
    char *delim = " ";
    fPtr = fopen(file_name, "r");     /* open file pointer */
    if(fPtr){// if file exist...
        fgets(buffer, 256, fPtr);
        chip_boundary.bound[0] = atof(strtok(buffer, delim));
        chip_boundary.bound[1] = atof(strtok(NULL, delim));
        chip_boundary.bound[2] = atof(strtok(NULL, delim));
        chip_boundary.bound[3] = atof(strtok(NULL, ":"));
        while(fgets(buffer, 256, fPtr)!=NULL){
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
    fclose(fPtr);
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

    /** For svg*/
    //strcat(output_file_name, ".svg");

    RTREENODE* root[total_layer];
    for(int i=0; i<total_layer; i++)
        root[i] = RTreeCreate();

    read(input_file_name);
    net *point = front;

    printf("finished reading\n");
    FILE *fPtr;
    fPtr = fopen(output_file_name,"w");

    /** For svg*/
    //printPrev(fPtr);


    while(point!=NULL){
        RTREEMBR test_rects ={{point->xmin, point->ymin, point->xmax, point->ymax}};
        RTreeInsertRect(&test_rects, point->num, &root[point->layer_num - 1], 0);
        fill_id++;
        /** For svg*/
        /*if(point->layer_num == 1)
            fprintf(fPtr, "<rect width=\"%f\" height=\"%f\" x=\"%f\" y=\"%f\" style=\"fill:#000000;stroke:none;stroke-width:0;stroke-miterlimit:4;stroke-dasharray:none;fill-opacity:1\"/>\n", point->xmax - point->xmin, point->ymax - point->ymin, point->xmin, point->ymin);
        */
        point=point->next;
    }
    printf("%f\n", fill_id);
    printf("finished building rtree\n");


    for(int i=0; i<total_layer; i++){
        /** For svg*/
        /*
        if(i != 0)
            break;
        */
        layer_dummy_insert(root[i], fPtr, i+1);
        printf("Layer %d completed\n", i+1);;
    }

    /** For svg*/
    //printEnd(fPtr);

    fclose(fPtr);
    printf("completed\n");
    for(int i=0; i<total_layer; i++)
        RTreeDestroy (root[i]);
    return 0;
}

void layer_dummy_insert(RTREENODE *root, FILE *fPtr, int layer){
    RTREEMBR window_rect = {{chip_boundary.bound[0], chip_boundary.bound[1], chip_boundary.bound[0] + window_width / 2.0, chip_boundary.bound[1] + window_width / 2.0}};
    while(window_rect.bound[3] <= chip_boundary.bound[3]){
        while(window_rect.bound[2] <= chip_boundary.bound[2]){
            /** For svg (draw window rect)
            fprintf(fPtr, "<rect width=\"%f\" height=\"%f\" x=\"%f\" y=\"%f\" style=\"fill:none;stroke:#000000;stroke-width:10;stroke-miterlimit:4;stroke-dasharray:none;fill-opacity:1;opacity:1;stroke-opacity:1\"/>\n", window_rect.bound[2] - window_rect.bound[0], window_rect.bound[3] - window_rect.bound[1], window_rect.bound[0], window_rect.bound[1]);
            */
            double  fill_width = max_fill_width;
            while(RTreeSearchDensity(root, &window_rect) <= min_density && fill_width >= min_width){
                dummymetalinsert(&root, &window_rect, fill_width, min_space, fPtr, layer);
                fill_width = ceil(fill_width * exp(alpha));
            }
            if(fill_width < min_width && RTreeSearchDensity(root, &window_rect) < min_density){
                fill_width = min_width;
                dummymetalinsert(&root, &window_rect, fill_width, min_space, fPtr, layer);
                if(fill_width < min_width && RTreeSearchDensity(root, &window_rect) < min_density){
                    printf("%f %f %f %f density insufficient\nDensity %f\n", window_rect, RTreeSearchDensity(root, &window_rect));
                    /** For svg (draw density insufficient region)
                    fprintf(fPtr, "<rect width=\"%f\" height=\"%f\" x=\"%f\" y=\"%f\" style=\"fill:none;stroke:#fd0000;stroke-width:100;stroke-miterlimit:4;stroke-dasharray:none;fill-opacity:1;opacity:1;stroke-opacity:1\"/>\n", window_rect.bound[2] - window_rect.bound[0], window_rect.bound[3] - window_rect.bound[1], window_rect.bound[0], window_rect.bound[1]);
                    */
                }
            }
            window_rect.bound[0] = window_rect.bound[0] + window_width / 2.0;
            window_rect.bound[2] = window_rect.bound[0] + window_width / 2.0;
        }
        window_rect.bound[1] = window_rect.bound[1] + window_width / 2.0;
        window_rect.bound[3] = window_rect.bound[1] + window_width / 2.0;
        window_rect.bound[0] = chip_boundary.bound[0];
        window_rect.bound[2] = window_rect.bound[0] + window_width / 2.0;
    }
}

void printPrev(FILE *fPtr){
    printf("%f %f\n", chip_boundary.bound[0], chip_boundary.bound[3]);
    fprintf(fPtr, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?><!-- Created with Inkscape (http://www.inkscape.org/) --><svg   xmlns:dc=\"http://purl.org/dc/elements/1.1/\"   xmlns:cc=\"http://creativecommons.org/ns#\"   xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"   xmlns:svg=\"http://www.w3.org/2000/svg\"   xmlns=\"http://www.w3.org/2000/svg\"   xmlns:sodipodi=\"http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd\"   xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\"   width=\"210mm\"   height=\"297mm\"   viewBox=\"%f %f 210 297\"   version=\"1.1\"   id=\"svg8\"   inkscape:version=\"0.92.3 (2405546, 2018-03-11)\"   sodipodi:docname=\"¹Ïµe.svg\">  <defs     id=\"defs2\" />  <sodipodi:namedview     id=\"base\"     pagecolor=\"#ffffff\"     bordercolor=\"#666666\"     borderopacity=\"1.0\"     inkscape:pageopacity=\"1\"     inkscape:pageshadow=\"2\"     inkscape:zoom=\"0.02\"     inkscape:cx=\"41203.926\"     inkscape:cy=\"-18724.963\"     inkscape:document-units=\"mm\"     inkscape:current-layer=\"layer1\"     showgrid=\"false\"     inkscape:window-width=\"1920\"     inkscape:window-height=\"1017\"     inkscape:window-x=\"-8\"     inkscape:window-y=\"-8\"     inkscape:window-maximized=\"1\"     inkscape:pagecheckerboard=\"false\"     showborder=\"false\" />  <metadata     id=\"metadata5\">    <rdf:RDF>      <cc:Work         rdf:about=\"\">        <dc:format>image/svg+xml</dc:format>        <dc:type           rdf:resource=\"http://purl.org/dc/dcmitype/StillImage\" />        <dc:title></dc:title>      </cc:Work>    </rdf:RDF>  </metadata>  <g     inkscape:label=\"¹Ï¼h 1\"     inkscape:groupmode=\"layer\"     id=\"layer1\">\n", chip_boundary.bound[0], chip_boundary.bound[3]);
}

void printEnd(FILE *fPtr){
    fprintf(fPtr, "</g></svg>");
}

void dummymetalinsert(RTREENODE **node, RTREEMBR *window, double fill_width, double space, FILE *fPtr, int layer){
    double  x = window->bound[0];
    double  y = window->bound[1];
    double  move_space;
    if(fill_width < 100)    move_space = 10.0;
    else if(fill_width < (space * 10)) move_space = space;
    else    move_space = floor(fill_width / 10);
    while(y + 2 * space + fill_width <= window->bound[3]){
        while(x + 2 * space + fill_width <= window->bound[2]){
            RTREEMBR outer_rect = {{x, y, x + 2 * space + fill_width, y + 2 * space + fill_width}};
            if(!RTreeLeafOverlap(*node, &outer_rect)){
                RTREEMBR inner_rect = {{x + space, y + space, x + space + fill_width, y + space + fill_width}};
                RTreeInsertRect(&inner_rect, ++fill_id, node, 0);
                fprintf(fPtr, "%.f %.f %.f %.f %.f 0 %d fill\n", fill_id, inner_rect.bound[0], inner_rect.bound[1], inner_rect.bound[2], inner_rect.bound[3], layer);
                /** For svg (draw dummy metal)
                if(layer == 1){
                    fprintf(fPtr, "<rect width=\"%f\" height=\"%f\" x=\"%f\" y=\"%f\" style=\"fill:#cd8088;stroke:none;stroke-width:0;stroke-miterlimit:4;stroke-dasharray:none;fill-opacity:1\"/>\n", inner_rect.bound[2] - inner_rect.bound[0], inner_rect.bound[3] - inner_rect.bound[1], inner_rect.bound[0], inner_rect.bound[1]);
                }
                */
                x = x + fill_width + move_space;
            }
            else x = x + move_space;
        }
        if(((x + move_space) < window->bound[2]) && (x + 2 * space + fill_width <= chip_boundary.bound[2])){
            RTREEMBR outer_rect = {{x, y, x + 2 * space + fill_width, y + 2 * space + fill_width}};
            if(!RTreeLeafOverlap(*node, &outer_rect)){
                RTREEMBR inner_rect = {{x + space, y + space, x + space + fill_width, y + space + fill_width}};
                RTreeInsertRect(&inner_rect, ++fill_id, node, 0);
                fprintf(fPtr, "%.f %.f %.f %.f %.f 0 %d fill\n", fill_id, inner_rect.bound[0], inner_rect.bound[1], inner_rect.bound[2], inner_rect.bound[3], layer);
                /** For svg (draw dummy metal)
                if(layer == 1){
                    fprintf(fPtr, "<rect width=\"%f\" height=\"%f\" x=\"%f\" y=\"%f\" style=\"fill:#cd8088;stroke:none;stroke-width:0;stroke-miterlimit:4;stroke-dasharray:none;fill-opacity:1\"/>\n", inner_rect.bound[2] - inner_rect.bound[0], inner_rect.bound[3] - inner_rect.bound[1], inner_rect.bound[0], inner_rect.bound[1]);
                }
                */
        }
        }
        y = y + move_space;
        x = window->bound[0];
    }
}
